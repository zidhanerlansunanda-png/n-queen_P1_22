
#include "nqueens.h"
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

static void dfsHoles(
    int n,
    int row,
    uint32_t full,
    const uint32_t* allowed,
    uint32_t cols,
    uint32_t left,
    uint32_t right,
    int* placement,
    vector<vector<int>>& solutions
) {
    if (row == n) {
        solutions.emplace_back(placement, placement + n);
        return;
    }

    uint32_t available =
        allowed[row] & ~(cols | left | right) & full;

    while (available) {
        uint32_t bit = available & -available;
        available &= available - 1u;

        placement[row] = __builtin_ctz(bit);

        dfsHoles(
            n,
            row + 1,
            full,
            allowed,
            cols | bit,
            ((left | bit) << 1) & full,
            (right | bit) >> 1,
            placement,
            solutions
        );
    }
}

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

    uint32_t allowed[12] = {};

    for (int r = 0; r < n; ++r) {
        if (grid[r].size() != static_cast<size_t>(n)) {
            return solutions;
        }

        uint32_t mask = 0;

        for (int c = 0; c < n; ++c) {
            if (grid[r][c] == '.') {
                mask |= 1u << c;
            }
        }

        if (mask == 0) {
            return solutions;
        }

        allowed[r] = mask;
    }

    size_t reserveCount = 1;

    for (int r = 0; r < n; ++r) {
        unsigned choices = __builtin_popcount(allowed[r]);

        if (choices == 0) {
            return solutions;
        }

        if (reserveCount > static_cast<size_t>(counts[n]) / choices) {
            reserveCount = counts[n];
            break;
        }

        reserveCount *= choices;
    }

    solutions.reserve(reserveCount);

    int placement[12];

    dfsHoles(
        n,
        0,
        full,
        allowed,
        0u,
        0u,
        0u,
        placement,
        solutions
    );

    return solutions;
}
