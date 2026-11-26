// Measures the memory of one run of a solver module, from outside it. Environment-neutral: the browser worker and the Node bench use it.
//
// The memory of a run is every byte the program wrote to, on the stack and the heap, plus all of its global and static data. The host fills
// the stack and the heap with a pattern before any code of the module runs, and afterwards looks at which bytes no longer hold it. Nothing
// inside the module takes part, so it does not matter how the program got hold of the memory: malloc, an allocator of its own, a global
// array, the stack, or an address it made up.
//
// A byte that happens to be written with the value the pattern already had would be missed. Runs are therefore made in pairs, the second with
// every byte of the pattern inverted: a byte cannot equal both, so the union of the two (WrittenBytes) has every byte written.
//
// What does not count: memory that was reserved but never written, and writing the same bytes again (memory that is freed and reused counts
// once). The tester's own few KB (the problem, the harness's data) are in every figure, the same for every solver on a board.
//
// The layout of a module (driver/scripts/emcc-args.mjs): static data from GLOBAL_BASE, then the stack of STACK_BYTES, then the heap.

export const GLOBAL_BASE = 1024;
export const STACK_BYTES = 1024 * 1024;
export const PATTERN_BYTES = 65536;
const PAGE_BYTES = 65536;

export interface MemoryUse {
  /** staticBytes + stackBytes + heapBytes */
  bytes: number;
  /** All global and static data, written or not (the harness's and the C++ library's too). */
  staticBytes: number;
  /** Bytes of the stack that were written. */
  stackBytes: number;
  /** Bytes of the heap that were written. */
  heapBytes: number;
}

/** 64 KB of random bytes. Random, so that a program cannot count on what is in memory it has not written. */
export function randomPattern(random: () => number = Math.random): Uint8Array {
  const words = new Uint32Array(PATTERN_BYTES / 4);
  let x = Math.floor(random() * 0xffffffff) >>> 0 || 0x9e3779b9;
  for (let i = 0; i < words.length; i++) {
    x ^= x << 13; // xorshift32
    x ^= x >>> 17;
    x ^= x << 5;
    words[i] = x >>> 0;
  }
  return new Uint8Array(words.buffer);
}

/** The pattern of the other run of a pair. */
export const invertPattern = (pattern: Uint8Array): Uint8Array => pattern.map((b) => b ^ 0xff);

/** Writes the pattern over [from, to). The byte at address a gets pattern[a % PATTERN_BYTES]. */
export function paint(memory: Uint8Array, pattern: Uint8Array, from: number, to: number): void {
  for (let at = from; at < to; ) {
    const offset = at % PATTERN_BYTES;
    const length = Math.min(PATTERN_BYTES - offset, to - at);
    memory.set(pattern.subarray(offset, offset + length), at);
    at += length;
  }
}

const ones = (x: number): number => {
  x -= (x >>> 1) & 0x55555555;
  x = (x & 0x33333333) + ((x >>> 2) & 0x33333333);
  return Math.imul((x + (x >>> 4)) & 0x0f0f0f0f, 0x01010101) >>> 24;
};

/** Which bytes were found written: one bit for each. Marking only adds, so two runs of a pair give their union. */
export class WrittenBytes {
  private bits = new Uint32Array(0);

  /**
   * Marks the bytes of [from, to) that no longer hold the pattern, except the ones in `skip`. `from` and `to` are multiples of 4 (every
   * border of a module's memory is).
   */
  mark(memory: Uint8Array, pattern: Uint8Array, from: number, to: number, skip?: readonly [number, number]): void {
    if (this.bits.length * 32 < to) {
      const grown = new Uint32Array(Math.ceil(to / 32));
      grown.set(this.bits);
      this.bits = grown;
    }
    const words = new Uint32Array(memory.buffer, memory.byteOffset, memory.byteLength >>> 2);
    const wanted = new Uint32Array(pattern.buffer, pattern.byteOffset, PATTERN_BYTES / 4);
    const [skipFrom, skipTo] = skip ?? [0, 0];
    const bits = this.bits;
    const mask = PATTERN_BYTES / 4 - 1;
    for (let w = from >>> 2, end = to >>> 2; w < end; w++) {
      // Most of the memory is as it was filled: go over it eight words at a time.
      if ((w & 7) === 0) {
        let p = w & mask;
        while (
          w + 8 <= end &&
          ((words[w]! ^ wanted[p]!) | (words[w + 1]! ^ wanted[p + 1]!) | (words[w + 2]! ^ wanted[p + 2]!) | (words[w + 3]! ^ wanted[p + 3]!) |
            (words[w + 4]! ^ wanted[p + 4]!) | (words[w + 5]! ^ wanted[p + 5]!) | (words[w + 6]! ^ wanted[p + 6]!) | (words[w + 7]! ^ wanted[p + 7]!)) === 0
        ) {
          w += 8;
          p = (p + 8) & mask;
        }
        if (w >= end) break;
      }
      const d = words[w]! ^ wanted[w & mask]!;
      if (d === 0) continue;
      let changed = (d & 0xff ? 1 : 0) | (d & 0xff00 ? 2 : 0) | (d & 0xff0000 ? 4 : 0) | (d >>> 24 ? 8 : 0);
      if (w * 4 < skipTo && w * 4 + 4 > skipFrom) {
        for (let k = 0; k < 4; k++) if (w * 4 + k >= skipFrom && w * 4 + k < skipTo) changed &= ~(1 << k);
      }
      bits[w >>> 3]! |= changed << ((w & 7) * 4);
    }
  }

  /** How many bytes of [from, to) are marked. */
  count(from: number, to: number): number {
    const end = Math.min(to, this.bits.length * 32);
    const bits = this.bits;
    let total = 0;
    for (let at = from; at < end; ) {
      const bit = at & 31;
      const length = Math.min(32 - bit, end - at);
      const word = bits[at >>> 5]!;
      if (word !== 0) total += ones(length === 32 ? word : word & (((1 << length) - 1) << bit));
      at += length;
    }
    return total;
  }
}

/** What the probe needs of a module. Both are missing in a module that was built before the memory was measured like this. */
export interface ProbedModule {
  _nq_stack_top?(): number;
  wasmMemory?: WebAssembly.Memory;
}

/** What one run, or a pair of runs, shares: the pattern of this run, and what has been found written. */
export interface MemoryPass {
  pattern: Uint8Array;
  written: WrittenBytes;
}

export const newPass = (): MemoryPass => ({ pattern: randomPattern(), written: new WrittenBytes() });
/** The second run of a pair. */
export const pairedPass = (first: MemoryPass): MemoryPass => ({ pattern: invertPattern(first.pattern), written: first.written });

export class MemoryProbe {
  private memory?: WebAssembly.Memory;
  private stackTop = 0;
  /** The size the memory has if it grew only the way the probe sees. */
  private size = 0;
  private paintMs = 0;
  private ownGrowth = false;

  constructor(
    private readonly pass: MemoryPass,
    private readonly now: () => number,
  ) {}

  /** Call before any code of the module runs (preRun). False when the module cannot be measured. */
  attach(mod: ProbedModule): boolean {
    const memory = mod.wasmMemory;
    if (!memory || typeof mod._nq_stack_top !== "function") return false;
    const top = mod._nq_stack_top();
    if (top <= GLOBAL_BASE + STACK_BYTES - 1) return false;
    this.memory = memory;
    this.stackTop = top;
    this.size = memory.buffer.byteLength;
    this.fill(0, GLOBAL_BASE); // nothing is kept here, but a program could use it
    this.fill(top - STACK_BYTES, this.size);

    // The memory grows through this method (Emscripten's emscripten_resize_heap): what is added gets the pattern too.
    const grow = memory.grow.bind(memory);
    memory.grow = (pages: number): number => {
      if (memory.buffer.byteLength !== this.size) this.ownGrowth = true;
      const before = grow(pages);
      const t0 = this.now();
      this.size = memory.buffer.byteLength;
      this.fill(before * PAGE_BYTES, this.size);
      this.paintMs += this.now() - t0;
      return before;
    };
    return true;
  }

  private fill(from: number, to: number): void {
    paint(new Uint8Array(this.memory!.buffer), this.pass.pattern, from, to);
  }

  /** The time spent on filling grown memory since this was last asked: it is the tester's, and is taken off the program's. */
  takePaintMs(): number {
    const ms = this.paintMs;
    this.paintMs = 0;
    return ms;
  }

  /**
   * The program grew the memory with the WebAssembly instruction itself, past the probe. What it added was not filled, so the memory of this
   * run cannot be measured.
   */
  get grewByItself(): boolean {
    return this.ownGrowth || (this.memory !== undefined && this.memory.buffer.byteLength !== this.size);
  }

  /** Looks at the memory as it is now. `skip`: a range that is not to be looked at, because the tester wrote it. */
  mark(skip?: readonly [number, number]): void {
    if (!this.memory) return;
    const bytes = new Uint8Array(this.memory.buffer);
    const { pattern, written } = this.pass;
    written.mark(bytes, pattern, 0, GLOBAL_BASE, skip);
    written.mark(bytes, pattern, this.stackTop - STACK_BYTES, bytes.length, skip);
  }

  /** The figure so far. Undefined for a module that cannot be measured. */
  use(): MemoryUse | undefined {
    if (!this.memory) return undefined;
    const { written } = this.pass;
    const stackLow = this.stackTop - STACK_BYTES;
    const staticBytes = stackLow - GLOBAL_BASE + written.count(0, GLOBAL_BASE);
    const stackBytes = written.count(stackLow, this.stackTop);
    const heapBytes = written.count(this.stackTop, this.memory.buffer.byteLength);
    return { bytes: staticBytes + stackBytes + heapBytes, staticBytes, stackBytes, heapBytes };
  }
}
