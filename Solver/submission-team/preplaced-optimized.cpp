
#include "nqueens.h"

static void emit(int n, const unsigned* bits,
                 std::vector<std::vector<int>>& out) {
  out.emplace_back(n);

  for (int i = 0; i < n; i++) {
    unsigned b = bits[i];
    int c = 0;

    while ((b & 1u) == 0u) {
      b >>= 1;
      c++;
    }

    out.back()[i] = c;
  }
}

static void search(int n, int row, unsigned full,
                   unsigned cols, unsigned d1, unsigned d2,
                   unsigned* bits, const unsigned* forb,
                   const int* fixedCol,
                   std::vector<std::vector<int>>& out) {
  if (row == n - 1) {
    if (fixedCol[row] >= 0) {
      unsigned b = 1u << fixedCol[row];

      if ((b & (cols | d1 | d2)) == 0u) {
        bits[row] = b;
        emit(n, bits, out);
      }
    } else {
      unsigned avail = ~(cols | d1 | d2 | forb[row]) & full;

      while (avail) {
        unsigned b = avail & -avail;
        avail ^= b;

        bits[row] = b;
        emit(n, bits, out);
      }
    }
    return;
  }

  if (fixedCol[row] >= 0) {
    unsigned b = 1u << fixedCol[row];

    if (b & (cols | d1 | d2)) return;

    bits[row] = b;

    search(n, row + 1, full,
           cols | b, (d1 | b) << 1, (d2 | b) >> 1,
           bits, forb, fixedCol, out);
    return;
  }

  unsigned avail = ~(cols | d1 | d2 | forb[row]) & full;

  while (avail) {
    unsigned b = avail & -avail;
    avail ^= b;

    bits[row] = b;

    search(n, row + 1, full,
           cols | b, (d1 | b) << 1, (d2 | b) >> 1,
           bits, forb, fixedCol, out);
  }
}

std::vector<std::vector<int>> solvePreplaced(
    int n, const std::vector<std::pair<int, int>>& preplaced) {
  std::vector<std::vector<int>> found;

  if (n <= 0 || n > 31) return found;

  int fixedCol[32];
  unsigned forb[32] = {};
  unsigned bits[32];

  bool diag1[64] = {};
  bool diag2[64] = {};

  unsigned fixedCols = 0;

  for (int i = 0; i < n; i++) {
    fixedCol[i] = -1;
  }

  for (size_t k = 0; k < preplaced.size(); k++) {
    int r = preplaced[k].first;
    int c = preplaced[k].second;

    if (r < 0 || r >= n || c < 0 || c >= n) return found;
    if (fixedCol[r] >= 0) return found;

    unsigned b = 1u << c;

    int d1 = r - c + n - 1;
    int d2 = r + c;

    if ((fixedCols & b) || diag1[d1] || diag2[d2]) {
      return found;
    }

    fixedCol[r] = c;
    fixedCols |= b;
    diag1[d1] = true;
    diag2[d2] = true;
  }

  for (int pr = 0; pr < n; pr++) {
    if (fixedCol[pr] < 0) continue;

    int pc = fixedCol[pr];

    for (int r = 0; r < n; r++) {
      if (r == pr) continue;

      int d = r - pr;
      if (d < 0) d = -d;

      unsigned mask = 1u << pc;

      if (pc + d < n) {
        mask |= 1u << (pc + d);
      }

      if (pc - d >= 0) {
        mask |= 1u << (pc - d);
      }

      forb[r] |= mask;
    }
  }

  unsigned full = (1u << n) - 1u;

  search(n, 0, full, 0, 0, 0,
         bits, forb, fixedCol, found);

  return found;
}
