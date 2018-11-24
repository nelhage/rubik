#include "rubik.h"
#include "rubik_impl.h"

#include "absl/strings/str_cat.h"

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

#include <algorithm>
#include <numeric>
#include <vector>
#include <map>

#include <iomanip>
#include <iostream>

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
    edge_union eu, oe;
    corner_union cu, oc;

    eu.mm = _mm_and_si128(edges, _mm_set1_epi8(kEdgePermMask));
    cu.mm = _mm_and_si128(corners, _mm_set1_epi8(kCornerPermMask));

    for (int i = 0; i < 12; ++i) {
        oe.arr[eu.arr[i]] = i;
    }
    oe.mm = _mm_or_si128(
            oe.mm,
            _mm_and_si128(_mm_shuffle_epi8(edges, oe.mm),
                          _mm_set1_epi8(kEdgeAlignMask)));

    for (int i = 0; i < 8; ++i) {
        oc.arr[cu.arr[i]] = i;
    }

    auto rot = _mm_and_si128(_mm_shuffle_epi8(corners, oc.mm),
                             _mm_set1_epi8(kCornerAlignMask));
    auto threes = _mm_set1_epi8(3 << kCornerAlignShift);
    auto zeromask = _mm_cmpeq_epi8(rot, _mm_set1_epi8(0));
    oc.mm = _mm_or_si128(
            oc.mm,
            _mm_andnot_si128(zeromask, _mm_sub_epi8(threes, rot)));
    return Cube(oe.mm, oc.mm);
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
    if (!debug_mode) {
        return;
    }

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
}

namespace {
static constexpr uint8_t E = Cube::kEdgeAlignMask;
static constexpr uint8_t C0 = 0 << Cube::kCornerAlignShift;
static constexpr uint8_t C1 = 1 << Cube::kCornerAlignShift;
static constexpr uint8_t C2 = 2 << Cube::kCornerAlignShift;
};


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
const std::vector<search_node> *make_qtm_tree() {
    static std::vector<search_node>
        l, linv,
        r, rinv,
        u, uinv,
        d, dinv,
        f, finv,
        b, binv;
    static std::vector<search_node>
        root({{Rotations::L, &l},
              {Rotations::Linv, &linv},
              {Rotations::R, &r},
              {Rotations::Rinv, &rinv},
              {Rotations::U, &u},
              {Rotations::Uinv, &uinv},
              {Rotations::D, &d},
              {Rotations::Dinv, &dinv},
              {Rotations::F, &f},
              {Rotations::Finv, &finv},
              {Rotations::B, &b},
              {Rotations::Binv, &binv}});

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
    return &root;
}
};

const vector<search_node> *qtm_root = make_qtm_tree();

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

Result<Cube, Error> from_algorithm(const string &str) {
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
            return Error{"unknown move: " + word};
        }
        out = out.apply(fnd->second);
        it = next;
        while (it != str.end() && *it == ' ') {
            ++it;
        }
    }
    return out;
}

Result<string, Error> to_algorithm(const vector<Cube> &path) {
    stringstream out;
    for (auto &cube : path) {
        auto fnd = find_if(moves.begin(),
                           moves.end(),
                           [&](auto &ent) {
                               return ent.second == cube;
                           });
        if (fnd == moves.end()) {
            return Error{"Unrecognized rotation"};
        }
        if (&cube != &path.front()) {
            out << ' ';
        }
        out << fnd->first;
    }
    return out.str();
}

namespace {
class facelet_parser {
    const map<pair<Color, Color>, uint8_t> edge_map {
        {{Color::Red, Color::Green},       0},
        {{Color::Green, Color::Red},     E|0},
        {{Color::Red, Color::White},       1},
        {{Color::White, Color::Red},     E|1},
        {{Color::Red, Color::Blue},        2},
        {{Color::Blue, Color::Red},      E|2},
        {{Color::Red, Color::Yellow},      3},
        {{Color::Yellow, Color::Red},    E|3},
        {{Color::White, Color::Green},     4},
        {{Color::Green, Color::White},   E|4},
        {{Color::Blue, Color::White},      5},
        {{Color::White, Color::Blue},    E|5},
        {{Color::Blue, Color::Yellow},     6},
        {{Color::Yellow, Color::Blue},   E|6},
        {{Color::Yellow, Color::Green},    7},
        {{Color::Green, Color::Yellow},  E|7},
        {{Color::Orange, Color::Green},    8},
        {{Color::Green, Color::Orange},  E|8},
        {{Color::Orange, Color::White},    9},
        {{Color::White, Color::Orange},  E|9},
        {{Color::Orange, Color::Blue},     10},
        {{Color::Blue, Color::Orange},   E|10},
        {{Color::Orange, Color::Yellow},   11},
        {{Color::Yellow, Color::Orange}, E|11},
            };

    const map<tuple<Color, Color, Color>, uint8_t> corner_map {
        {{Color::Red, Color::Green, Color::White},        0},
        {{Color::Green, Color::White, Color::Red},     C1|0},
        {{Color::White, Color::Red, Color::Green},     C2|0},

        {{Color::Red, Color::White, Color::Blue},         1},
        {{Color::White, Color::Blue, Color::Red},      C1|1},
        {{Color::Blue, Color::Red, Color::White},      C2|1},

        {{Color::Red, Color::Blue, Color::Yellow},        2},
        {{Color::Blue, Color::Yellow, Color::Red},     C1|2},
        {{Color::Yellow, Color::Red, Color::Blue},     C2|2},

        {{Color::Red, Color::Yellow, Color::Green},       3},
        {{Color::Yellow, Color::Green, Color::Red},    C1|3},
        {{Color::Green, Color::Red, Color::Yellow},    C2|3},

        {{Color::Orange, Color::White, Color::Green},     4},
        {{Color::White, Color::Green, Color::Orange},  C1|4},
        {{Color::Green, Color::Orange, Color::White},  C2|4},

        {{Color::Orange, Color::Blue, Color::White},      5},
        {{Color::Blue, Color::White, Color::Orange},   C1|5},
        {{Color::White, Color::Orange, Color::Blue},   C2|5},

        {{Color::Orange, Color::Yellow, Color::Blue},     6},
        {{Color::Yellow, Color::Blue, Color::Orange},  C1|6},
        {{Color::Blue, Color::Orange, Color::Yellow},  C2|6},

        {{Color::Orange, Color::Green, Color::Yellow},    7},
        {{Color::Green, Color::Yellow, Color::Orange}, C1|7},
        {{Color::Yellow, Color::Orange, Color::Green}, C2|7},
            };

    const vector<pair<int, int>> edge_indexes {
        {24, 23},
        {13, 7},
        {26, 27},
        {37, 46},

        {3, 10},
        {16, 5},
        {40, 50},
        {48, 34},

        {32, 21},
        {19, 1},
        {30, 29},
        {43, 52},
            };

    const vector<tuple<int, int, int>> corner_indexes {
        {12, 11, 6},
        {14, 8, 15},
        {38, 39, 47},
        {36, 45, 35},

        {20, 0, 9},
        {18, 17, 2},
        {42, 53, 41},
        {44, 33, 51},
            };

    const vector<pair<int, Color>> centers {
        {4, Color::White},
        {22, Color::Green},
        {25, Color::Red},
        {28, Color::Blue},
        {31, Color::Orange},
        {49, Color::Yellow},
            };
public:

    facelet_parser() {
        if (debug_mode) {
            return;
        }

        array<int, 6*9> indices;
        fill(indices.begin(), indices.end(), 0);

        const auto &visit = [&](int idx) {
            if (idx >= (int)indices.size()) {
                cerr << "index out of range: " << idx << "\n";
                abort();
            }
            if (indices[idx]) {
                cerr << "duplicate index: " << idx << "\n";
                abort();
            }
            indices[idx] = 1;
        };

        for (auto p : edge_indexes) {
            visit(p.first);
            visit(p.second);

        }
        for (auto p : corner_indexes) {
            visit(get<0>(p));
            visit(get<1>(p));
            visit(get<2>(p));
        }
        for (auto p : centers) {
            visit(p.first);
        }
        for (auto &i : indices) {
            if (!i) {
                cerr << "missing index: " << &i - &indices.front() << "\n";
                abort();
            }
        }
    }

    Result<Cube, Error> parse(const std::string &str) {
        if (str.size() != 6*9) {
            return Error{"Wrong string size: " + str.size()};
        }
        int i = 0;
        for (auto c : centers) {
            if ((Color)str[c.first] != c.second) {
                return Error{
                    absl::StrCat("Wrong ordering: side ", i, " should be `",
                                 string(1, (char)c.second), "', got `",
                                 string(1, str[c.first]), "'")
                        };
            }
            ++i;
        }
        edge_union eu;
        corner_union cu;
        i = 0;
        for (auto e : edge_indexes) {
            auto cs = make_pair((Color)str[e.first], (Color)str[e.second]);
            auto fnd = edge_map.find(cs);
            if (fnd == edge_map.end()) {
                return Error {
                    absl::StrCat("Can't find edge: ",
                                 string(&str[e.first], 1), "/", string(&str[e.second], 1),
                                 " (index ", e.first, "/", e.second, ")")
                        };
            }
            eu.arr[i++] = fnd->second;
        }

        i = 0;
        for (auto c : corner_indexes) {
            auto cs = make_tuple(
                    (Color)str[get<0>(c)],
                    (Color)str[get<1>(c)],
                    (Color)str[get<2>(c)]);
            auto fnd = corner_map.find(cs);
            if (fnd == corner_map.end()) {
                return Error {
                    absl::StrCat("Can't find corner: ",
                                 string(&str[get<0>(c)], 1), "/",
                                 string(&str[get<1>(c)], 1), "/",
                                 string(&str[get<2>(c)], 1),
                                 " (index ",
                                 get<0>(c), "/",
                                 get<1>(c), "/",
                                 get<2>(c),
                                 ")")
                        };
            }
            cu.arr[i++] = fnd->second;
        }

        return Cube(eu.mm, cu.mm);
    }
};

};

Result<Cube, Error> from_facelets(const std::string &str) {
    static facelet_parser parser;
    return parser.parse(str);
}


};
