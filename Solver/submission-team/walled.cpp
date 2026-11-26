#include "nqueens.h"
#include <algorithm>
#include <cstdint>

namespace {

struct Cell {
  int r;
  int c;
  int deg;
  uint64_t atk[3];
};

struct WalledSolver {
  int n;
  int total;
  int bestCount;
  std::vector<Cell> cells;
  std::vector<std::vector<std::pair<int, int>>> bestSolutions;
  std::vector<std::pair<int, int>> current;

  int upperBound(int start, uint64_t m0, uint64_t m1, uint64_t m2) const {
    int ub = 0;
    uint64_t b0 = m0, b1 = m1, b2 = m2;
    for (int i = start; i < total; i++) {
      int w = i >> 6;
      uint64_t bit = 1ULL << (i & 63);
      uint64_t mask = (w == 0) ? b0 : ((w == 1) ? b1 : b2);
      if ((mask & bit) == 0ULL) {
        ub++;
        b0 |= cells[i].atk[0];
        b1 |= cells[i].atk[1];
        b2 |= cells[i].atk[2];
      }
    }
    return ub;
  }

  void dfs(int idx, uint64_t m0, uint64_t m1, uint64_t m2) {
    int curSize = static_cast<int>(current.size());
    if (curSize + (total - idx) < bestCount) return;
    if (curSize + upperBound(idx, m0, m1, m2) < bestCount) return;

    if (idx == total) {
      if (curSize > bestCount) {
        bestCount = curSize;
        bestSolutions.clear();
        bestSolutions.push_back(current);
      } else if (curSize == bestCount && bestCount > 0) {
        bestSolutions.push_back(current);
      }
      return;
    }

    int w = idx >> 6;
    uint64_t bit = 1ULL << (idx & 63);
    uint64_t mask = (w == 0) ? m0 : ((w == 1) ? m1 : m2);

    if ((mask & bit) == 0ULL) {
      current.push_back({cells[idx].r, cells[idx].c});
      dfs(idx + 1, m0 | cells[idx].atk[0], m1 | cells[idx].atk[1],
          m2 | cells[idx].atk[2]);
      current.pop_back();
    }

    dfs(idx + 1, m0, m1, m2);
  }
};

}  

std::vector<std::vector<std::pair<int, int>>> solveWalled(
    int n, const std::vector<std::string>& grid) {
  if (n <= 0 || n > 12) return {};

  struct RawCell {
    int r, c;
    int deg;
    uint64_t atk[3];
  };

  std::vector<RawCell> raw;
  int idGrid[12][12];
  for (int r = 0; r < n; r++) {
    for (int c = 0; c < n; c++) {
      if (grid[r][c] != 'W') {
        idGrid[r][c] = static_cast<int>(raw.size());
        raw.push_back({r, c, 0, {0ULL, 0ULL, 0ULL}});
      } else {
        idGrid[r][c] = -1;
      }
    }
  }

  int m = static_cast<int>(raw.size());
  if (m == 0) return {};

  const int dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  const int dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

  for (int i = 0; i < m; i++) {
    int r0 = raw[i].r;
    int c0 = raw[i].c;
    raw[i].atk[i >> 6] |= (1ULL << (i & 63));
    int reach = 1;

    for (int d = 0; d < 8; d++) {
      int nr = r0 + dr[d];
      int nc = c0 + dc[d];
      while (nr >= 0 && nr < n && nc >= 0 && nc < n) {
        if (grid[nr][nc] == 'W') break;
        int tid = idGrid[nr][nc];
        uint64_t tbit = 1ULL << (tid & 63);
        if ((raw[i].atk[tid >> 6] & tbit) == 0ULL) {
          raw[i].atk[tid >> 6] |= tbit;
          reach++;
        }
        nr += dr[d];
        nc += dc[d];
      }
    }
    raw[i].deg = reach;
  }

  std::vector<int> order(m);
  for (int i = 0; i < m; i++) order[i] = i;
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    if (raw[a].deg != raw[b].deg) return raw[a].deg > raw[b].deg;
    if (raw[a].r != raw[b].r) return raw[a].r < raw[b].r;
    return raw[a].c < raw[b].c;
  });

  std::vector<int> invOrder(m);
  WalledSolver solver;
  solver.n = n;
  solver.total = m;
  solver.bestCount = 0;
  solver.cells.resize(m);

  for (int newIdx = 0; newIdx < m; newIdx++) {
    int oldIdx = order[newIdx];
    invOrder[oldIdx] = newIdx;
    solver.cells[newIdx].r = raw[oldIdx].r;
    solver.cells[newIdx].c = raw[oldIdx].c;
    solver.cells[newIdx].deg = raw[oldIdx].deg;
    solver.cells[newIdx].atk[0] = 0ULL;
    solver.cells[newIdx].atk[1] = 0ULL;
    solver.cells[newIdx].atk[2] = 0ULL;
  }

  for (int newIdx = 0; newIdx < m; newIdx++) {
    int oldIdx = order[newIdx];
    for (int oldTarget = 0; oldTarget < m; oldTarget++) {
      if ((raw[oldIdx].atk[oldTarget >> 6] & (1ULL << (oldTarget & 63))) != 0ULL) {
        int newTarget = invOrder[oldTarget];
        solver.cells[newIdx].atk[newTarget >> 6] |= (1ULL << (newTarget & 63));
      }
    }
  }

  solver.dfs(0, 0ULL, 0ULL, 0ULL);
  return solver.bestSolutions;
}