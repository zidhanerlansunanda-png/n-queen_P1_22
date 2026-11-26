// The em++ arguments of a solver module. The build script of the local app and the leaderboard server both call this, so a
// solver is compiled with the same flags wherever it is built, and the times of the two can be compared.

/**
 * @param {object} o
 * @param {string[]} o.sources  the solver and the harness (Tester/Harness/nqueen_runtime.cpp)
 * @param {string[]} o.includes include directories, without -I
 * @param {string} o.outJs      the module to write, solver.mjs; Emscripten writes solver.wasm next to it
 * @param {number} o.maxMemBytes WASM MAXIMUM_MEMORY: a solver returns all its solutions from this memory
 */
export function emccArgs({ sources, includes, outJs, maxMemBytes }) {
  return [
    ...sources,
    ...includes.map((dir) => `-I${dir}`),
    "-O2",
    "-std=c++17",
    "-sMODULARIZE=1",
    "-sEXPORT_ES6=1",
    "-sEXPORT_NAME=createSolver",
    // worker for the browser, node for the benchmark harness (see README note)
    "-sENVIRONMENT=worker,node",
    "-sALLOW_MEMORY_GROWTH=1",
    `-sMAXIMUM_MEMORY=${maxMemBytes}`,
    // The layout the memory measurement relies on (Tester/Execution/memory-probe.ts): static data from address 1024, then the 1 MiB stack,
    // then the heap. Keep the two in step.
    "-sGLOBAL_BASE=1024",
    "-sSTACK_SIZE=1048576",
    "-sFILESYSTEM=0",
    // The harness in Tester/Harness/nqueen_runtime.cpp: the host writes the problem, runs the solver, then reads the result.
    "-sEXPORTED_FUNCTIONS=_nq_alloc_input,_nq_prepare,_nq_solve,_nq_pack,_nq_result,_nq_count,_nq_bad,_nq_stack_top",
    // wasmMemory: the host fills the free memory with a pattern before the solver runs, and sees the memory grow.
    "-sEXPORTED_RUNTIME_METHODS=HEAPU8,wasmMemory",
    // Keeps function names in the name section, so a trap shows which function it was in. Does not slow the code down.
    "--profiling-funcs",
    "-o",
    outJs,
  ];
}
