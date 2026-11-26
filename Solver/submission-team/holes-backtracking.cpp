

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

    if (n <= 0 || grid.size() != static_cast<size_t>(n)) {
        return solutions;
    }

    for (const string& row : grid) {
        if (row.size() != static_cast<size_t>(n)) {
            return solutions;
        }
    }

    vector<uint32_t> allowed(n, 0);

    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            if (grid[r][c] == '.') {
                allowed[r] |= (1u << c);
            }
        }
    }

    const uint32_t allColumns = (1u << n) - 1u;

    vector<int> current(n, -1);

    auto backtrack = [&](auto&& self,
                         int row,
                         uint32_t columns,
                         uint32_t diagLeft,
                         uint32_t diagRight) -> void {

        if (row == n) {
            solutions.push_back(current);
            return;
        }

        uint32_t available =
            allowed[row] &
            ~(columns | diagLeft | diagRight) &
            allColumns;

        while (available != 0) {

            uint32_t bit = available & (~available + 1u);

            available &= (available - 1u);

            int col = 0;
            while ((bit & (1u << col)) == 0) {
                ++col;
            }

            current[row] = col;

            self(
                self,
                row + 1,
                columns | bit,
                ((diagLeft | bit) << 1) & allColumns,
                (diagRight | bit) >> 1
            );

            current[row] = -1;
        }
    };

    backtrack(backtrack, 0, 0, 0, 0);

    return solutions;
}
