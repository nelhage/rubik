#include "rubik.h"

#include <algorithm>
#include <numeric>
#include <vector>

#include <cassert>

using namespace std;

namespace rubik {

Cube::Cube() {
    iota(edges.begin(), edges.end(), 0);
    iota(corners.begin(), corners.end(), 0);
    sanityCheck();
}

Cube::Cube(array<uint8_t, 12> edges, array<uint8_t, 8> corners)
        : edges(edges), corners(corners) {
    sanityCheck();
}

Cube Cube::apply(const Cube &other) const {
    array<uint8_t, 12> out_edges;
    array<uint8_t, 8> out_corners;
    for (int i = 0; i < 12; i++) {
        out_edges[i] = edges[other.edges[i] & kEdgePermMask];
        out_edges[i] ^= other.edges[i] & kEdgeAlignMask;
    }
    for (int i = 0; i < 8; i++) {
        out_corners[i] = corners[other.corners[i] & kCornerPermMask];
        out_corners[i] += other.corners[i] & kCornerAlignMask;
        if ((out_corners[i] >> kCornerAlignShift) >= 3) {
            out_corners[i] -= (3 << kCornerAlignShift);
        }
    }
    return Cube(out_edges, out_corners);
}

Cube Cube::invert() const {
    array<uint8_t, 12> out_edges;
    array<uint8_t, 8> out_corners;
    for (int i = 0; i < 12; ++i) {
        auto idx = edges[i] & kEdgePermMask;
        out_edges[idx] = i;
    }
    for (int i = 0; i < 12; ++i) {
        auto idx = out_edges[i] & kEdgePermMask;
        out_edges[i] ^= edges[idx] & kEdgeAlignMask;
    }
    for (int i = 0; i < 8; ++i) {
        auto idx = corners[i] & kCornerPermMask;
        out_corners[idx] = i;
    }
    for (int i = 0; i < 8; ++i) {
        auto idx = out_corners[i] & kCornerPermMask;
        uint8_t align = (3 - ((corners[idx] & kCornerAlignMask) >> kCornerAlignShift)) % 3;
        out_corners[i] |= align << kCornerAlignShift;
    }
    return Cube(out_edges, out_corners);
}

bool Cube::operator==(const Cube &rhs) const {
    if (this == &rhs) {
        return true;
    }
    return edges == rhs.edges && corners == rhs.corners;
}

void Cube::sanityCheck() const {
#ifndef NDEBUG
    array<uint8_t, 12> edge_perms;
    edge_perms.fill(0);
    array<uint8_t, 8> corner_perms;
    corner_perms.fill(0);

    for (auto e : edges) {
        assert((e & ~(kEdgePermMask|kEdgeAlignMask)) == 0);
        assert((e & kEdgePermMask) < 12);
        edge_perms[e & kEdgePermMask]++;
    }
    for (auto c : corners) {
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
