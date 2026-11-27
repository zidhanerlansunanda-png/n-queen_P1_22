
#include "nqueens.h"
#include <cstdint>
#include <string>
#include <vector>

std::vector<std::vector<int>> solveHoles(
    int n,
    const std::vector<std::string>& grid
) {
    std::vector<std::vector<int>> solutions;

    if (n <= 0 || n > 12 ||
        grid.size() != static_cast<size_t>(n)) {
        return solutions;
    }

    const uint32_t full = (1u << n) - 1u;

    uint32_t allowed[12] = {};

    for (int r = 0; r < n; ++r) {
        if (grid[r].size() != static_cast<size_t>(n)) {
            return solutions;
        }

        for (int c = 0; c < n; ++c) {
            if (grid[r][c] != 'H') {
                allowed[r] |= (1u << c);
            }
        }

        if (allowed[r] == 0) {
            return solutions;
        }
    }

    int placement[12];
    bool assigned[12] = {};

    auto search = [&](auto&& self,
                      int depth,
                      uint32_t cols,
                      uint32_t diag1,
                      uint32_t diag2) -> void {

        if (depth == n) {
            solutions.emplace_back(placement, placement + n);
            return;
        }

        int bestRow = -1;
        uint32_t bestMask = 0;
        int minCount = n + 1;

        for (int r = 0; r < n; ++r) {
            if (assigned[r]) {
                continue;
            }

            uint32_t candidates = allowed[r] & ~cols & full;
            uint32_t bits = candidates;
            uint32_t valid = 0;

            while (bits) {
                uint32_t bit = bits & -bits;
                bits ^= bit;

                int c = __builtin_ctz(bit);

                int d1 = r - c + n - 1;
                int d2 = r + c;

                if (!(diag1 & (1u << d1)) &&
                    !(diag2 & (1u << d2))) {
                    valid |= bit;
                }
            }

            int count = __builtin_popcount(valid);

            if (count == 0) {
                return;
            }

            if (count < minCount) {
                minCount = count;
                bestRow = r;
                bestMask = valid;

                if (count == 1) {
                    break;
                }
            }
        }

        assigned[bestRow] = true;

        while (bestMask) {
            uint32_t bit = bestMask & -bestMask;
            bestMask ^= bit;

            int c = __builtin_ctz(bit);

            placement[bestRow] = c;

            int d1 = bestRow - c + n - 1;
            int d2 = bestRow + c;

            self(
                self,
                depth + 1,
                cols | bit,
                diag1 | (1u << d1),
                diag2 | (1u << d2)
            );
        }

        assigned[bestRow] = false;
    };

    search(search, 0, 0, 0, 0);

    return solutions;
}
