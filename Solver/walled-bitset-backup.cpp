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

    Bits adj[144] = {};
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
                    adj[id][other / 64] |= 1ULL << (other % 64);

                    nr += dr[d];
                    nc += dc[d];
                }
            }
        }
    }

    int bestSize = -1;
    Board current;

    auto popcount = [](const Bits& bits) -> int {
        return __builtin_popcountll(bits[0]) +
               __builtin_popcountll(bits[1]) +
               __builtin_popcountll(bits[2]);
    };

    auto search = [&](auto&& self, Bits available) -> void {
        int remaining = popcount(available);

        if (static_cast<int>(current.size()) + remaining < bestSize) {
            return;
        }

        if (remaining == 0) {
            int count = static_cast<int>(current.size());

            if (count > bestSize) {
                bestSize = count;
                solutions.clear();
            }

            if (count == bestSize) {
                solutions.push_back(current);
            }

            return;
        }

        int chosen = -1;
        int maxDegree = -1;

        for (int id = 0; id < total; ++id) {
            uint64_t bit = 1ULL << (id % 64);

            if (!(available[id / 64] & bit)) {
                continue;
            }

            int degree = 0;

            for (int w = 0; w < words; ++w) {
                degree += __builtin_popcountll(
                    adj[id][w] & available[w]
                );
            }

            if (degree > maxDegree) {
                maxDegree = degree;
                chosen = id;
            }
        }

        if (chosen < 0) {
            return;
        }

        uint64_t chosenBit = 1ULL << (chosen % 64);

        Bits withQueen = available;
        withQueen[chosen / 64] &= ~chosenBit;

        for (int w = 0; w < words; ++w) {
            withQueen[w] &= ~adj[chosen][w];
        }

        current.emplace_back(chosen / n, chosen % n);
        self(self, withQueen);
        current.pop_back();

        Bits withoutQueen = available;
        withoutQueen[chosen / 64] &= ~chosenBit;

        self(self, withoutQueen);
    };

    search(search, candidates);

    return solutions;
}
