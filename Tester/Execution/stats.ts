// Statistics of repeated timings. Pure.
import type { TimeStats } from "./protocol";

/** A spread above this share of the median means the machine was busy, and the time is not to be trusted. */
export const NOISY_SPREAD = 0.05;
/**
 * Below this a run is too short to time well: browsers round timers, and in the measurements this was chosen from the
 * time of runs under a millisecond still moved by several percent.
 */
export const RELIABLE_MS = 1;

export function median(values: readonly number[]): number {
  if (values.length === 0) throw new Error("median of no values");
  const s = [...values].sort((a, b) => a - b);
  const mid = s.length >> 1;
  return s.length % 2 ? s[mid]! : (s[mid - 1]! + s[mid]!) / 2;
}

/** Median absolute deviation from the median. */
export function mad(values: readonly number[]): number {
  const m = median(values);
  return median(values.map((v) => Math.abs(v - m)));
}

/** `samples` are the timed runs; `warmupMs` is a first run that is not among them, when there is one. */
export function summarize(samples: readonly number[], warmupMs?: number): TimeStats {
  return {
    minMs: Math.min(...samples),
    medianMs: median(samples),
    madMs: mad(samples),
    samples: samples.length,
    ...(warmupMs !== undefined ? { warmupMs } : {}),
    runsMs: [...samples],
  };
}

export const isNoisy = (t: TimeStats): boolean => t.medianMs > 0 && t.madMs / t.medianMs > NOISY_SPREAD;
export const isBelowResolution = (t: TimeStats): boolean => t.minMs < RELIABLE_MS;
