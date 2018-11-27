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

vector<pair<Cube, Cube>> topSymmetries;

void precompute() {
    auto r = rubik::from_facelets("GGGGWGGGGYYYRRRWWWOOOYGYRRRWBWOOOYYYRRRWWWOOOBBBBYBBBB");
    if (!absl::holds_alternative<Cube>(r)) {
        cerr << "bad1: " << get<rubik::Error>(r).error << "\n";
        abort();
    }
    auto rot = get<Cube>(r);
    /*
    r = rubik::from_facelets("WWWWWWWWW BBBRRRGGGOOO BGBRRRGBGOOO BBBRRRGGGOOO YYYYYYYYY");
    if (!absl::holds_alternative<Cube>(r)) {
        cerr << "bad2: " << get<rubik::Error>(r).error << "\n";
        abort();
    }
    auto mirror = get<Cube>(r);
    */
    topSymmetries = {
        {rot, rot.invert()},
        {rot.apply(rot), rot.apply(rot)},
        {rot.invert(), rot},
        /*
          mirror,
        rot.apply(mirror),
        rot.apply(rot).apply(mirror),
        rot.invert().apply(mirror),
        */
    };
    if (debug_mode) {
        for (const auto &s1 : topSymmetries) {
            for (const auto &s2 : topSymmetries) {
                if (&s1 == &s2)
                    continue;
                if (s1.first == s2.first) {
                    cerr << "identical symmetries: " << (&s1 - &topSymmetries.front()) << "/" << (&s2 - &topSymmetries.front()) << "\n";
                    abort();
                }
            }
            assert(s1.first == s1.second.invert());
        }
    }
}

int two_heuristic(const Cube &pos) {
    auto inv = pos.invert();
    edge_union eu;
    corner_union cu;
    eu.mm = inv.getEdges();
    cu.mm = inv.getCorners();
    int d = rubik::pair0_dist[(eu.arr[0] << 5) | cu.arr[0]];
    for (auto &p : symmetries) {
        auto c = p.second.apply(inv.apply(p.first));
        eu.mm = c.getEdges();
        cu.mm = c.getCorners();
        d = max<int>(d, rubik::pair0_dist[(eu.arr[0] << 5) | cu.arr[0]]);
    }
    return d;
}

int quad_heuristic(const Cube &pos) {
    auto inv = pos.invert();
    edge_union eu;
    corner_union cu;
    eu.mm = inv.getEdges();
    cu.mm = inv.getCorners();
    int d = rubik::quad01_dist[(eu.arr[0] << 15) |
                               (eu.arr[1] << 10) |
                               (cu.arr[0] << 5) |
                               (cu.arr[1])];
    assert(d >= 0);
    for (auto &p : symmetries) {
        auto c = p.second.apply(inv.apply(p.first));
        eu.mm = c.getEdges();
        cu.mm = c.getCorners();
        d = max<int>(d, rubik::quad01_dist[(eu.arr[0] << 15) |
                                           (eu.arr[1] << 10) |
                                           (cu.arr[0] << 5) |
                                           (cu.arr[1])]);
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
         h = max<int>(h, edge_dist[(i << 5) | v]);
    }
    for (uint i = 0; i < cu.arr.size(); ++i) {
        auto v = cu.arr[i];
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
                size_t h = quad_heuristic(pos);
                assert(h <= (size_t)(max_depth - depth));
                if (h >= vals[depth].size()) {
                    return;
                }
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
    auto superflip = get<Cube>(
            rubik::from_algorithm(
                    "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2"));

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
                    auto inv = pos.invert();
                    edge_union eu;
                    corner_union cu;
                    eu.mm = inv.getEdges();
                    cu.mm = inv.getCorners();
                    auto d = rubik::pair0_dist[(eu.arr[0] << 5) | cu.arr[0]];
                    if (d > depth) {
                        return true;
                    }
                    for (auto &p : topSymmetries) {
                        auto c = p.second.apply(inv.apply(p.first));
                        eu.mm = c.getEdges();
                        cu.mm = c.getCorners();
                        if (rubik::pair0_dist[(eu.arr[0] << 5) | cu.arr[0]] > depth) {
                            return true;
                        }
                    }
                    return false;
                },
                [&](int, const Cube &rot) {
                    path.push_back(rot);
                });
        if (!ok) {
            cout << "depth=" << depth << ": no\n";
        } else {
            reverse(path.begin(), path.end());
            cout << "depth=" << depth << ": yes: "<< get<string>(to_algorithm(path)) << "\n";
            return;
        }
    }
}

int main() {
    precompute();

    search_heuristic(8);
    // search_two();

    return 0;
}
