#include "nqueens.h"

static void emit(int n, const unsigned* bits,
                 std::vector<std::vector<int>>& out) {
  std::vector<int> a(n);
  for (int i = 0; i < n; i++) {
    int c = 0;
    while (((bits[i] >> c) & 1u) == 0) c++;
    a[i] = c;
  }
  out.push_back(std::move(a));
}

static void search(int n, int row, unsigned full, unsigned cols, unsigned d1,
                   unsigned d2, unsigned* bits, const unsigned* forb,
                   const int* fixedCol, std::vector<std::vector<int>>& out) {
  if (row == n) {
    emit(n, bits, out);
    return;
  }
  if (fixedCol[row] >= 0) {
    unsigned b = 1u << fixedCol[row];
    bits[row] = b;
    search(n, row + 1, full, cols | b, (d1 | b) << 1, (d2 | b) >> 1, bits,
           forb, fixedCol, out);
    return;
  }
  unsigned avail = ~(cols | d1 | d2 | forb[row]) & full;
  while (avail) {
    unsigned b = avail & -avail;
    avail ^= b;
    bits[row] = b;
    search(n, row + 1, full, cols | b, (d1 | b) << 1, (d2 | b) >> 1, bits,
           forb, fixedCol, out);
  }
}

std::vector<std::vector<int>> solvePreplaced(
    int n, const std::vector<std::pair<int, int>>& preplaced) {
  std::vector<std::vector<int>> found;
  if (n <= 0 || n > 31) return found;

  int fixedCol[32];
  unsigned forb[32];
  unsigned bits[32];
  for (int i = 0; i < n; i++) {
    fixedCol[i] = -1;
    forb[i] = 0;
    bits[i] = 0;
  }


  for (size_t k = 0; k < preplaced.size(); k++) {
    int r = preplaced[k].first, c = preplaced[k].second;
    if (r < 0 || r >= n || c < 0 || c >= n) return found;
    if (fixedCol[r] >= 0) return found;
    fixedCol[r] = c;
  }

  for (int r1 = 0; r1 < n; r1++) {
    if (fixedCol[r1] < 0) continue;
    for (int r2 = r1 + 1; r2 < n; r2++) {
      if (fixedCol[r2] < 0) continue;
      int dc = fixedCol[r2] - fixedCol[r1];
      if (dc == 0 || dc == r2 - r1 || dc == r1 - r2) return found;
    }
  }

  for (int pr = 0; pr < n; pr++) {
    if (fixedCol[pr] < 0) continue;
    int pc = fixedCol[pr];
    for (int r = 0; r < n; r++) {
      if (r == pr) continue;
      int d = r - pr;
      if (d < 0) d = -d;
      forb[r] |= 1u << pc;
      if (pc + d < n) forb[r] |= 1u << (pc + d);
      if (pc - d >= 0) forb[r] |= 1u << (pc - d);
    }
  }

  unsigned full = (1u << n) - 1;
  search(n, 0, full, 0, 0, 0, bits, forb, fixedCol, found);
  return found;
}