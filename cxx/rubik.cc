#include "rubik.h"

#include <algorithm>
#include <numeric>
#include <vector>

#include <cassert>

using namespace std;

namespace rubik {

Cube::Cube() {
    iota(store.edges.begin(), store.edges.end(), 0);
    iota(store.corners.begin(), store.corners.end(), 0);
    sanityCheck();
}

Cube::Cube(std::array<uint8_t, 12> edges, std::array<uint8_t, 8> corners) {
    store.edges = edges;
    store.corners = corners;
    sanityCheck();
}

Cube::Cube(storage store) : store(store) {
    sanityCheck();
}

constexpr uint64_t broadcast(uint8_t bits) {
    return bits * 0x0101010101010101;
}

Cube Cube::apply(const Cube &other) const {
    Cube::storage out;

    for (int i = 0; i < 12; i++) {
        out.edges[i] = store.edges[other.store.edges[i] & kEdgePermMask];
    }
    out.edge_bits.low  ^= other.store.edge_bits.low  & broadcast(kEdgeAlignMask);
    out.edge_bits.high ^= other.store.edge_bits.high & broadcast(kEdgeAlignMask);

    for (int i = 0; i < 8; i++) {
        out.corners[i] = store.corners[other.store.corners[i] & kCornerPermMask];
    }

    out.corner_bits += (other.store.corner_bits & broadcast(kCornerAlignMask));
    uint64_t overflow = out.corner_bits & broadcast(kCornerAlignMask);
    overflow &= overflow >> 1;
    out.corner_bits -= (overflow | (overflow << 1));

    return out;
}

Cube Cube::invert() const {
    Cube::storage out;

    for (int i = 0; i < 12; ++i) {
        auto idx = store.edges[i] & kEdgePermMask;
        out.edges[idx] = i;
    }
    for (int i = 0; i < 12; ++i) {
        auto idx = out.edges[i] & kEdgePermMask;
        out.edges[i] ^= store.edges[idx] & kEdgeAlignMask;
    }
    for (int i = 0; i < 8; ++i) {
        auto idx = store.corners[i] & kCornerPermMask;
        out.corners[idx] = i;
    }
    for (int i = 0; i < 8; ++i) {
        auto idx = out.corners[i] & kCornerPermMask;
        uint8_t align = (3 - ((store.corners[idx] & kCornerAlignMask) >> kCornerAlignShift)) % 3;
        out.corners[i] |= align << kCornerAlignShift;
    }
    return out;
}

bool Cube::operator==(const Cube &rhs) const {
    if (this == &rhs) {
        return true;
    }
    return store.edges == rhs.store.edges && store.corners == rhs.store.corners;
}

void Cube::sanityCheck() const {
#ifndef NDEBUG
    array<uint8_t, 12> edge_perms;
    edge_perms.fill(0);
    array<uint8_t, 8> corner_perms;
    corner_perms.fill(0);

    for (auto e : store.edges) {
        assert((e & ~(kEdgePermMask|kEdgeAlignMask)) == 0);
        assert((e & kEdgePermMask) < 12);
        edge_perms[e & kEdgePermMask]++;
    }
    for (auto c : store.corners) {
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

};
