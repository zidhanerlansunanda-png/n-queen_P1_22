
#include "nqueens.h"

#include <cstdint>
#include <string>
#include <vector>

std::vector<std::vector<int>> solveHoles(
    int n,
    const std::vector<std::string>& grid
) {
    std::vector<std::vector<int>> solutions;

    if (n <= 0 || n > 12) {
        return solutions;
    }

    const uint32_t fullMask = (1u << n) - 1u;
    uint32_t allowed[12] = {};

    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            if (grid[r][c] != 'H') {
                allowed[r] |= (1u << c);
            }
        }
    }

    std::vector<int> placement(n);

    auto backtrack = [&](auto&& self,
                         int row,
                         uint32_t columns,
                         uint32_t diagLeft,
                         uint32_t diagRight) -> void {

        if (row == n) {
            solutions.emplace_back(placement);
            return;
        }

        uint32_t available =
            allowed[row] &
            ~(columns | diagLeft | diagRight) &
            fullMask;

        while (available != 0) {
            uint32_t bit = available & (~available + 1u);
            available ^= bit;

            int col = __builtin_ctz(bit);
            placement[row] = col;

            self(
                self,
                row + 1,
                columns | bit,
                ((diagLeft | bit) << 1) & fullMask,
                (diagRight | bit) >> 1
            );
        }
    };

    backtrack(backtrack, 0, 0u, 0u, 0u);

    return solutions;
}
