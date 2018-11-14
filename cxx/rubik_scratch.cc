#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

#include "rubik.h"
#include "rubik_impl.h"

using namespace rubik;
using namespace std;

static Cube solved;

int edge_heuristic(const Cube &pos) {
    auto mask = _mm_cmpeq_epi8(pos.getEdges(), solved.getEdges());
    int inplace = __builtin_popcount(_mm_movemask_epi8(mask) & 0x0fff);
    int missing = 12 - inplace;
    static const int lookup[13] = {
        0,
        1, 1, 1, 1,
        2, 2, 2, 2,
        3, 3,
        4, 4,
    };
    return lookup[missing];
}

int heuristic(const Cube &pos) {
    auto mask = _mm_cmpeq_epi8(pos.getEdges(), solved.getEdges());
    int inplace = __builtin_popcount(_mm_movemask_epi8(mask) & 0x0fff);
    int missing = 12 - inplace;
    static const int lookup[13] = {
        0,
        1, 1, 1, 1,
        2, 2, 2, 2,
        3, 3,
        4, 4,
    };
    return lookup[missing];
}

void search_heuristic(int max_depth) {
    vector<vector<int>> vals(max_depth + 1);
    for (auto &v : vals) {
        v.resize(13, 0);
    }

    search(
            Cube(), *ftm_root, max_depth,
            [&](const Cube &pos, int depth) {
                int h = heuristic(pos);
                ++vals[depth][h];
            });

    cout << "heuristic:\n";
    reverse(vals.begin(), vals.end());

    int i = -1;
    for (auto &depth : vals) {
        cout << "d=" << (++i) << ": [";

        uint64_t sum = 0, count = 0;
        int idx = 0;

        bool first = true;
        for (auto e : depth) {
            if (first) {
                first = false;
            } else {
                cout << ' ';
            }
            cout << setw(8) << e;
            sum += idx*e;
            count += e;
            ++idx;
        }
        cout << "] n=" << count << " avg=" << ((double)sum)/count << "\n";
    }
}

void search_face() {
    auto superflip = rubik::from_algorithm(
            "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2");

    const auto edge_mask = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff);
    const auto corner_mask = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff);

    for (int depth = 0; depth < 15; ++depth) {
        vector<Cube> path;
        bool ok = search(
                superflip, *ftm_root, depth,
                [&](const Cube &pos, int) {
                    return _mm_test_all_zeros(
                            _mm_or_si128(
                                    _mm_and_si128(
                                            edge_mask,
                                            _mm_xor_si128(pos.getEdges(), solved.getEdges())),
                                    _mm_and_si128(
                                            corner_mask,
                                            _mm_xor_si128(pos.getCorners(), solved.getCorners()))),
                            _mm_set_epi32(0, 0xffffffff, 0xffffffff, 0xffffffff));
                },
                [&](const Cube &pos, int depth) {
                    return false;
                },
                [&](int, const Cube &rot) {
                    path.push_back(rot);
                });
        if (!ok) {
            cout << "depth=" << depth << ": no\n";
        } else {
            cout << "depth=" << depth << ": yes: "<< to_algorithm(path) << "\n";
            return;
        }
    }
}

int main() {
    search_face();

    return 0;
}
