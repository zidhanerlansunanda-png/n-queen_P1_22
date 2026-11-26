// Environment-neutral solver runner: no DOM, no `self`. The browser worker and the Node bench both wrap this, so a
// benchmark and a real run measure the same code path.
//
// One run: load the module, hand it the problem, run the solver's function, pack what it returned, and check it. Every run does all of it
// the same way, so that a program cannot tell a run that is timed from one that is checked.
//
// Timed: the call of the solver's function, and nothing else. A program has no code that runs outside it: the rules (checked when it is
// compiled, driver/scripts/check-rules.mjs) forbid global variables and redefining what the tester uses too, such as operator new.
// Not timed: loading the module, handing over the problem, packing the result, measuring the memory (memory-probe.ts) and checking.
import { findDuplicates, firstInvalid } from "../Board/solution-check";
import { MemoryProbe, newPass, type MemoryPass, type MemoryUse } from "./memory-probe";
import { indexSet, type SolutionSet } from "../Board/solution-set";
import {
  CASE_ID,
  ENTRY_POINT,
  encodeInput,
  type ErrorKind,
  type RunProgress,
  type ProblemInput,
  type RunOutcome,
} from "./protocol";
import { SnapshotStore, type SnapshotState } from "./snapshot-store";

/** The exports of Tester/Harness/nqueen_runtime.cpp, as Emscripten gives them. */
export interface SolverModule {
  _nq_alloc_input(bytes: number): number;
  _nq_prepare(kase: number, debug: number): number;
  _nq_solve(): number;
  _nq_pack(): number;
  _nq_result(): number;
  _nq_count(): number;
  _nq_bad(which: number): number;
  /** This and wasmMemory are missing in a module built before the memory was measured from outside it. */
  _nq_stack_top?(): number;
  wasmMemory?: WebAssembly.Memory;
  /** A getter in Emscripten: the view is replaced when memory grows, so read it again after every call. */
  readonly HEAPU8: Uint8Array;
}

/** What the host does while a module is being loaded. Emscripten calls these as `preRun` and `onRuntimeInitialized`. */
export interface LoadHooks {
  /** The module is instantiated and none of its code has run. */
  preRun(mod: SolverModule): void;
}

export interface RunOptions {
  input: ProblemInput;
  /** Debug run: snapshot() is recorded. The caller should not judge the time. */
  debug: boolean;
  snapshotHead: number;
  snapshotTail: number;
  /** Instantiates the WASM module, calling the hooks. Excluded from compute time. */
  loadModule: (hooks: LoadHooks) => Promise<SolverModule>;
  /** Receives the messages of a run in progress: started, snapshots and solved. */
  emit: (msg: RunProgress) => void;
  /** Milliseconds, with sub-millisecond resolution. */
  now?: () => number;
  /** Where __nqHost is installed. Default: globalThis. */
  hostTarget?: object;
  /** How often snapshots are sent to the main thread while the solver runs. */
  flushMs?: number;
  /** The pattern of this run and the bytes found written by the run it is paired with. Default: a pass of its own. */
  memory?: MemoryPass;
  /** The result bytes of an earlier run on this problem that were checked and found right: the same bytes are not checked again. */
  expected?: Uint8Array;
}

const defaultNow = (): number => performance.now();

function classify(e: unknown): ErrorKind {
  return (e as { name?: string } | null)?.name === "ExitStatus" ? "exit" : "trap";
}
const messageOf = (e: unknown): string =>
  String((e as { message?: string } | null)?.message ?? e);
const stackOf = (e: unknown): string | undefined => {
  const stack = (e as { stack?: string } | null)?.stack;
  return stack ? stack.split("\n").slice(0, 8).join("\n") : undefined;
};

const emptySet = (n: number, kind: SolutionSet["kind"]): SolutionSet => ({
  kind,
  n,
  count: 0,
  data: new Uint8Array(0),
});

type Failure = Extract<RunOutcome, { status: "error" }>;
type Ran =
  | { ok: true; mod: SolverModule; computeMs: number; packed: number; memory?: MemoryUse }
  | { ok: false; failure: Failure };

/**
 * Loads a fresh module, hands it the problem, runs the solver's function, which alone is timed, and packs what it returned. The memory is
 * looked at after the function and again after the packing, which writes the result: those bytes are the tester's. `extra` adds what a
 * failure should carry besides its message, such as the snapshots.
 */
async function execute(opts: RunOptions, extra: () => { snapshots?: SnapshotState }): Promise<Ran> {
  const { input, debug, emit } = opts;
  const now = opts.now ?? defaultNow;
  const probe = new MemoryProbe(opts.memory ?? newPass(), now);
  const failed = (kind: ErrorKind, message: string, computeMs: number, stack?: string): Ran => ({
    ok: false,
    failure: { status: "error", kind, message, ...(stack ? { stack } : {}), computeMs, ...extra() },
  });

  let mod: SolverModule;
  try {
    mod = await opts.loadModule({
      preRun: (m) => {
        probe.attach(m);
      },
    });
  } catch (e) {
    return failed("load", messageOf(e), 0, stackOf(e));
  }

  const bytes = encodeInput(input);
  try {
    const at = mod._nq_alloc_input(bytes.length);
    mod.HEAPU8.set(bytes, at);
    if (mod._nq_prepare(CASE_ID[input.kase], debug ? 1 : 0) !== 0) return failed("trap", "The problem could not be passed to the solver.", 0);
  } catch (e) {
    return failed(classify(e), messageOf(e), 0, stackOf(e));
  }

  probe.takePaintMs(); // memory that grew while the problem went in is not the solve's
  emit({ type: "started" });
  let thrown: { error: unknown } | undefined;
  let code = 0;
  const t0 = now();
  try {
    code = mod._nq_solve();
  } catch (error) {
    thrown = { error };
  }
  // Filling grown memory with the pattern is the tester's work, and is taken off.
  const computeMs = Math.max(0, now() - t0 - probe.takePaintMs());
  emit({ type: "solved", computeMs });
  if (thrown) return failed(classify(thrown.error), messageOf(thrown.error), computeMs, stackOf(thrown.error));
  if (code === 1) return failed("missing", `The solver does not define ${ENTRY_POINT[input.kase]}`, computeMs);
  if (code !== 0) return failed("trap", `The solver call ended with code ${code}.`, computeMs);

  let packed: number;
  try {
    probe.mark();
    packed = mod._nq_pack();
  } catch (e) {
    return failed(classify(e), messageOf(e), computeMs, stackOf(e));
  }

  const result = packed > 0 ? mod._nq_result() : 0;
  probe.mark(packed > 0 ? [result, result + packed] : undefined);
  if (probe.grewByItself) return failed("trap", "The program grew its memory by itself, so its memory cannot be measured.", computeMs);
  const memory = probe.use();
  return { ok: true, mod, computeMs, packed, ...(memory ? { memory } : {}) };
}

const sameBytes = (a: Uint8Array, b: Uint8Array): boolean => a.length === b.length && a.every((v, i) => v === b[i]);

export async function runSolver(opts: RunOptions): Promise<RunOutcome> {
  const { input, debug, emit } = opts;
  const now = opts.now ?? defaultNow;
  const flushMs = opts.flushMs ?? 50;
  const target = (opts.hostTarget ?? globalThis) as Record<string, unknown>;
  const kind: SolutionSet["kind"] = input.kase === "walled" ? "cells" : "cols";

  const store = debug ? new SnapshotStore(opts.snapshotHead, opts.snapshotTail) : undefined;
  let lastFlush = -Infinity; // the first snapshot is sent at once: a solver that then gets stuck would never send another
  const snapshotsNow = () => (store ? { snapshots: store.state() } : {});

  // The solver calls this from inside WASM. It is only reached in a Debug run, never in the time-critical path of a
  // normal run.
  target.__nqHost = {
    snapshot(flat: Int32Array, label: string): void {
      if (!store) return;
      const queens: Array<[number, number]> = [];
      for (let i = 0; i + 1 < flat.length; i += 2) queens.push([flat[i]!, flat[i + 1]!]);
      store.push(label, queens);
      const t = now();
      if (t - lastFlush >= flushMs) {
        lastFlush = t;
        emit({ type: "snapshots", state: store.state() });
      }
    },
  };

  try {
    const ran = await execute(opts, snapshotsNow);
    if (!ran.ok) return ran.failure;
    const { mod, computeMs, packed, memory } = ran;

    // Check. Not compute time, and the watchdog no longer applies.
    const common = { computeMs, ...(memory ? { memory } : {}) };
    if (packed < 0) {
      return {
        status: "completed",
        ...common,
        set: emptySet(input.n, kind),
        invalid: formatProblem(mod, packed, input),
        duplicates: { count: 0 },
        ...snapshotsNow(),
      };
    }
    const at = mod._nq_result();
    const set: SolutionSet = {
      kind,
      n: input.n,
      count: mod._nq_count(),
      data: mod.HEAPU8.slice(at, at + packed), // a copy: the module's memory is not ours to transfer
    };
    if (kind === "cells") indexSet(set);

    if (opts.expected && sameBytes(opts.expected, set.data)) {
      return { status: "completed", ...common, set, duplicates: { count: 0 }, ...snapshotsNow() };
    }
    const invalid = firstInvalid(set, { preplaced: input.preplaced, terrain: input.terrain });
    return {
      status: "completed",
      ...common,
      set,
      ...(invalid ? { invalid } : {}),
      duplicates: invalid ? { count: 0 } : findDuplicates(set),
      ...snapshotsNow(),
    };
  } finally {
    delete target.__nqHost;
  }
}

/** The harness could not pack the result. Says which solution had the wrong shape. */
function formatProblem(mod: SolverModule, code: number, input: ProblemInput) {
  const index = mod._nq_bad(0);
  const a = mod._nq_bad(1);
  const b = mod._nq_bad(2);
  const { n } = input;
  const board = index + 1;
  let message: string;
  if (input.kase === "walled") {
    message =
      code === -1
        ? `Board ${board} lists ${a} queens, more than a board can hold`
        : `Board ${board} has a queen at (${a},${b}), outside the ${n}x${n} board`;
  } else {
    message =
      code === -1
        ? `Solution ${board} has ${a} entries, but a solution needs exactly ${n}, one column for each row`
        : `Solution ${board}: row ${a} has column ${b}, outside 0 to ${n - 1}`;
  }
  return { index, rule: "format" as const, message, cells: [] as Array<[number, number]> };
}
