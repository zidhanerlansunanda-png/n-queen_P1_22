#include "nqueens.h"
#include <cstdint>
#include <vector>

using namespace std;

vector<vector<int>> solvePreplaced(
    int n,
    const vector<pair<int, int>>& preplaced
) {
    vector<vector<int>> solutions;

    if (n <= 0 || n > 12) {
        return solutions;
    }

    // Bitmask untuk kolom dan kedua arah diagonal.
    uint32_t usedCols = 0;
    uint32_t diagDown = 0;
    uint32_t diagUp = 0;

    // Menyimpan posisi queen pada setiap baris.
    int placement[12];

    // Menandai baris yang sudah memiliki queen.
    uint32_t assignedRows = 0;

    for (int r = 0; r < n; ++r) {
        placement[r] = -1;
    }

    // Masukkan seluruh queen yang sudah ditentukan.
    for (const auto& queen : preplaced) {
        int r = queen.first;
        int c = queen.second;

        // Validasi koordinat.
        if (r < 0 || r >= n || c < 0 || c >= n) {
            return solutions;
        }

        uint32_t colBit = 1u << c;
        uint32_t downBit = 1u << (r - c + n - 1);
        uint32_t upBit = 1u << (r + c);

        // Tolak jika queen saling menyerang.
        if ((usedCols & colBit) ||
            (diagDown & downBit) ||
            (diagUp & upBit)) {
            return solutions;
        }

        usedCols |= colBit;
        diagDown |= downBit;
        diagUp |= upBit;

        assignedRows |= (1u << r);
        placement[r] = c;
    }

    const uint32_t allRows = (1u << n) - 1u;

    auto backtrack = [&](auto&& self,
                         uint32_t rows,
                         uint32_t cols,
                         uint32_t down,
                         uint32_t up) -> void {

        // Semua baris sudah terisi.
        if (rows == allRows) {
            solutions.emplace_back(placement, placement + n);
            return;
        }

        int bestRow = -1;
        int minCount = n + 1;
        uint32_t bestCandidates = 0;

        // MRV: pilih baris dengan kandidat valid paling sedikit.
        for (int r = 0; r < n; ++r) {
            if (rows & (1u << r)) {
                continue;
            }

            uint32_t candidates = 0;
            int count = 0;

            for (int c = 0; c < n; ++c) {
                uint32_t colBit = 1u << c;
                uint32_t downBit = 1u << (r - c + n - 1);
                uint32_t upBit = 1u << (r + c);

                if (!(cols & colBit) &&
                    !(down & downBit) &&
                    !(up & upBit)) {

                    candidates |= colBit;
                    ++count;
                }
            }

            // Tidak ada posisi valid: langsung prune.
            if (count == 0) {
                return;
            }

            if (count < minCount) {
                minCount = count;
                bestRow = r;
                bestCandidates = candidates;

                // Tidak mungkin mendapatkan kandidat
                // lebih sedikit daripada satu.
                if (minCount == 1) {
                    break;
                }
            }
        }

        if (bestRow == -1) {
            return;
        }

        // Coba setiap posisi valid pada baris terpilih.
        uint32_t available = bestCandidates;

        while (available) {
            uint32_t bit = available & (~available + 1u);
            available &= available - 1u;

            int col = 0;
            while (!(bit & (1u << col))) {
                ++col;
            }

            uint32_t downBit =
                1u << (bestRow - col + n - 1);

            uint32_t upBit =
                1u << (bestRow + col);

            placement[bestRow] = col;

            self(
                self,
                rows | (1u << bestRow),
                cols | bit,
                down | downBit,
                up | upBit
            );

            placement[bestRow] = -1;
        }
    };

    backtrack(
        backtrack,
        assignedRows,
        usedCols,
        diagDown,
        diagUp
    );

    return solutions;
}
