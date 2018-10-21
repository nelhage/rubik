#include "rubik.h"

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

#include <algorithm>
#include <numeric>
#include <vector>

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
    if (this == &rhs) {
        return true;
    }
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
vector<Cube> rotations = {
    Rotations::L,
    Rotations::Linv,
    Rotations::R,
    Rotations::Rinv,
    Rotations::U,
    Rotations::Uinv,
    Rotations::D,
    Rotations::Dinv,
    Rotations::F,
    Rotations::Finv,
    Rotations::B,
    Rotations::Binv,
};

bool search_loop(Cube pos,
                 vector<Cube> &path,
                 int depth,
                 int max_depth) {
    static Cube solved;
    if (pos == solved) {
        path.resize(depth);
        return true;
    }
    if (depth >= max_depth) {
        return false;
    }
    for (auto &rot: rotations) {
        Cube next = pos.apply(rot);
        if (search_loop(next, path, depth+1, max_depth)) {
            path[depth] = rot;
            return true;
        }
    }
    return false;
}
};

bool search(Cube start, vector<Cube> &path, int max_depth) {
    return search_loop(start, path, 0, max_depth);
}

};
