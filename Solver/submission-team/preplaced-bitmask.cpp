#include "nqueens.h"
#include <cstdint>
#include <vector>

std::vector<std::vector<int>> solvePreplaced(
    int n,
    const std::vector<std::pair<int, int>>& preplaced
) {
    std::vector<std::vector<int>> solutions;

    if (n <= 0 || n > 12) {
        return solutions;
    }

    const uint32_t full = (1u << n) - 1u;

    int fixedCol[12];
    uint32_t forbidden[12] = {};

    for (int r = 0; r < n; ++r) {
        fixedCol[r] = -1;
    }

    for (const auto& p : preplaced) {
        int r = p.first;
        int c = p.second;

        if (r < 0 || r >= n || c < 0 || c >= n) {
            return solutions;
        }

        if (fixedCol[r] != -1) {
            return solutions;
        }

        fixedCol[r] = c;
    }

    for (int r1 = 0; r1 < n; ++r1) {
        if (fixedCol[r1] == -1) {
            continue;
        }

        for (int r2 = r1 + 1; r2 < n; ++r2) {
            if (fixedCol[r2] == -1) {
                continue;
            }

            int diff = fixedCol[r2] - fixedCol[r1];

            if (diff == 0 || diff == r2 - r1 || diff == r1 - r2) {
                return solutions;
            }
        }
    }

    for (int r = 0; r < n; ++r) {
        if (fixedCol[r] == -1) {
            continue;
        }

        int c = fixedCol[r];

        for (int row = 0; row < n; ++row) {
            if (row == r) {
                continue;
            }

            int d = row > r ? row - r : r - row;

            forbidden[row] |= (1u << c);

            if (c + d < n) {
                forbidden[row] |= (1u << (c + d));
            }

            if (c - d >= 0) {
                forbidden[row] |= (1u << (c - d));
            }
        }
    }

    std::vector<int> placement(n);

    auto backtrack = [&](auto&& self,
                         int row,
                         uint32_t cols,
                         uint32_t diagLeft,
                         uint32_t diagRight) -> void {

        if (row == n) {
            solutions.emplace_back(placement);
            return;
        }

        uint32_t blocked = cols | diagLeft | diagRight | forbidden[row];

        if (fixedCol[row] != -1) {
            uint32_t bit = 1u << fixedCol[row];

            if (blocked & bit) {
                return;
            }

            placement[row] = fixedCol[row];

            self(
                self,
                row + 1,
                cols | bit,
                ((diagLeft | bit) << 1) & full,
                (diagRight | bit) >> 1
            );

            return;
        }

        uint32_t available = full & ~blocked;

        while (available) {
            uint32_t bit = available & (~available + 1u);
            available ^= bit;

            int col = __builtin_ctz(bit);
            placement[row] = col;

            self(
                self,
                row + 1,
                cols | bit,
                ((diagLeft | bit) << 1) & full,
                (diagRight | bit) >> 1
            );
        }
    };

    backtrack(backtrack, 0, 0u, 0u, 0u);

    return solutions;
}