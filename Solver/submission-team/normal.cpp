#include "nqueens.h"

static void search(int n, int row, unsigned full, unsigned cols, unsigned d1,
                   unsigned d2, std::vector<int>& cur,
                   std::vector<std::vector<int>>& out, bool mirror) {
  if (row == n) {
    out.push_back(cur);
    if (mirror) {
      std::vector<int> m(n);
      for (int i = 0; i < n; i++) m[i] = n - 1 - cur[i];
      out.push_back(std::move(m));
    }
    return;
  }
  unsigned avail = ~(cols | d1 | d2) & full;
  while (avail) {
    unsigned b = avail & -avail;
    avail -= b;
    int c = 0;
    while (((b >> c) & 1u) == 0) c++;
    cur[row] = c;
    search(n, row + 1, full, cols | b, (d1 | b) << 1, (d2 | b) >> 1, cur, out, mirror);
  }
}

std::vector<std::vector<int>> solveNQueens(int n) {
  std::vector<std::vector<int>> found;
  std::vector<int> cur(n);
  unsigned full = (1u << n) - 1;
  int half = n / 2;
  for (int c = 0; c < half; c++) {
    unsigned b = 1u << c;
    cur[0] = c;
    search(n, 1, full, b, b << 1, b >> 1, cur, found, true);
  }
  if (n % 2 == 1) {
    unsigned b = 1u << half;
    cur[0] = half;
    search(n, 1, full, b, b << 1, b >> 1, cur, found, false);
  }
  return found;
}