#include "nqueens.h"

namespace {

struct Cell {
  int r;
  int c;
  unsigned long long atk[3];
};

struct WalledSearch {
  int n;
  int bestCount;
  std::vector<Cell> cells;
  std::vector<std::vector<std::pair<int, int>>> bestSolutions;
  std::vector<std::pair<int, int>> current;

  void dfs(int idx, unsigned long long m0, unsigned long long m1,
           unsigned long long m2) {
    int curSize = static_cast<int>(current.size());
    int rem = static_cast<int>(cells.size()) - idx;

    if (curSize + rem < bestCount) return;

    if (idx == static_cast<int>(cells.size())) {
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
    unsigned long long bit = 1ULL << (idx & 63);
    unsigned long long mask = (w == 0) ? m0 : ((w == 1) ? m1 : m2);

    if ((mask & bit) == 0ULL) {
      current.push_back({cells[idx].r, cells[idx].c});
      dfs(idx + 1, m0 | cells[idx].atk[0], m1 | cells[idx].atk[1],
          m2 | cells[idx].atk[2]);
      current.pop_back();
    }

    dfs(idx + 1, m0, m1, m2);
  }
};

}  // namespace

std::vector<std::vector<std::pair<int, int>>> solveWalled(
    int n, const std::vector<std::string>& grid) {
  if (n <= 0 || n > 12) return {};

  int idGrid[12][12];
  WalledSearch search;
  search.n = n;
  search.bestCount = 0;

  for (int r = 0; r < n; r++) {
    for (int c = 0; c < n; c++) {
      if (grid[r][c] != 'W') {
        idGrid[r][c] = static_cast<int>(search.cells.size());
        search.cells.push_back({r, c, {0ULL, 0ULL, 0ULL}});
      } else {
        idGrid[r][c] = -1;
      }
    }
  }

  int totalCells = static_cast<int>(search.cells.size());
  if (totalCells == 0) return {};

  const int dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  const int dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

  for (int i = 0; i < totalCells; i++) {
    int r0 = search.cells[i].r;
    int c0 = search.cells[i].c;
    search.cells[i].atk[i >> 6] |= (1ULL << (i & 63));

    for (int d = 0; d < 8; d++) {
      int nr = r0 + dr[d];
      int nc = c0 + dc[d];
      while (nr >= 0 && nr < n && nc >= 0 && nc < n) {
        if (grid[nr][nc] == 'W') break;
        int targetId = idGrid[nr][nc];
        search.cells[i].atk[targetId >> 6] |= (1ULL << (targetId & 63));
        nr += dr[d];
        nc += dc[d];
      }
    }
  }

  search.dfs(0, 0ULL, 0ULL, 0ULL);
  return search.bestSolutions;
}