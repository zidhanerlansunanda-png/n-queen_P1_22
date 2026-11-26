#include "nqueens.h"
#include <array>
#include <cstdint>
#include <vector>

std::vector<std::vector<std::pair<int, int>>> solveWalled(
    int n,
    const std::vector<std::string>& grid
) {
    using Bits = std::array<uint64_t, 3>;
    using Board = std::vector<std::pair<int, int>>;

    std::vector<Board> solutions;

    if (n <= 0 || n > 12) {
        return solutions;
    }

    const int total = n * n;
    const int words = (total + 63) / 64;

    Bits conflict[144] = {};
    Bits compatible[144] = {};
    Bits candidates = {};

    const int dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            if (grid[r][c] == 'W') {
                continue;
            }

            int id = r * n + c;
            candidates[id / 64] |= 1ULL << (id % 64);

            for (int d = 0; d < 8; ++d) {
                int nr = r + dr[d];
                int nc = c + dc[d];

                while (nr >= 0 && nr < n && nc >= 0 && nc < n) {
                    if (grid[nr][nc] == 'W') {
                        break;
                    }

                    int other = nr * n + nc;
                    conflict[id][other / 64] |=
                        1ULL << (other % 64);

                    nr += dr[d];
                    nc += dc[d];
                }
            }
        }
    }

    for (int id = 0; id < total; ++id) {
        uint64_t selfBit = 1ULL << (id % 64);

        if (!(candidates[id / 64] & selfBit)) {
            continue;
        }

        for (int w = 0; w < words; ++w) {
            compatible[id][w] =
                candidates[w] & ~conflict[id][w];
        }

        compatible[id][id / 64] &= ~selfBit;
    }

    int bestSize = -1;
    std::vector<int> current;

    auto popcount = [&](const Bits& bits) {
        int count = 0;

        for (int w = 0; w < words; ++w) {
            count += __builtin_popcountll(bits[w]);
        }

        return count;
    };

    auto firstBit = [](uint64_t x) {
        return __builtin_ctzll(x);
    };

    auto saveSolution = [&]() {
        int count = static_cast<int>(current.size());

        if (count > bestSize) {
            bestSize = count;
            solutions.clear();
        }

        if (count == bestSize) {
            Board board;
            board.reserve(count);

            for (int id : current) {
                board.emplace_back(id / n, id % n);
            }

            solutions.push_back(std::move(board));
        }
    };

    auto search = [&](auto&& self, Bits available) -> void {
        if (popcount(available) == 0) {
            saveSolution();
            return;
        }

        std::vector<int> order;
        std::vector<int> bounds;

        order.reserve(total);
        bounds.reserve(total);

        Bits uncolored = available;
        int color = 0;

        while (popcount(uncolored) > 0) {
            ++color;

            Bits independent = uncolored;

            while (true) {
                int vertex = -1;

                for (int w = 0; w < words; ++w) {
                    if (independent[w]) {
                        vertex = w * 64 + firstBit(independent[w]);
                        break;
                    }
                }

                if (vertex < 0 || vertex >= total) {
                    break;
                }

                uint64_t bit = 1ULL << (vertex % 64);

                uncolored[vertex / 64] &= ~bit;
                independent[vertex / 64] &= ~bit;

                for (int w = 0; w < words; ++w) {
                    independent[w] &= ~compatible[vertex][w];
                }

                order.push_back(vertex);
                bounds.push_back(color);
            }
        }

        Bits remaining = available;

        for (int i = static_cast<int>(order.size()) - 1;
             i >= 0; --i) {

            if (static_cast<int>(current.size()) + bounds[i]
                < bestSize) {
                return;
            }

            int vertex = order[i];
            uint64_t bit = 1ULL << (vertex % 64);

            if (!(remaining[vertex / 64] & bit)) {
                continue;
            }

            Bits next = remaining;

            for (int w = 0; w < words; ++w) {
                next[w] &= compatible[vertex][w];
            }

            current.push_back(vertex);
            self(self, next);
            current.pop_back();

            remaining[vertex / 64] &= ~bit;
        }
    };

    search(search, candidates);

    return solutions;
}
