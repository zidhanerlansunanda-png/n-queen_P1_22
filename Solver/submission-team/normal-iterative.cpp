#include "nqueens.h"

#include <cstdint>
#include <vector>

std::vector<std::vector<int>> solveNQueens(int n) {
    std::vector<std::vector<int>> solutions;

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

    uint32_t cols[12];
    uint32_t diagLeft[12];
    uint32_t diagRight[12];
    uint32_t available[12];

    for (int firstCol = 0; firstCol < (n + 1) / 2; ++firstCol) {
        const bool mirror = firstCol < half;
        const uint32_t firstBit = 1u << firstCol;

        placement[0] = firstCol;

        cols[1] = firstBit;
        diagLeft[1] = (firstBit << 1) & full;
        diagRight[1] = firstBit >> 1;

        available[1] =
            full & ~(cols[1] | diagLeft[1] | diagRight[1]);

        int row = 1;

        while (row >= 1) {
            if (available[row] == 0) {
                --row;
                continue;
            }

            const uint32_t bit =
                available[row] & (~available[row] + 1u);

            available[row] ^= bit;
            placement[row] = __builtin_ctz(bit);

            if (row == n - 1) {
                solutions.emplace_back(n);

                for (int r = 0; r < n; ++r) {
                    solutions.back()[r] = placement[r];
                }

                if (mirror) {
                    solutions.emplace_back(n);

                    for (int r = 0; r < n; ++r) {
                        solutions.back()[r] = n - 1 - placement[r];
                    }
                }
            } else {
                const uint32_t nextCols = cols[row] | bit;
                const uint32_t nextLeft =
                    ((diagLeft[row] | bit) << 1) & full;
                const uint32_t nextRight =
                    (diagRight[row] | bit) >> 1;

                ++row;

                cols[row] = nextCols;
                diagLeft[row] = nextLeft;
                diagRight[row] = nextRight;

                available[row] =
                    full & ~(nextCols | nextLeft | nextRight);
            }
        }
    }

    return solutions;
}
