#include "rubik.h"
#include "tables.h"
#include "rubik_impl.h"

using namespace std;

namespace rubik {

Cube solved;

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

};
