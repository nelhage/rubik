#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <assert.h>

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

#include "rubik.h"
#include "rubik_impl.h"
#include "tables.h"

using namespace rubik;
using namespace std;

static Cube solved;

int face_heuristic(const Cube &pos) {
    rubik::edge_union eu;
    rubik::corner_union cu;

    eu.mm = pos.getEdges();
    cu.mm = pos.getCorners();

    int h = 0;
    for (uint i = 0; i < eu.arr.size(); ++i) {
        auto v = eu.arr[i];
        if ((v & rubik::Cube::kEdgePermMask) >= 4)
            continue;
        h = max<int>(h, edge_dist[(i << 5) | v]);
    }
    for (uint i = 0; i < cu.arr.size(); ++i) {
        auto v = cu.arr[i];
        if ((v & rubik::Cube::kCornerPermMask) >= 4)
            continue;
        h = max<int>(h, corner_dist[(i << 5) | v]);
    }
    return h;
}

void search_heuristic(int max_depth) {
    vector<vector<int>> vals(max_depth + 1);
    for (auto &v : vals) {
        v.resize(13, 0);
    }

    search(
            Cube(), *ftm_root, max_depth,
            [&](const Cube &pos, int depth) {
                int h = face_heuristic(pos);
                assert(h <= max_depth - depth);
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

bool face_prune(const Cube &pos, int depth) {
    rubik::edge_union eu;
    rubik::corner_union cu;

    eu.mm = pos.getEdges();
    cu.mm = pos.getCorners();

    for (uint i = 0; i < eu.arr.size(); ++i) {
        auto v = eu.arr[i];
        if ((v & rubik::Cube::kEdgePermMask) >= 4)
            continue;
        if (edge_dist[(i << 5) | v] > depth) {
            return true;
        }
    }
    for (uint i = 0; i < cu.arr.size(); ++i) {
        auto v = cu.arr[i];
        if ((v & rubik::Cube::kCornerPermMask) >= 4)
            continue;
        if (corner_dist[(i << 5) | v] > depth) {
            return true;
        }
    }
    return false;
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
                    return face_prune(pos, depth);
                },
                [&](int, const Cube &rot) {
                    path.push_back(rot);
                });
        if (!ok) {
            cout << "depth=" << depth << ": no\n";
        } else {
            reverse(path.begin(), path.end());
            cout << "depth=" << depth << ": yes: "<< to_algorithm(path) << "\n";
            return;
        }
    }
}

int main() {
    search_face();
    // search_heuristic(6);

    return 0;
}
