
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

    bool hasBits(const Bits& bits) const {
        for (int w = 0; w < words; ++w) {
            if (bits[w]) return true;
        }
        return false;
    }

    int firstVertex(const Bits& bits) const {
        for (int w = 0; w < words; ++w) {
            if (bits[w]) {
                return w * 64 + __builtin_ctzll(bits[w]);
            }
        }
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

        int order[144];
        int bounds[144];
        int orderSize = 0;
        int color = 0;

        Bits uncolored = available;

        while (hasBits(uncolored)) {
            ++color;
            Bits independent = uncolored;

            while (hasBits(independent)) {
                int vertex = firstVertex(independent);
                int word = vertex / 64;
                uint64_t bit = 1ULL << (vertex % 64);

                uncolored[word] &= ~bit;
                independent[word] &= ~bit;

                for (int w = 0; w < words; ++w) {
                    independent[w] &= ~compatible[vertex][w];
                }

                order[orderSize] = vertex;
                bounds[orderSize] = color;
                ++orderSize;
            }
        }

        Bits remaining = available;

        for (int i = orderSize - 1; i >= 0; --i) {
            if (currentSize + bounds[i] < bestSize) {
                return;
            }

            int vertex = order[i];
            int word = vertex / 64;
            uint64_t bit = 1ULL << (vertex % 64);

            if (!(remaining[word] & bit)) {
                continue;
            }

            Bits next = remaining;

            for (int w = 0; w < words; ++w) {
                next[w] &= compatible[vertex][w];
            }

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
                    candidates[id / 64] |= 1ULL << (id % 64);
                }
            }
        }

        for (int id = 0; id < total; ++id) {
            int r = id / n;
            int c = id % n;
            int word = id / 64;
            uint64_t selfBit = 1ULL << (id % 64);

            if (!(candidates[word] & selfBit)) {
                continue;
            }

            for (int w = 0; w < words; ++w) {
                compatible[id][w] = candidates[w];
            }

            compatible[id][word] &= ~selfBit;

            for (int d = 0; d < 8; ++d) {
                int nr = r + dr[d];
                int nc = c + dc[d];

                while (nr >= 0 && nr < n && nc >= 0 && nc < n) {
                    if (grid[nr][nc] == 'W') {
                        break;
                    }

                    int other = nr * n + nc;
                    compatible[id][other / 64] &=
                        ~(1ULL << (other % 64));

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
        if (row.size() != static_cast<size_t>(n)) {
            return {};
        }
    }

    WalledSolver solver;
    solver.n = n;
    solver.total = n * n;
    solver.words = (solver.total + 63) / 64;

    solver.buildCompatibility(grid);

    return std::move(solver.solutions);
}
