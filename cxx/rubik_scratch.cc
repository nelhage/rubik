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
static array<int8_t, 32*32> two_dist;

bool prefix_prune(const Cube &pos, int n, int depth) {
    rubik::edge_union eu;
    rubik::corner_union cu;

    eu.mm = pos.getEdges();
    cu.mm = pos.getCorners();

    for (uint i = 0; i < eu.arr.size(); ++i) {
        auto v = eu.arr[i];
        if ((v & rubik::Cube::kEdgePermMask) >= n)
            continue;
        if (edge_dist[(i << 5) | v] > depth) {
            return true;
        }
    }
    for (uint i = 0; i < cu.arr.size(); ++i) {
        auto v = cu.arr[i];
        if ((v & rubik::Cube::kCornerPermMask) >= n)
            continue;
        if (corner_dist[(i << 5) | v] > depth) {
            return true;
        }
    }
    return false;
}

int prefix_search(const Cube &init, int n) {
    union {
        uint8_t bits[16];
        __m128i mm;
    } maskbits;
    for (int i = 0; i < 16; i++) {
        if (i < n) {
            maskbits.bits[i] = 0xff;
        } else {
            maskbits.bits[i] = 0;
        }
    }
    const auto mask = maskbits.mm;

    for (int depth = 0;; ++depth) {
        bool ok = search(
                init, *qtm_root, depth,
                [&](const Cube &pos, int) {
                    return _mm_test_all_zeros(
                            _mm_or_si128(
                                    _mm_xor_si128(pos.getEdges(), solved.getEdges()),
                                    _mm_xor_si128(pos.getCorners(), solved.getCorners())),
                            mask);
                },
                [&](const Cube &pos, int depth) {
                    return prefix_prune(pos, n, depth);
                },
                [&](int, const Cube &) {});
        if (ok) {
            return depth;
        }
    }
}

void precompute() {
    vector<int8_t> all_edge;
    vector<int8_t> all_corner;
    for (int e = 0; e < 12; ++e) {
        for (int ea = 0; ea < 2; ++ea) {
            all_edge.push_back((ea << rubik::Cube::kEdgeAlignShift) | e);
        }
    }
    for (int c = 0; c < 8; ++c) {
        for (int ca = 0; ca < 3; ++ca) {
            all_corner.push_back((ca << rubik::Cube::kCornerAlignShift) | c);
        }
    }


    for (auto e : all_edge) {
        for (auto c: all_corner) {
            rubik::edge_union eu;
            rubik::corner_union cu;
            eu.mm = solved.getEdges();
            cu.mm = solved.getCorners();

            eu.arr[e & rubik::Cube::kEdgePermMask] = 0;
            eu.arr[0] = e;
            cu.arr[c & rubik::Cube::kCornerPermMask] = 0;
            cu.arr[0] = c;

            Cube pos(eu.mm, cu.mm);
            int d = prefix_search(pos.invert(), 1);
            two_dist[(e << 5) | c] = d;
        }
    }
}

int two_heuristic(const Cube &pos) {
    auto inv = pos.invert();
    edge_union eu;
    corner_union cu;
    eu.mm = inv.getEdges();
    cu.mm = inv.getCorners();
    int d = two_dist[(eu.arr[0] << 5) | cu.arr[0]];
    if (debug_mode) {
        int canon = prefix_search(pos, 1);
        if (d != canon) {
            abort();
        }
    }
    return d;
}

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
            Cube(), *qtm_root, max_depth,
            [&](const Cube &pos, int depth) {
                int h = two_heuristic(pos);
                assert(h <= max_depth - depth);
                ++vals[depth][h];
            });

    cout << "heuristic:\n";
    reverse(vals.begin(), vals.end());

    int i = -1;
    for (auto &depth : vals) {
        cout << "d=" << (++i) << ": [";

        int dmax = 0;
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
            if (e > 0) {
                dmax = idx;
            }
            ++idx;
        }
        cout << "] n=" << count << " avg=" << ((double)sum)/count << " max=" << dmax << "\n";
    }
}

void search_two() {
    auto superflip = rubik::from_algorithm(
            "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2");

    const auto edge_mask = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff);
    const auto corner_mask = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff);

    for (int depth = 0; depth < 15; ++depth) {
        vector<Cube> path;
        bool ok = search(
                superflip, *qtm_root, depth,
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
                    if (prefix_prune(pos, 4, depth)) {
                        return true;
                    }
                    auto inv = pos.invert();
                    edge_union eu;
                    corner_union cu;
                    eu.mm = inv.getEdges();
                    cu.mm = inv.getCorners();
                    auto d = two_dist[(eu.arr[0] << 5) | cu.arr[0]];
                    return (d > depth);
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
    precompute();

    // search_heuristic(6);
    search_two();

    return 0;
}
