export interface EmccArgsOptions {
  /** The solver and the harness. */
  sources: string[];
  /** Include directories, without -I. */
  includes: string[];
  /** The module to write, solver.mjs. */
  outJs: string;
  /** WASM MAXIMUM_MEMORY in bytes. */
  maxMemBytes: number;
}
export function emccArgs(options: EmccArgsOptions): string[];
