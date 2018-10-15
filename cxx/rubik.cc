#include "rubik.h"

#include <algorithm>
#include <vector>

#include <cassert>

using namespace std;

namespace rubik {

Cube::Cube() {
    for (uint8_t i = 0; i < 12; i ++) {
        edges[i] = i;
    }
    for (uint8_t i = 0; i < 8; i ++) {
        corners[i] = i;
    }
    sanityCheck();
}

Cube::Cube(uint8_t edges[12], uint8_t corners[8]) {
    std::copy(edges, &edges[12], this->edges);
    std::copy(corners, &corners[12], this->corners);
    sanityCheck();
}

void Cube::sanityCheck() const {
    vector<uint8_t> edge_perms;
    vector<uint8_t> corner_perms;

    for (int i = 0; i < 12; ++i) {
        assert((edges[i] & ~(kEdgePermMask|kEdgeAlignMask)) == 0);
        assert((edges[i] & kEdgePermMask) < 12);
        edge_perms.push_back(edges[i] & kEdgePermMask);
    }
    for (int i = 0; i < 8; ++i) {
        assert((corners[i] & ~(kCornerPermMask|kCornerAlignMask)) == 0);
        assert((corners[i] & kCornerPermMask) < 12);
        corner_perms.push_back(corners[i] & kCornerPermMask);
    }

    sort(edge_perms.begin(), edge_perms.end());
    sort(corner_perms.begin(), corner_perms.end());
    for (int i = 0; i < 12; i++) {
        assert(edge_perms[i] == i);
    }
    for (int i = 0; i < 8; i++) {
        assert(corner_perms[i] == i);
    }
}
};
