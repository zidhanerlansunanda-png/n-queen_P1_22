export interface Violation {
  rule: 1 | 2 | 3;
  text: string;
}
export const ALLOWED_HEADERS: Set<string>;
export function checkArgs(o: { source: string; includes: string[]; outObject: string }): string[];
export function logicalLines(source: string): string[];
export function sourceViolations(source: string): Violation[];
export function readableName(symbol: string): string;
export function readObject(bytes: Uint8Array): {
  symbols: Array<{ kind: "function" | "data"; name: string; local?: boolean; segment?: number; segmentName?: string }>;
  initFunctions: number;
};
export function objectViolations(bytes: Uint8Array): Violation[];
export function debugViolations(lines: Iterable<string> | AsyncIterable<string>, sourcePath: string): Promise<Violation[]>;
export function checkRules(o: { source: string; objectFile: string; debugFile: string; sourcePath: string }): Promise<Violation[]>;
export function describeViolations(violations: readonly Violation[]): string;
