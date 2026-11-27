#include "nqueens.h"

template <bool STORE>
static long run(int n, unsigned b0, bool mirror,
                std::vector<std::vector<int>>* out) {
  unsigned full = (1u << n) - 1;
  unsigned C[32], D1[32], D2[32], A[32], B[32];
  long count = 0;
  C[1] = b0;
  D1[1] = b0 << 1;
  D2[1] = b0 >> 1;
  A[1] = ~(C[1] | D1[1] | D2[1]) & full;
  B[0] = b0;
  int row = 1;
  int pre = n - 2;
  while (row >= 1) {
    unsigned a = A[row];
    if (!a) {
      row--;
      continue;
    }
    unsigned b = a & -a;
    A[row] = a ^ b;
    B[row] = b;
    unsigned nc = C[row] | b, nd1 = (D1[row] | b) << 1, nd2 = (D2[row] | b) >> 1;
    if (row == pre) {
      unsigned la = ~(nc | nd1 | nd2) & full;
      if (la) {
        count += mirror ? 2 : 1;
        if (STORE) {
          B[n - 1] = la;
          std::vector<int> v(n);
          for (int i = 0; i < n; i++) {
            int c = 0;
            while (((B[i] >> c) & 1u) == 0) c++;
            v[i] = c;
          }
          if (mirror) {
            std::vector<int> m(n);
            for (int i = 0; i < n; i++) m[i] = n - 1 - v[i];
            out->push_back(std::move(v));
            out->push_back(std::move(m));
          } else {
            out->push_back(std::move(v));
          }
        }
      }
      continue;
    }
    row++;
    C[row] = nc;
    D1[row] = nd1;
    D2[row] = nd2;
    A[row] = ~(nc | nd1 | nd2) & full;
  }
  return count;
}

std::vector<std::vector<int>> solveNQueens(int n) {
  std::vector<std::vector<int>> found;
  if (n == 1) {
    found.push_back(std::vector<int>(1, 0));
    return found;
  }
  if (n < 4 || n > 31) return found;
  int half = n / 2;
  for (int c = 0; c < half; c++) run<true>(n, 1u << c, true, &found);
  if (n % 2 == 1) run<true>(n, 1u << half, false, &found);
  return found;
}