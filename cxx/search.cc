#include "rubik.h"
#include "tables.h"
#include "rubik_impl.h"

#include <vector>
#include <iostream>

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

using namespace std;

namespace rubik {

namespace {
Cube solved;

Cube must_parse(const std::string &str) {
    auto r = rubik::from_facelets(str);
    if (!absl::holds_alternative<Cube>(r)) {
        cerr << "bad: " << str << ": " << get<rubik::Error>(r).error << "\n";
        abort();
    }
    return get<Cube>(r);
}

vector<pair<Cube, Cube>> compute_symmetries() {
    auto yaw   = must_parse("GGGGWGGGGYYYRRRWWWOOOYGYRRRWBWOOOYYYRRRWWWOOOBBBBYBBBB");
    auto pitch = must_parse("RRRRWRRRRGGGYYYBBBWWWGGGYRYBBBWOWGGGYYYBBBWWWOOOOYOOOO");
    auto roll  = must_parse("WWWWWWWWWOOOGGGRRRBBBOGOGRGRBRBOBOOOGGGRRRBBBYYYYYYYYY");

    vector<Cube> symmetries{
        yaw,
        yaw.apply(yaw),
        yaw.invert(),
        pitch,
        pitch.apply(pitch),
        pitch.invert(),
        roll,
        roll.apply(roll),
        roll.invert(),
    };

    if (debug_mode) {
        for (const auto &s1 : symmetries) {
            for (const auto &s2 : symmetries) {
                if (&s1 == &s2)
                    continue;
                if (s1 == s2) {
                    cerr << "identical symmetries: " << (&s1 - &symmetries.front()) << "/" << (&s2 - &symmetries.front()) << "\n";
                    abort();
                }
            }
        }
    }
    vector<pair<Cube, Cube>> out;
    out.reserve(symmetries.size());
    for (auto &sym : symmetries) {
        out.emplace_back(sym, sym.invert());
    }
    return out;
}

bool prune_two(const Cube &pos, int depth) __attribute__((used));
bool prune_two(const Cube &pos, int depth) {
    auto inv = pos.invert();
    edge_union eu;
    corner_union cu;
    eu.mm = inv.getEdges();
    cu.mm = inv.getCorners();
    auto d = pair0_dist[(eu.arr[0] << 5) | cu.arr[0]];
    if (d > depth) {
        return true;
    }
    for (auto &p : symmetries) {
        auto c = p.second.apply(inv.apply(p.first));
        eu.mm = c.getEdges();
        cu.mm = c.getCorners();
        if (pair0_dist[(eu.arr[0] << 5) | cu.arr[0]] > depth) {
            return true;
        }
    }
    return false;
}

bool prune_quad(const Cube &pos, int depth) {
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
    if (d > depth) {
        return true;
    }
    for (auto &p : symmetries) {
        auto c = p.second.apply(inv.apply(p.first));
        eu.mm = c.getEdges();
        cu.mm = c.getCorners();
        auto d = rubik::quad01_dist[(eu.arr[0] << 15) |
                                    (eu.arr[1] << 10) |
                                    (cu.arr[0] << 5) |
                                    (cu.arr[1])];
        if (d > depth) {
            return true;
        }
    }
    return false;
}

};

const vector<pair<Cube, Cube>> symmetries = compute_symmetries();

int flip_heuristic(const Cube &pos) {
    auto mask = _mm_slli_epi16(pos.getEdges(), 3);
    int flipped = __builtin_popcount(_mm_movemask_epi8(mask) & 0x0fff);
    assert(flipped >= 0 && flipped <= 12);
    static const int depths[13] = {
        0,
        1, 1, 1, 1,
        2, 2,
        3, 3,
        5, 5,
        6, 6,
    };
    return depths[flipped];
}

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

bool search(Cube start, vector<Cube> &path, int max_depth) {
    path.resize(0);
    bool ok = search(
            start, *qtm_root, max_depth,
            [&](const Cube &pos, int) {
                return (pos == solved);
            },
            [&](const Cube &pos, int depth) {
                auto edge = edge_heuristic(pos);
                if (depth < edge)
                    return true;
                auto flip = flip_heuristic(pos);
                if (depth < flip)
                    return true;
                return prune_quad(pos, depth);
                // return false;
            },
            [&](int depth, const Cube &rot) {
                path.push_back(rot);
            });
    if (ok) {
        reverse(path.begin(), path.end());
    }
    return ok;
}

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
    Cube solved;

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
                    if (prefix_prune(pos, n, depth)) {
                        return true;
                    }
                    if (n >= 2) {
                        auto inv = pos.invert();
                        edge_union eu;
                        corner_union cu;
                        eu.mm = inv.getEdges();
                        cu.mm = inv.getCorners();
                        auto d = pair0_dist[(eu.arr[0] << 5) | cu.arr[0]];
                        if (d > depth) {
                            return true;
                        }
                    }
                    return false;
                },
                [&](int, const Cube &) {});
        if (ok) {
            return depth;
        }
    }
}

};
