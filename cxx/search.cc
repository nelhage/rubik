#include "rubik.h"
#include "tables.h"
#include "rubik_impl.h"

#include <vector>
#include <iostream>

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

bool prune_two(const Cube &pos, int depth) {
    auto inv = pos.invert();
    edge_union eu;
    corner_union cu;
    eu.mm = inv.getEdges();
    cu.mm = inv.getCorners();
    auto d = rubik::pair0_dist[(eu.arr[0] << 5) | cu.arr[0]];
    if (d > depth) {
        return true;
    }
    for (auto &p : symmetries) {
        auto c = p.second.apply(inv.apply(p.first));
        eu.mm = c.getEdges();
        cu.mm = c.getCorners();
        if (rubik::pair0_dist[(eu.arr[0] << 5) | cu.arr[0]] > depth) {
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
                return prune_two(pos, depth);
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

};
