#include "rubik.h"

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

#include <algorithm>
#include <numeric>
#include <vector>

#include <iostream>
#include <iomanip>

#include <cassert>

using namespace std;

namespace rubik {

Cube::Cube() :
        edges(_mm_set_epi8(0, 0, 0, 0, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)),
        corners(_mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 7, 6, 5, 4, 3, 2, 1, 0)) {
    sanityCheck();
}

Cube::Cube(__m128i edges, __m128i corners)
        : edges(edges), corners(corners) {
    sanityCheck();
}

union edge_union {
    __m128i mm;
    struct {
        std::array<uint8_t, 12> arr;
        uint32_t pad;
    };
};

union corner_union {
    __m128i mm;
    struct {
        std::array<uint8_t, 8> arr;
        uint32_t pad;
    };
};

Cube::Cube(std::array<uint8_t, 12> edges,
           std::array<uint8_t, 8> corners) {
    edge_union e;
    e.pad = 0;
    e.arr = edges;
    this->edges = e.mm;

    corner_union c;
    c.pad = 0;
    c.arr = corners;
    this->corners = c.mm;
}


Cube Cube::apply(const Cube &other) const {
    auto edge_perm = _mm_and_si128(other.edges, _mm_set1_epi8(kEdgePermMask));
    auto out_edges = _mm_shuffle_epi8(edges, edge_perm);
    out_edges = _mm_xor_si128(out_edges, _mm_and_si128(other.edges, _mm_set1_epi8(kEdgeAlignMask)));

    auto corner_perm = _mm_and_si128(other.corners, _mm_set1_epi8(kCornerPermMask));
    auto out_corners = _mm_shuffle_epi8(corners, corner_perm);
    out_corners = _mm_add_epi8(out_corners,
                               _mm_and_si128(other.corners, _mm_set1_epi8(kCornerAlignMask)));
    auto lim = _mm_set1_epi8(3 << kCornerAlignShift);
    auto mask = _mm_cmplt_epi8(out_corners, lim);
    out_corners = _mm_sub_epi8(out_corners, _mm_andnot_si128(mask, lim));
    return Cube(out_edges, out_corners);
}

Cube Cube::invert() const {
    array<uint8_t, 12> out_edges;
    array<uint8_t, 8> out_corners;

    edge_union eu;
    corner_union cu;
    eu.mm = edges;
    cu.mm = corners;

    for (int i = 0; i < 12; ++i) {
        auto idx = eu.arr[i] & kEdgePermMask;
        out_edges[idx] = i;
    }
    for (int i = 0; i < 12; ++i) {
        auto idx = out_edges[i] & kEdgePermMask;
        out_edges[i] ^= eu.arr[idx] & kEdgeAlignMask;
    }
    for (int i = 0; i < 8; ++i) {
        auto idx = cu.arr[i] & kCornerPermMask;
        out_corners[idx] = i;
    }
    for (int i = 0; i < 8; ++i) {
        auto idx = out_corners[i] & kCornerPermMask;
        uint8_t align = (3 - ((cu.arr[idx] & kCornerAlignMask) >> kCornerAlignShift)) % 3;
        out_corners[i] |= align << kCornerAlignShift;
    }
    return Cube(out_edges, out_corners);
}

bool Cube::operator==(const Cube &rhs) const {
    return _mm_test_all_zeros(
            _mm_or_si128(
                    _mm_xor_si128(edges, rhs.edges),
                    _mm_and_si128(
                            _mm_set_epi32(0, 0, 0xffffffff, 0xffffffff),
                            _mm_xor_si128(corners, rhs.corners))),
            _mm_set_epi32(0, 0xffffffff, 0xffffffff, 0xffffffff));
}

void Cube::sanityCheck() const {
#ifndef NDEBUG
    edge_union eu;
    corner_union cu;

    eu.mm = edges;
    cu.mm = corners;

    /* assert(eu.pad == 0);
       assert(cu.pad == 0); */

    array<uint8_t, 12> edge_perms;
    edge_perms.fill(0);
    array<uint8_t, 8> corner_perms;
    corner_perms.fill(0);

    for (auto e : eu.arr) {
        assert((e & ~(kEdgePermMask|kEdgeAlignMask)) == 0);
        assert((e & kEdgePermMask) < 12);
        edge_perms[e & kEdgePermMask]++;
    }
    for (auto c : cu.arr) {
        assert((c & ~(kCornerPermMask|kCornerAlignMask)) == 0);
        assert((c & kCornerPermMask) < 12);
        corner_perms[c & kCornerPermMask]++;
    }

    for (int i = 0; i < 12; i++) {
        assert(edge_perms[i] == 1);
    }
    for (int i = 0; i < 8; i++) {
        assert(corner_perms[i] == 1);
    }
#endif
}


const Cube Rotations::L({4, 1, 2, 3, 8, 5, 6, 0, 7, 9, 10, 11},
                        {C1|4, C0|1, C0|2, C2|0, C2|7, C0|5, C0|6, C1|3});
const Cube Rotations::L2(L.apply(L));
const Cube Rotations::Linv(L.invert());

const Cube Rotations::R({0, 1, E|6, 3, 4, E|2, E|10, 7, 8, 9, E|5, 11},
                        {C0|0, C2|2, C1|6, C0|3, C0|4, C1|1, C2|5, C0|7});
const Cube Rotations::R2(R.apply(R));
const Cube Rotations::Rinv(R.invert());

const Cube Rotations::U({3, 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11},
                        {C0|3, C0|0, C0|1, C0|2, C0|4, C0|5, C0|6, C0|7});
const Cube Rotations::U2(U.apply(U));
const Cube Rotations::Uinv(U.invert());

const Cube Rotations::D({0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 8},
                        {C0|0, C0|1, C0|2, C0|3, C0|5, C0|6, C0|7, C0|4});
const Cube Rotations::D2(D.apply(D));
const Cube Rotations::Dinv(D.invert());

const Cube Rotations::F({0, 1, 2, E|7, 4, 5, 3, E|11, 8, 9, 10, 6},
                        {C0|0, C0|1, C2|3, C1|7, C0|4, C0|5, C1|2, C2|6});
const Cube Rotations::F2(F.apply(F));
const Cube Rotations::Finv(F.invert());

const Cube Rotations::B({0, 5, 2, 3, E|1, 9, 6, 7, 8, E|4, 10, 11},
                        {C2|1, C1|5, C0|2, C0|3, C1|0, C2|4, C0|6, C0|7});
const Cube Rotations::B2(B.apply(B));
const Cube Rotations::Binv(B.invert());

namespace {

class search_tree {
public:
    struct rot {
        const Cube &rotation;
        vector<rot> *next;
    };

    vector<rot> root,
        l, linv,
        r, rinv,
        u, uinv,
        d, dinv,
        f, finv,
        b, binv;

    search_tree()
            : root({
               rot {Rotations::L, &l},
               rot {Rotations::Linv, &linv},
               rot {Rotations::R, &r},
               rot {Rotations::Rinv, &rinv},
               rot {Rotations::U, &u},
               rot {Rotations::Uinv, &uinv},
               rot {Rotations::D, &d},
               rot {Rotations::Dinv, &dinv},
               rot {Rotations::F, &f},
               rot {Rotations::Finv, &finv},
               rot {Rotations::B, &b},
               rot {Rotations::Binv, &binv}})
    {
        const vector<pair<const Cube&, const Cube&>> inverses = {
            {Rotations::L, Rotations::Linv},
            {Rotations::R, Rotations::Rinv},
            {Rotations::U, Rotations::Uinv},
            {Rotations::D, Rotations::Dinv},
            {Rotations::F, Rotations::Finv},
            {Rotations::B, Rotations::Binv},
        };
        const vector<pair<const Cube&, const Cube&>> obverse = {
            {Rotations::L, Rotations::R},
            {Rotations::U, Rotations::D},
            {Rotations::F, Rotations::B},
        };
        for (auto &ent : root) {
            auto fwd = find_if(inverses.begin(),
                               inverses.end(),
                               [&](auto &pair) {
                                   return pair.first == ent.rotation;
                               });
            auto inv = find_if(inverses.begin(),
                               inverses.end(),
                               [&](auto &pair) {
                                   return pair.second == ent.rotation;
                               });
            auto ob = find_if(obverse.begin(),
                              obverse.end(),
                              [&](auto &pair) {
                                  return (pair.first == ent.rotation ||
                                          pair.first == ent.rotation.invert());
                              });
            for (auto &next : root) {
                // No need to ever search M -> M'
                if (fwd != inverses.end()
                    && fwd->second == next.rotation) {
                    continue;
                }
                // No need to search M' -> M
                // No need to search M' -> M'; it's the same as M -> M
                if (inv != inverses.end()
                    && (inv->second == next.rotation || inv->first == next.rotation)) {
                    continue;
                }
                // L and L' commute with R and R';
                // canonicalize so we always search R -> L or R' -> L
                // and never L -> R
                // similarly for U/D and F/B
                if (ob != obverse.end()) {
                    auto obinv = find_if(
                            inverses.begin(),
                            inverses.end(),
                            [&](auto &pair) {
                                return pair.first == ob->second;
                            });
                    if (next.rotation == ob->second ||
                        next.rotation == obinv->second) {
                        continue;
                    }
                }
                ent.next->push_back(next);
            }
        }
    }
} tree;

Cube solved;
};

class SearchImpl {
public:
    static int flip_heuristic(const Cube &pos) {
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

    static int edge_heuristic(const Cube &pos) {
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

    static int min_depth(const Cube &pos) {
        return max(flip_heuristic(pos), edge_heuristic(pos));
    }

    template <typename Check, typename Prune, typename Unwind>
    static bool search(const Cube &pos,
                            vector<search_tree::rot> &moves,
                            int depth,
                            const Check &check, const Prune &prune, const Unwind &unwind) {
        if (check(pos, depth)) {
            return true;
        }
        if (depth <= 0) {
            return false;
        }
        if (prune(pos, depth)) {
            return false;
        }
        for (auto &rot: moves) {
            Cube next = pos.apply(rot.rotation);
            if (search(next, *rot.next, depth-1, check, prune, unwind)) {
                unwind(depth, rot.rotation);
                return true;
            }
        }
        return false;
    }

    template <typename Visit>
    static void search(const Cube &pos,
                            vector<search_tree::rot> &moves,
                            int depth,
                            const Visit &visit) {
        search(pos, moves, depth,
               [&](const Cube &pos, int depth) {
                   visit(pos, depth);
                   return false;
               },
               [&](const Cube&, int) { return false; },
               [&](int, const Cube&) {});
    }

    static int heuristic(const Cube &pos) {
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
};

bool search(Cube start, vector<Cube> &path, int max_depth) {
    path.resize(0);
    bool ok = SearchImpl::search(
            start, tree.root, max_depth,
            [&](const Cube &pos, int) {
                return (pos == solved);
            },
            [&](const Cube &pos, int depth) {
                auto edge = SearchImpl::edge_heuristic(pos);
                if (depth < edge)
                    return true;
                auto flip = SearchImpl::flip_heuristic(pos);
                if (depth < flip)
                    return true;
                return false;
            },
            [&](int depth, const Cube &rot) {
                path.push_back(rot);
            });
    if (ok) {
        reverse(path.begin(), path.end());
    }
    return ok;
}

void search_heuristic(int max_depth) {
    vector<vector<int>> heuristic(max_depth + 1);
    for (auto &v : heuristic) {
        v.resize(13, 0);
    }

    SearchImpl::search(
            Cube(), tree.root, max_depth,
            [&](const Cube &pos, int depth) {
                int h = SearchImpl::heuristic(pos);
                ++heuristic[depth][h];
            });

    cout << "heuristic:\n";
    reverse(heuristic.begin(), heuristic.end());

    int i = -1;
    for (auto &depth : heuristic) {
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

vector<pair<string, const Cube&>> moves = {
    {"R", Rotations::R},
    {"R'", Rotations::Rinv},
    {"R2", Rotations::R2},
    {"L", Rotations::L},
    {"L'", Rotations::Linv},
    {"L2", Rotations::L2},
    {"F", Rotations::F},
    {"F'", Rotations::Finv},
    {"F2", Rotations::F2},
    {"B", Rotations::B},
    {"B'", Rotations::Binv},
    {"B2", Rotations::B2},
    {"U", Rotations::U},
    {"U'", Rotations::Uinv},
    {"U2", Rotations::U2},
    {"D", Rotations::D},
    {"D'", Rotations::Dinv},
    {"D2", Rotations::D2},
};

Cube from_algorithm(const string &str) {
    Cube out;
    auto it = str.begin();
    while (it != str.end()) {
        auto next = find(it, str.end(), ' ');
        string word(it, next);
        auto fnd = find_if(moves.begin(),
                           moves.end(),
                           [&](auto &ent) {
                               return ent.first == word;
                           });
        if (fnd == moves.end()) {
            cerr << "unknown move: " << word << "\n";
            abort();
        }
        out = out.apply(fnd->second);
        it = next;
        while (it != str.end() && *it == ' ') {
            ++it;
        }
    }
    return out;
}

string to_algorithm(const vector<Cube> &path) {
    stringstream out;
    for (auto &cube : path) {
        auto fnd = find_if(moves.begin(),
                           moves.end(),
                           [&](auto &ent) {
                               return ent.second == cube;
                           });
        assert(fnd != moves.end());
        if (&cube != &path.front()) {
            out << ' ';
        }
        out << fnd->first;
    }
    return out.str();
}

};
