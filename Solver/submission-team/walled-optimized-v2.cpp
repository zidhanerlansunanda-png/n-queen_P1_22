
#include "nqueens.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

using Bits = std::array<uint64_t, 3>;
using Board = std::vector<std::pair<int, int>>;

struct WalledSolver {
    int n;
    int total;
    int words;
    int bestSize = -1;

    Bits compatible[144] = {};
    int current[144];
    int currentSize = 0;

    std::vector<Board> solutions;

    inline bool hasBits(const Bits& b) const {
        return b[0] || (words > 1 && b[1]) ||
               (words > 2 && b[2]);
    }

    inline int firstVertex(const Bits& b) const {
        if (b[0]) return __builtin_ctzll(b[0]);
        if (words > 1 && b[1])
            return 64 + __builtin_ctzll(b[1]);
        if (words > 2 && b[2])
            return 128 + __builtin_ctzll(b[2]);
        return -1;
    }

    void saveSolution() {
        if (currentSize < bestSize) return;

        if (currentSize > bestSize) {
            bestSize = currentSize;
            solutions.clear();
        }

        Board board;
        board.reserve(currentSize);

        for (int i = 0; i < currentSize; ++i) {
            int id = current[i];
            board.emplace_back(id / n, id % n);
        }

        solutions.push_back(std::move(board));
    }

    void search(Bits available) {
        if (!hasBits(available)) {
            saveSolution();
            return;
        }

        uint8_t order[144];
        uint8_t bounds[144];

        int orderSize = 0;
        int color = 0;

        Bits uncolored = available;

        while (hasBits(uncolored)) {
            ++color;
            Bits independent = uncolored;

            while (hasBits(independent)) {
                int vertex = firstVertex(independent);
                int word = vertex >> 6;
                uint64_t bit = 1ULL << (vertex & 63);

                uncolored[word] &= ~bit;
                independent[word] &= ~bit;

                const Bits& comp = compatible[vertex];

                independent[0] &= ~comp[0];
                if (words > 1) independent[1] &= ~comp[1];
                if (words > 2) independent[2] &= ~comp[2];

                order[orderSize] = static_cast<uint8_t>(vertex);
                bounds[orderSize] = static_cast<uint8_t>(color);
                ++orderSize;
            }
        }

        Bits remaining = available;

        for (int i = orderSize - 1; i >= 0; --i) {
            if (currentSize + bounds[i] < bestSize)
                return;

            int vertex = order[i];
            int word = vertex >> 6;
            uint64_t bit = 1ULL << (vertex & 63);

            if (!(remaining[word] & bit))
                continue;

            Bits next;

            const Bits& comp = compatible[vertex];

            next[0] = remaining[0] & comp[0];
            next[1] = remaining[1] & comp[1];
            next[2] = remaining[2] & comp[2];

            current[currentSize++] = vertex;
            search(next);
            --currentSize;

            remaining[word] &= ~bit;
        }
    }

    void buildCompatibility(const std::vector<std::string>& grid) {
        const int dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
        const int dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

        Bits candidates = {};

        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                if (grid[r][c] != 'W') {
                    int id = r * n + c;
                    candidates[id >> 6] |= 1ULL << (id & 63);
                }
            }
        }

        for (int id = 0; id < total; ++id) {
            int r = id / n;
            int c = id % n;
            int word = id >> 6;
            uint64_t selfBit = 1ULL << (id & 63);

            if (!(candidates[word] & selfBit))
                continue;

            compatible[id][0] = candidates[0];
            compatible[id][1] = candidates[1];
            compatible[id][2] = candidates[2];

            compatible[id][word] &= ~selfBit;

            for (int d = 0; d < 8; ++d) {
                int nr = r + dr[d];
                int nc = c + dc[d];

                while (nr >= 0 && nr < n &&
                       nc >= 0 && nc < n) {
                    if (grid[nr][nc] == 'W')
                        break;

                    int other = nr * n + nc;
                    compatible[id][other >> 6] &=
                        ~(1ULL << (other & 63));

                    nr += dr[d];
                    nc += dc[d];
                }
            }
        }

        search(candidates);
    }
};

std::vector<std::vector<std::pair<int, int>>> solveWalled(
    int n,
    const std::vector<std::string>& grid
) {
    if (n <= 0 || n > 12 ||
        grid.size() != static_cast<size_t>(n)) {
        return {};
    }

    for (const auto& row : grid) {
        if (row.size() != static_cast<size_t>(n))
            return {};
    }

    WalledSolver solver;
    solver.n = n;
    solver.total = n * n;
    solver.words = (solver.total + 63) / 64;

    solver.buildCompatibility(grid);

    return std::move(solver.solutions);
}
