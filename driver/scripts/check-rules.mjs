// The rules a solver keeps, as far as they can be checked by a machine. The build script of the local app and the leaderboard server both call
// this, so a program is refused in both or in neither.
//
//   1  No redefining what the tester uses too: the global operator new and delete, the functions of the C and runtime libraries, and
//      anything of the standard library (std::hash for a type of one's own is the exception). No #line.
//   2  No global variables: only constants (numbers, characters and arrays of them), types and functions. A static inside a function is fine.
//   3  Only the C++ standard library and "nqueens.h".
//   (4, no stored queen placements, is checked by people.)
//
// With them, nothing of a program runs outside its solver function, which is all that is timed.
//
// What the program defines is read from a check compile of its own (CHECK_FLAGS: no optimisation, so that nothing is hidden by inlining,
// and debug information, which says in which file each thing was written), not guessed from its text. Only what is a directive of the
// preprocessor (#include, #line) is read from the text: a macro cannot make one.
import { createReadStream, readFileSync, statSync } from "node:fs";
import { createInterface } from "node:readline";
import { RUNTIME_SYMBOLS } from "./runtime-symbols.mjs";

/** The em++ arguments of the check compile. It writes an object file; nothing is linked or run. */
export const checkArgs = ({ source, includes, outObject }) => ["-c", source, ...includes.map((dir) => `-I${dir}`), "-O0", "-g", "-std=c++17", "-o", outObject];

/** Largest debug information that is read. A program of the allowed size does not come near it. */
const MAX_DEBUG_BYTES = 256 * 1024 * 1024;

const violation = (rule, text) => ({ rule, text });

// ---- the text: directives --------------------------------------------------------------------------------------------------------------

const CPP_HEADERS =
  "algorithm any array atomic barrier bit bitset charconv chrono codecvt compare complex concepts condition_variable coroutine deque exception execution filesystem format forward_list fstream functional future initializer_list iomanip ios iosfwd iostream istream iterator latch limits list locale map memory memory_resource mutex new numbers numeric optional ostream queue random ranges ratio regex scoped_allocator semaphore set shared_mutex source_location span sstream stack stdexcept stop_token streambuf string string_view strstream syncstream system_error thread tuple type_traits typeindex typeinfo unordered_map unordered_set utility valarray variant vector version";
const C_HEADERS = "assert complex ctype errno fenv float inttypes iso646 limits locale math setjmp signal stdarg stdbool stddef stdint stdio stdlib string tgmath time uchar wchar wctype";

/** What may be included between angle brackets. */
export const ALLOWED_HEADERS = new Set([
  ...CPP_HEADERS.split(" "),
  ...C_HEADERS.split(" ").flatMap((name) => [`c${name}`, `${name}.h`]),
  "ccomplex",
  "ciso646",
  "cstdalign",
]);

/**
 * The source as the preprocessor splits it into lines: backslash-newlines joined, comments blanked, the contents of raw strings dropped.
 * Ordinary strings are kept (a directive's file name may be one), and are stepped over so that a "/*" inside one starts no comment.
 */
export function logicalLines(source) {
  const text = source.replace(/\r\n?/g, "\n").replace(/\?\?=/g, "#").replace(/\?\?\//g, "\\").replace(/\\[ \t]*\n/g, "");
  let out = "";
  let i = 0;
  while (i < text.length) {
    const c = text[i];
    const next = text[i + 1];
    if (c === "/" && next === "/") {
      while (i < text.length && text[i] !== "\n") i++;
    } else if (c === "/" && next === "*") {
      const end = text.indexOf("*/", i + 2);
      i = end < 0 ? text.length : end + 2;
      out += " ";
    } else if (c === "R" && next === '"' && !/[A-Za-z0-9_]/.test(text[i - 1] ?? "") ) {
      const open = text.indexOf("(", i + 2);
      const end = open < 0 ? -1 : text.indexOf(`)${text.slice(i + 2, open)}"`, open + 1);
      i = end < 0 ? text.length : end + (open - i - 2) + 2;
      out += '""';
    } else if (c === '"' || (c === "'" && !/(^|[^A-Za-z0-9_'.])[0-9][A-Za-z0-9_'.]*$/.test(text.slice(Math.max(0, i - 40), i)))) {
      // a string or a character; a ' inside a number (1'000'000) is neither
      let j = i + 1;
      while (j < text.length && text[j] !== c && text[j] !== "\n") j += text[j] === "\\" ? 2 : 1;
      out += text.slice(i, j + 1);
      i = j + 1;
    } else {
      out += c;
      i++;
    }
  }
  return out.split("\n");
}

/** Rules 1 (#line) and 3 (what is included). */
export function sourceViolations(source) {
  const found = [];
  for (const line of logicalLines(source)) {
    const m = /^\s*(?:#|%:)\s*([A-Za-z_]+|\d+)\s*(.*)$/.exec(line);
    if (!m) continue;
    const [, directive, rest] = m;
    if (directive === "line" || /^\d+$/.test(directive)) found.push(violation(1, "#line is not allowed"));
    else if (directive === "include" || directive === "include_next" || directive === "import") {
      const angled = /^<([^>]*)>/.exec(rest);
      const quoted = /^"([^"]*)"/.exec(rest);
      if (directive !== "include") found.push(violation(3, `#${directive} is not allowed`));
      else if (angled) {
        if (!ALLOWED_HEADERS.has(angled[1].trim())) found.push(violation(3, `the header <${angled[1].trim()}> is not part of the C++ standard library`));
      } else if (quoted) {
        if (quoted[1] !== "nqueens.h") found.push(violation(3, `the header "${quoted[1]}" is not allowed; only "nqueens.h" and the standard library`));
      } else found.push(violation(3, "an #include must name its header, not a macro"));
    } else if (directive === "embed") found.push(violation(3, "#embed is not allowed"));
  }
  return found;
}

// ---- the object: what is defined ------------------------------------------------------------------------------------------------------

/** The global operator new and delete that a program may replace (not placement new, and not one of a class of its own). */
const REPLACEABLE = /^_Z(?:(?:nw|na)m|(?:dl|da)Pv[mj]?)(?:St11align_val_t)?(?:RKSt9nothrow_t)?$/;
/** Names the compiler itself gives to a program's main, which is never run. */
const MAIN = new Set(["main", "__original_main", "__main_argc_argv", "__main_void"]);
/** Of the standard library, or a type information or table that the compiler writes. */
const LIBRARY_DATA = /^(?:_ZN?K?St|_ZGVN?S?t|_ZGVSt|_ZGRN?St|_ZT[VISTC]|_ZGVNSt|_ZZ|_ZGVZ|_ZGRZ|\.L|__dso_handle$)/;

/** "_ZL4memo" is "memo", "_ZN3Foo3barE" is "Foo::bar". Anything else is given as it is. */
export function readableName(symbol) {
  const nested = /^_ZN/.test(symbol);
  const start = /^_Z(?:N|L)?/.exec(symbol);
  if (!start) return symbol;
  const parts = [];
  let rest = symbol.slice(start[0].length).replace(/^L/, "");
  for (;;) {
    const m = /^(\d+)/.exec(rest);
    if (!m) break;
    const part = rest.slice(m[1].length, m[1].length + Number(m[1]));
    parts.push(part === "_GLOBAL__N_1" ? null : part);
    rest = rest.slice(m[1].length + Number(m[1]));
    if (!nested) break;
  }
  return parts.filter(Boolean).join("::") || symbol;
}

/** The symbols and the functions that run by themselves, from the "linking" section of a WebAssembly object file. */
export function readObject(bytes) {
  let at = 8;
  const byte = () => {
    if (at >= bytes.length) throw new Error("the object file ends early");
    return bytes[at++];
  };
  const leb = () => {
    let value = 0;
    for (let shift = 0; ; shift += 7) {
      const b = byte();
      value += (b & 0x7f) * 2 ** shift;
      if ((b & 0x80) === 0) return value;
    }
  };
  const string = () => {
    const length = leb();
    const s = new TextDecoder().decode(bytes.subarray(at, at + length));
    at += length;
    return s;
  };
  const segments = [];
  const symbols = [];
  let initFunctions = 0;
  while (at < bytes.length) {
    const id = byte();
    const size = leb();
    const end = at + size;
    if (id === 0 && string() === "linking") {
      leb(); // version
      while (at < end) {
        const type = byte();
        const length = leb();
        const next = at + length;
        if (type === 5) {
          for (let n = leb(); n > 0; n--) {
            segments.push(string());
            leb();
            leb();
          }
        } else if (type === 6) {
          initFunctions = leb();
        } else if (type === 8) {
          for (let n = leb(); n > 0; n--) {
            const kind = byte();
            const flags = leb();
            const undefined_ = (flags & 0x10) !== 0;
            if (kind === 0 || kind === 2 || kind === 4 || kind === 5) {
              leb(); // index
              const name = !undefined_ || (flags & 0x40) !== 0 ? string() : undefined;
              if (kind === 0 && !undefined_) symbols.push({ kind: "function", name, local: (flags & 0x2) !== 0 });
            } else if (kind === 1) {
              const name = string();
              if (!undefined_) {
                const segment = leb();
                leb(); // offset
                leb(); // size
                symbols.push({ kind: "data", name, segment });
              }
            } else if (kind === 3) leb();
            else throw new Error(`unknown symbol kind ${kind}`);
          }
        }
        at = next;
      }
    }
    at = end;
  }
  return { symbols: symbols.map((s) => (s.kind === "data" ? { ...s, segmentName: segments[s.segment] ?? "" } : s)), initFunctions };
}

/** Rules 1 (replaced functions) and 2 (global variables, and code that runs by itself). */
export function objectViolations(bytes) {
  const { symbols, initFunctions } = readObject(bytes);
  const found = [];
  let globals = 0;
  for (const s of symbols) {
    if (s.kind === "function") {
      if (s.local) continue; // "static" or in an unnamed namespace: it takes nobody's place
      if (REPLACEABLE.test(s.name)) found.push(violation(1, `you define the global operator ${/^_Z(nw|na)/.test(s.name) ? "new" : "delete"}`));
      else if (RUNTIME_SYMBOLS.has(s.name) && !MAIN.has(s.name)) found.push(violation(1, `you define "${s.name}", which is a function of the C or runtime library`));
    } else if (/^\.(?:bss|data|tbss|tdata)\b/.test(s.segmentName) && !LIBRARY_DATA.test(s.name)) {
      globals++;
      found.push(violation(2, `global variable "${readableName(s.name)}"`));
    }
  }
  if (initFunctions > 0 && globals === 0) found.push(violation(2, "code of the program runs by itself, outside its solver function (a global object or a constructor attribute)"));
  return found;
}

// ---- the debug information: what was written where -----------------------------------------------------------------------------------

/**
 * Rule 1 for the standard library: a function or variable of std whose definition the debug information places in the program's own file.
 * The library's templates that a program uses are placed in the library's headers, whatever types they are used with. `lines` are those of
 * `llvm-dwarfdump --debug-info`.
 */
export async function debugViolations(lines, sourcePath) {
  const dies = new Map();
  let die;
  for await (const line of lines) {
    const head = /^(0x[0-9a-f]+):\s+(DW_TAG_\w+|NULL)/.exec(line);
    if (head) {
      die = { tag: head[2] };
      if (head[2] === "DW_TAG_subprogram" || head[2] === "DW_TAG_variable") dies.set(head[1], die);
      continue;
    }
    const attr = /^\s+(DW_AT_(?:linkage_name|decl_file|specification|abstract_origin|low_pc|location))\s+\((.*)/.exec(line);
    if (attr && die) die[attr[1]] = attr[2].replace(/\)\s*$/, "");
  }
  const text = (v) => v?.replace(/^"|"$/g, "");
  const mine = (file) => file === sourcePath || file?.endsWith(`/${sourcePath.replace(/^.*\//, "")}`);
  const found = new Set();
  for (const d of dies.values()) {
    if (d.DW_AT_low_pc === undefined && d.DW_AT_location === undefined) continue; // declared only: nothing of it is in the program
    const ref = d.DW_AT_specification ?? d.DW_AT_abstract_origin;
    const base = ref ? dies.get(ref.split(" ")[0]) : undefined;
    const linkage = text(d.DW_AT_linkage_name ?? base?.DW_AT_linkage_name);
    if (!linkage || !/^_Z(?:Z)?(?:NK?)?St/.test(linkage) || /^_Z(?:Z)?(?:NK?)?St(?:3__2)?4hash/.test(linkage)) continue;
    if (mine(text(d.DW_AT_decl_file ?? base?.DW_AT_decl_file))) {
      const member = /^_ZZ?(?:NK?)?St(?:3__2)?(\d+)/.exec(linkage);
      const name = member ? linkage.slice(member[0].length, member[0].length + Number(member[1])) : "";
      found.add(`you define something of the standard library${name ? ` (std::${name})` : ""}`);
    }
  }
  return [...found].map((t) => violation(1, t));
}

// ---- together --------------------------------------------------------------------------------------------------------------------------

/**
 * Every violation of a program that compiled. `source` is its text, `objectFile` what the check compile wrote, `debugFile` the output of
 * `llvm-dwarfdump --debug-info` on it, and `sourcePath` the file name the compiler was given.
 */
export async function checkRules({ source, objectFile, debugFile, sourcePath }) {
  const found = [...sourceViolations(source), ...objectViolations(readFileSync(objectFile))];
  if (statSync(debugFile).size > MAX_DEBUG_BYTES) found.push(violation(1, "the program is too large to check"));
  else found.push(...(await debugViolations(createInterface({ input: createReadStream(debugFile), crlfDelay: Infinity }), sourcePath)));
  const seen = new Map(found.map((v) => [`${v.rule}:${v.text}`, v]));
  return [...seen.values()].sort((a, b) => a.rule - b.rule);
}

const MAX_LISTED = 12;

/** What a participant is told. */
export function describeViolations(violations) {
  const lines = violations.slice(0, MAX_LISTED).map((v) => `  Rule ${v.rule}: ${v.text}`);
  if (violations.length > MAX_LISTED) lines.push(`  ... and ${violations.length - MAX_LISTED} more`);
  return `The program breaks the rules of the competition, so it was not accepted:\n${lines.join("\n")}`;
}
