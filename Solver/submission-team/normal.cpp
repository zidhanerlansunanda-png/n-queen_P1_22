#include "nqueens.h"

static void emit(int n, const unsigned* bits,
                 std::vector<std::vector<int>>& out, bool mirror) {
  std::vector<int> a(n);
  for (int i = 0; i < n; i++) {
    int c = 0;
    while (((bits[i] >> c) & 1u) == 0) c++;
    a[i] = c;
  }
  if (mirror) {
    std::vector<int> m(n);
    for (int i = 0; i < n; i++) m[i] = n - 1 - a[i];
    out.push_back(std::move(a));
    out.push_back(std::move(m));
  } else {
    out.push_back(std::move(a));
  }
}

static void search(int n, int row, unsigned full, unsigned cols, unsigned d1,
                   unsigned d2, unsigned* bits,
                   std::vector<std::vector<int>>& out, bool mirror) {
  unsigned avail = ~(cols | d1 | d2) & full;
  if (row == n - 1) {
    if (avail) {
      bits[row] = avail;
      emit(n, bits, out, mirror);
    }
    return;
  }
  while (avail) {
    unsigned b = avail & -avail;
    avail ^= b;
    bits[row] = b;
    search(n, row + 1, full, cols | b, (d1 | b) << 1, (d2 | b) >> 1, bits,
           out, mirror);
  }
}

std::vector<std::vector<int>> solveNQueens(int n) {
  std::vector<std::vector<int>> found;
  if (n <= 0) return found;
  if (n == 1) {
    found.push_back(std::vector<int>(1, 0));
    return found;
  }
  unsigned bits[32];
  unsigned full = (1u << n) - 1;
  int half = n / 2;
  for (int c = 0; c < half; c++) {
    unsigned b = 1u << c;
    bits[0] = b;
    search(n, 1, full, b, b << 1, b >> 1, bits, found, true);
  }
  if (n % 2 == 1) {
    unsigned b = 1u << half;
    bits[0] = b;
    search(n, 1, full, b, b << 1, b >> 1, bits, found, false);
  }
  return found;
}