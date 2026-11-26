// Environment-neutral measurement of one solver on one board, for the browser worker and the Node bench.
//
// The first run's outcome is reported at once, so that the result can be shown while the rest is still going on. It is also the warm-up, and
// its time is left out of the statistics. Then comes a fixed number of further runs, each in a fresh module, that do exactly what the first
// did and must give the same answer: a program cannot answer only the run that is checked. The time and memory are reported together when
// the last one is done. The time is the fastest of the further runs. The memory is that of the run that wrote the most.
//
// The leaderboard server asks for one more run before all of these (warmupRun), whose outcome is dropped: the first run of a module in a
// process finds it not yet compiled. With it, and with the compiler flags the server starts its runner with, the first
// run is like the others and its time counts too.
import { WARMUP_RUNS } from "../Configuration/budget";
import { queenCountAt, type SolutionSet } from "../Board/solution-set";
import { newPass, pairedPass, type MemoryUse } from "./memory-probe";
import type { MeasureReport, RunOutcome } from "./protocol";
import { runSolver, type RunOptions } from "./run-solver";
import { summarize } from "./stats";

export type { MeasureReport };

export interface MeasureOptions extends Omit<RunOptions, "emit" | "memory" | "expected"> {
  /** Timed runs after the first one. 0 runs the solver once and measures nothing. A Debug run is never repeated. */
  timedRuns: number;
  /** One discarded run before the first one, whose time then counts. Only for a run that is measured. */
  warmupRun?: boolean;
  report(msg: MeasureReport): void;
}

/**
 * Collects the garbage of the runs before, where the process allows it (node --expose-gc, as the server's runner), so that it is not done
 * in the middle of the next run's time. A browser has no such function.
 */
const collectGarbage = (): void => (globalThis as { gc?: () => void }).gc?.();

/** What two runs on one board must agree on: how many solutions, and how many boards have how many queens. */
function answerShape(set: SolutionSet): string {
  if (set.kind === "cols") return `${set.count}`;
  const sizes = new Map<number, number>();
  for (let i = 0; i < set.count; i++) {
    const q = queenCountAt(set, i);
    sizes.set(q, (sizes.get(q) ?? 0) + 1);
  }
  return `${set.count}:${[...sizes].sort((a, b) => a[0] - b[0]).join(";")}`;
}

/** Why a later run does not count: it did not answer as the first one did. The words say nothing about the board. */
function differs(shape: string, out: Extract<RunOutcome, { status: "completed" }>): string | undefined {
  if (out.invalid || out.duplicates.count > 0 || answerShape(out.set) !== shape) return "returned a different answer from the first run";
  return undefined;
}

export async function measureSolver(opts: MeasureOptions): Promise<void> {
  const { report, timedRuns, warmupRun, ...run } = opts;
  const warmed = warmupRun === true && timedRuns > 0 && !opts.debug;
  if (warmed) {
    // Exactly what every run does, so that all of it is compiled. What comes of it is dropped, a failure too: the first run will meet the
    // same and report it. Its start and end are announced, so the watchdog's cutoff applies to it as to the first run.
    await runSolver({ ...run, emit: report });
    report({ type: "warmed" });
    collectGarbage();
  }
  let pass = newPass();
  const first = await runSolver({ ...run, memory: pass, emit: report });

  // A result that is wrong has no speed worth measuring, and nothing came back from a run that failed.
  const repeat =
    timedRuns > 0 &&
    !opts.debug &&
    first.status === "completed" &&
    !first.invalid &&
    first.duplicates.count === 0;
  // Kept before the report: the browser worker hands the buffer of the result over to the page.
  const checked = repeat && first.status === "completed" ? { expected: first.set.data.slice(), shape: answerShape(first.set) } : undefined;
  report({ type: "outcome", outcome: first, timedRuns: repeat ? timedRuns : 0 });
  if (!checked || first.status !== "completed") return;

  const total = WARMUP_RUNS + timedRuns;
  // After a discarded run the first run is a sample like the others.
  const times: number[] = warmed ? [first.computeMs] : [];
  let most: MemoryUse | undefined = first.memory;
  const failed = (run_: number, why: string): void => report({ type: "metrics", measurement: { state: "failed", reason: `run ${run_} of ${total}: ${why}` } });
  try {
    for (let i = 0; i < timedRuns; i++) {
      const number = WARMUP_RUNS + i + 1;
      // Runs go in pairs: the second of a pair has the inverse of the first one's pattern, so that together they miss no byte.
      pass = (number - 1) % 2 === 1 ? pairedPass(pass) : newPass();
      collectGarbage();
      const out = await runSolver({ ...run, debug: false, memory: pass, expected: checked.expected, emit: report });
      if (out.status === "error") return failed(number, out.message);
      const why = differs(checked.shape, out);
      if (why) return failed(number, why);
      times.push(out.computeMs);
      if (out.memory && (!most || out.memory.bytes > most.bytes)) most = out.memory;
      report({ type: "measuring", done: number, total });
    }
  } catch (e) {
    report({
      type: "metrics",
      measurement: { state: "failed", reason: String((e as { message?: string } | null)?.message ?? e) },
    });
    return;
  }
  report({
    type: "metrics",
    measurement: {
      state: "done",
      metrics: {
        time: summarize(times, warmed ? undefined : first.computeMs),
        ...(most ? { memory: most } : {}),
      },
    },
  });
}
