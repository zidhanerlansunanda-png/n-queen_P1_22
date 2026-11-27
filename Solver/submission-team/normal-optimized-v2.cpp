
#include "nqueens.h"
#include <cstdint>
#include <vector>

using namespace std;

static void dfsNormal(
    int n,
    int row,
    uint32_t full,
    uint32_t cols,
    uint32_t left,
    uint32_t right,
    int* placement,
    bool mirror,
    vector<vector<int>>& solutions
) {
    if (row == n) {
        solutions.emplace_back(placement, placement + n);

        if (mirror) {
            solutions.emplace_back(n);

            vector<int>& mirrored = solutions.back();

            for (int r = 0; r < n; ++r) {
                mirrored[r] = n - 1 - placement[r];
            }
        }

        return;
    }

    uint32_t available = full & ~(cols | left | right);

    while (available) {
        uint32_t bit = available & -available;
        available &= available - 1u;

        placement[row] = __builtin_ctz(bit);

        dfsNormal(
            n,
            row + 1,
            full,
            cols | bit,
            ((left | bit) << 1) & full,
            (right | bit) >> 1,
            placement,
            mirror,
            solutions
        );
    }
}

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

    for (int col = 0; col < half; ++col) {
        uint32_t bit = 1u << col;

        placement[0] = col;

        dfsNormal(
            n,
            1,
            full,
            bit,
            (bit << 1) & full,
            bit >> 1,
            placement,
            true,
            solutions
        );
    }

    if (n & 1) {
        uint32_t bit = 1u << half;

        placement[0] = half;

        dfsNormal(
            n,
            1,
            full,
            bit,
            (bit << 1) & full,
            bit >> 1,
            placement,
            false,
            solutions
        );
    }

    return solutions;
}
