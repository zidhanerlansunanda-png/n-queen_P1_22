#include "nqueens.h"
#include <cstdint>
#include <vector>

using namespace std;

vector<vector<int>> solveNQueens(int n) {
    vector<vector<int>> solutions;

    if (n <= 0 || n > 12) {
        return solutions;
    }

    const int counts[13] = {
        0, 1, 0, 0, 2, 10, 4,
        40, 92, 352, 724, 2680, 14200
    };

    solutions.reserve(counts[n]);

    if (n == 1) {
        solutions.push_back({0});
        return solutions;
    }

    const uint32_t full = (1u << n) - 1u;
    const int half = n / 2;

    int placement[12];

    auto dfs = [&](auto&& self,
                   int row,
                   uint32_t cols,
                   uint32_t left,
                   uint32_t right,
                   bool mirror) -> void {

        if (row == n) {
            solutions.emplace_back(placement, placement + n);

            if (mirror) {
                solutions.emplace_back(n);

                for (int r = 0; r < n; ++r) {
                    solutions.back()[r] = n - 1 - placement[r];
                }
            }

            return;
        }

        uint32_t available =
            full & ~(cols | left | right);

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
                (right | bit) >> 1,
                mirror
            );
        }
    };

    for (int col = 0; col < half; ++col) {
        uint32_t bit = 1u << col;

        placement[0] = col;

        dfs(
            dfs,
            1,
            bit,
            (bit << 1) & full,
            bit >> 1,
            true
        );
    }

    if (n % 2 == 1) {
        uint32_t bit = 1u << half;

        placement[0] = half;

        dfs(
            dfs,
            1,
            bit,
            (bit << 1) & full,
            bit >> 1,
            false
        );
    }

    return solutions;
}