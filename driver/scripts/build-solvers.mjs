#!/usr/bin/env node
// Compiles every solver, Solver/<category>/<name>.cpp, to WASM with emcc, each one into its own module under
// Solver/dist/<category>/<name>/, and writes Solver/dist/manifest.json.
// Default: runs emcc inside a pinned emscripten/emsdk Docker image.
// A solver that breaks a rule of the competition (see check-rules.mjs and the README) is not built, and the reason is printed. A folder
// with a file NO_RULES in it is not checked.
// Use --native (or NQ_NATIVE=1) to use an emcc already on PATH instead.
import { spawnSync } from "node:child_process";
import { existsSync, mkdirSync, writeFileSync, readFileSync, rmSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import os from "node:os";
import { checkArgs, checkRules, describeViolations } from "./check-rules.mjs";
import { discoverSolvers } from "./discover.mjs";
import { emccArgs } from "./emcc-args.mjs";

const repo = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  "..",
  "..",
);
const EMSDK_TAG = process.env.EMSDK_TAG ?? "5.0.5"; // pinned; override with EMSDK_TAG
const native =
  process.argv.includes("--native") || process.env.NQ_NATIVE === "1";

const cfg = JSON.parse(
  readFileSync(
    path.join(repo, "Tester/Configuration/tester.config.json"),
    "utf8",
  ),
);
const maxMemBytes = Math.round(cfg.solverMaxMemoryMB * 1024 * 1024);

const posix = (p) => p.split(path.sep).join("/");
const rel = (p) => posix(path.relative(repo, p));

// Helper headers of the solvers that fail on purpose. Not every checkout has them.
const helpers = existsSync(path.join(repo, "driver/check/fixtures")) ? ["driver/check/fixtures"] : [];

function solverArgs(solver, outJs) {
  return emccArgs({
    sources: [solver.source, path.join(repo, "Tester/Harness/nqueen_runtime.cpp")].map(rel),
    includes: [...helpers, "Tester/Harness"],
    outJs: rel(outJs),
    maxMemBytes,
  });
}

/** Runs a tool of the toolchain, "em++" by default, in the root of the repository. */
function run(args, tool = "em++") {
  if (native) {
    const shell = process.platform === "win32";
    // Through a shell, the path of a tool with a space in it (a user's folder) is one word only in quotes.
    return spawnSync(shell && tool.includes(" ") ? `"${tool}"` : tool, args, {
      cwd: repo,
      encoding: "utf8",
      shell,
    });
  }
  const dockerArgs = ["run", "--rm", "-v", `${repo}:/src`, "-w", "/src"];
  if (process.platform !== "win32" && os.userInfo().uid >= 0) {
    dockerArgs.push("-u", `${os.userInfo().uid}:${os.userInfo().gid}`);
  }
  dockerArgs.push(`emscripten/emsdk:${EMSDK_TAG}`, tool, ...args);
  return spawnSync("docker", dockerArgs, { cwd: repo, encoding: "utf8" });
}

// Nothing to build is an error before anything is touched.
const solvers = discoverSolvers(repo);
if (solvers.length === 0) {
  console.error("No solvers found. A solver is a .cpp directly in a folder of Solver/, as Solver/<category>/<name>.cpp.");
  process.exit(1);
}

// Check the toolchain before touching Solver/dist: without one, every solver would "fail" and the
// existing builds would be lost. The first Docker run also pulls the image here.
console.log(`Checking toolchain (${native ? "em++" : `Docker, emsdk ${EMSDK_TAG}`}) ...`);
const probe = run(["--version"]);
if (probe.error || probe.status !== 0) {
  console.error(
    `Toolchain not available. Solver/dist was left unchanged.\n${
      probe.error?.message ?? `${probe.stdout ?? ""}${probe.stderr ?? ""}`
    }`,
  );
  process.exit(1);
}

/** The tool that reads debug information: part of the LLVM that comes with emsdk. */
function dwarfdumpTool() {
  if (!native) return "/emsdk/upstream/bin/llvm-dwarfdump";
  const root = spawnSync("em-config", ["LLVM_ROOT"], { encoding: "utf8", shell: process.platform === "win32" });
  return path.join((root.stdout ?? "").trim(), "llvm-dwarfdump");
}
const dwarfdump = dwarfdumpTool();

/**
 * What the rules say about a solver, as the leaderboard checks it: an error text, or undefined. A compile error is left to the build
 * itself to report.
 */
async function ruleProblem(solver, outDir) {
  const object = path.join(outDir, "check.o");
  const debug = path.join(outDir, "check.dwarf");
  try {
    const compiled = run(checkArgs({ source: rel(solver.source), includes: [...helpers, "Tester/Harness"], outObject: rel(object) }));
    if (compiled.error || compiled.status !== 0) return undefined;
    const read = run(["--debug-info", rel(object), "-o", rel(debug)], dwarfdump);
    if (read.error || read.status !== 0) return `The rules could not be checked: ${read.error?.message ?? read.stderr}`;
    const found = await checkRules({ source: readFileSync(solver.source, "utf8"), objectFile: object, debugFile: debug, sourcePath: rel(solver.source) });
    return found.length > 0 ? describeViolations(found) : undefined;
  } finally {
    rmSync(object, { force: true });
    rmSync(debug, { force: true });
  }
}

const distRoot = path.join(repo, "Solver/dist");
rmSync(distRoot, { recursive: true, force: true });
mkdirSync(distRoot, { recursive: true });

const manifest = {
  generatedAt: new Date().toISOString(),
  emsdk: native ? "native" : EMSDK_TAG,
  solvers: [],
};
let failed = 0;

for (const s of solvers) {
  const outDir = path.join(distRoot, s.category, s.name);
  mkdirSync(outDir, { recursive: true });
  const outJs = path.join(outDir, "solver.mjs");
  process.stdout.write(`Building ${s.id} ... `);
  const entry = { id: s.id, category: s.category, name: s.name, file: s.file, snapshots: s.snapshots };
  const broken = existsSync(path.join(path.dirname(s.source), "NO_RULES")) ? undefined : await ruleProblem(s, outDir);
  if (broken) {
    failed++;
    console.log("NOT BUILT");
    console.error(broken);
    manifest.solvers.push({ ...entry, ok: false, error: broken });
    continue;
  }
  const r = run(solverArgs(s, outJs));
  if (r.error || r.status !== 0) {
    failed++;
    console.log("FAILED");
    const msg = r.error
      ? String(r.error.message)
      : `${r.stdout ?? ""}${r.stderr ?? ""}`;
    console.error(msg);
    manifest.solvers.push({ ...entry, ok: false, error: msg });
    continue;
  }
  console.log("ok");
  manifest.solvers.push({
    ...entry,
    ok: true,
    module: `/Solver/dist/${s.id}/solver.mjs`,
    wasm: `/Solver/dist/${s.id}/solver.wasm`,
  });
}

writeFileSync(
  path.join(distRoot, "manifest.json"),
  JSON.stringify(manifest, null, 2),
);
console.log(
  `Manifest written: Solver/dist/manifest.json (${manifest.solvers.length - failed} ok, ${failed} failed)`,
);
process.exit(failed ? 1 : 0);
