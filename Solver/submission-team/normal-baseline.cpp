#include "nqueens.h"

static void place(int n, int row, unsigned cols, unsigned d1, unsigned d2,
                  std::vector<int>& cur, std::vector<std::vector<int>>& out) {
  if (row == n) { out.push_back(cur); return; }
  unsigned avail = ~(cols | d1 | d2) & ((1u << n) - 1);
  while (avail) {
    unsigned b = avail & -avail;
    avail -= b;
    int c = 0;
    while (((b >> c) & 1u) == 0) c++;
    cur[row] = c;
    place(n, row + 1, cols | b, (d1 | b) << 1, (d2 | b) >> 1, cur, out);
  }
}

std::vector<std::vector<int>> solveNQueens(int n) {
  std::vector<std::vector<int>> found;
  std::vector<int> cur(n);
  place(n, 0, 0, 0, 0, cur, found);
  return found;
}