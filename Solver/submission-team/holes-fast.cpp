#include "nqueens.h"
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

vector<vector<int>> solveHoles(
    int n,
    const vector<string>& grid
) {
    vector<vector<int>> solutions;

    if (n <= 0 || n > 12 ||
        grid.size() != static_cast<size_t>(n)) {
        return solutions;
    }

    const uint32_t full = (1u << n) - 1u;
    const int counts[13] = {
        0, 1, 0, 0, 2, 10, 4,
        40, 92, 352, 724, 2680, 14200
    };

    solutions.reserve(counts[n]);

    uint32_t allowed[12] = {};

    for (int r = 0; r < n; ++r) {
        if (grid[r].size() != static_cast<size_t>(n)) {
            return {};
        }

        for (int c = 0; c < n; ++c) {
            if (grid[r][c] == '.') {
                allowed[r] |= (1u << c);
            }
        }

        if (allowed[r] == 0) {
            return solutions;
        }
    }

    int placement[12];

    auto dfs = [&](auto&& self,
                   int row,
                   uint32_t cols,
                   uint32_t left,
                   uint32_t right) -> void {

        if (row == n) {
            solutions.emplace_back(placement, placement + n);
            return;
        }

        uint32_t available =
            allowed[row] & ~(cols | left | right) & full;

        while (available) {
            uint32_t bit = available & -available;
            available &= available - 1u;

            int col = __builtin_ctz(bit);
            placement[row] = col;

            self(
                self,
                row + 1,
                cols | bit,
                ((left | bit) << 1) & full,
                (right | bit) >> 1
            );
        }
    };

    dfs(dfs, 0, 0u, 0u, 0u);

    return solutions;
}
