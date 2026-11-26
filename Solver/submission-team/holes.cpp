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
                   unsigned d2, unsigned* bits, const unsigned* hole,
                   std::vector<std::vector<int>>& out) {
  unsigned avail = ~(cols | d1 | d2 | hole[row]) & full;
  if (row == n - 1) {
    if (avail) {
      bits[row] = avail;
      emit(n, bits, out);
    }
    return;
  }
  while (avail) {
    unsigned b = avail & -avail;
    avail ^= b;
    bits[row] = b;
    search(n, row + 1, full, cols | b, (d1 | b) << 1, (d2 | b) >> 1, bits,
           hole, out);
  }
}

std::vector<std::vector<int>> solveHoles(int n,
                                         const std::vector<std::string>& grid) {
  std::vector<std::vector<int>> found;
  if (n <= 0 || n > 31) return found;

  unsigned hole[32];
  unsigned bits[32];
  for (int r = 0; r < n; r++) {
    hole[r] = 0;
    bits[r] = 0;
    for (int c = 0; c < n; c++) {
      if (grid[r][c] == 'H') hole[r] |= 1u << c;
    }
  }

  unsigned full = (1u << n) - 1;
  for (int r = 0; r < n; r++) {
    if ((hole[r] & full) == full) return found;
  }
  search(n, 0, full, 0, 0, 0, bits, hole, found);
  return found;
}