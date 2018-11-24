#include <iostream>
#include <vector>
#include <assert.h>

#include "rubik.h"
#include "rubik_impl.h"

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

using namespace std;
using namespace rubik;

namespace rubik {
array<int8_t, 32*32> corner_dist;
array<int8_t, 32*32> edge_dist;
array<int8_t, 32*32> pair0_dist;
};

void floyd_warshall(size_t n, array<int8_t, 32*32> &grid) {
    for (size_t k = 0; k < n; ++k) {
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                grid[i*n+j] = min<int8_t>(grid[i*n+j], grid[i*n+k] + grid[k*n+j]);
            }
        }
    }
}

const int8_t kInfinity = 50;

void render(const string &name, array<int8_t, 32*32> &vals) {
    cout << "std::array<int8_t, 32*32> " << name << " {\n";
    for (int i = 0; i < 32; i++) {
        cout << "    ";
        for (int j = 0; j < 32; j++) {
            auto v = vals[i*32+j];
            if (v >= kInfinity) {
                cout << "-1";
            } else {
                cout << (int)v;
            }
            cout << ",";
            if (j != 31) {
                cout << ' ';
            }
        }
        cout << "\n";
    }
    cout << "};\n";
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
                    return prefix_prune(pos, n, depth);
                },
                [&](int, const Cube &) {});
        if (ok) {
            return depth;
        }
    }
}

void compute_edge_dist(vector<Cube> &all_moves) {
    fill(edge_dist.begin(), edge_dist.end(), kInfinity);
    for (const auto &m : all_moves) {
        rubik::edge_union eu;
        eu.mm = m.getEdges();
        for (uint i = 0; i < eu.arr.size(); ++i) {
            for (int a = 0; a < 2; a++) {
                uint from = (a << rubik::Cube::kEdgeAlignShift) | i;
                uint to   = eu.arr[i] ^ (a << rubik::Cube::kEdgeAlignShift);
                edge_dist[from*32 + to] = 1;
            }
        }
    }
    for (int i = 0; i < 32; ++i) {
        edge_dist[i*32 + i] = 0;
    }
    floyd_warshall(32, edge_dist);
}

void compute_corner_dist(vector<Cube> &all_moves) {
    fill(corner_dist.begin(), corner_dist.end(), kInfinity);
    for (const auto &m : all_moves) {
        rubik::corner_union cu;
        cu.mm = m.getCorners();
        for (uint i = 0; i < cu.arr.size(); ++i) {
            for (int a = 0; a < 3; a++) {
                uint from = (a << rubik::Cube::kCornerAlignShift) | i;
                uint to   = cu.arr[i];
                to += (a << rubik::Cube::kCornerAlignShift);
                while (to >= (3 << rubik::Cube::kCornerAlignShift)) {
                    to -= (3 << rubik::Cube::kCornerAlignShift);
                }
                assert(from < 32);
                assert(to < 32);
                corner_dist[from*32 + to] = 1;
            }
        }
    }
    for (int i = 0; i < 32; ++i) {
        corner_dist[i*32 + i] = 0;
    }
    floyd_warshall(32, corner_dist);
}

void compute_pair0_dist() {
    Cube solved;

    vector<int8_t> all_edge;
    vector<int8_t> all_corner;
    for (int e = 0; e < 12; ++e) {
        for (int ea = 0; ea < 2; ++ea) {
            all_edge.push_back((ea << rubik::Cube::kEdgeAlignShift) | e);
        }
    }
    for (int c = 0; c < 8; ++c) {
        for (int ca = 0; ca < 3; ++ca) {
            all_corner.push_back((ca << rubik::Cube::kCornerAlignShift) | c);
        }
    }


    for (auto e : all_edge) {
        for (auto c: all_corner) {
            rubik::edge_union eu;
            rubik::corner_union cu;
            eu.mm = solved.getEdges();
            cu.mm = solved.getCorners();

            eu.arr[e & rubik::Cube::kEdgePermMask] = 0;
            eu.arr[0] = e;
            cu.arr[c & rubik::Cube::kCornerPermMask] = 0;
            cu.arr[0] = c;

            Cube pos(eu.mm, cu.mm);
            int d = prefix_search(pos.invert(), 1);
            pair0_dist[(e << 5) | c] = d;
        }
    }
}

int main() {
    vector<rubik::Cube> moves{
        rubik::Rotations::L,
        rubik::Rotations::R,
        rubik::Rotations::U,
        rubik::Rotations::D,
        rubik::Rotations::F,
        rubik::Rotations::B,
        rubik::Rotations::Linv,
        rubik::Rotations::Rinv,
        rubik::Rotations::Uinv,
        rubik::Rotations::Dinv,
        rubik::Rotations::Finv,
        rubik::Rotations::Binv,
    };

    compute_edge_dist(moves);
    compute_corner_dist(moves);
    compute_pair0_dist();

    cout << "#include \"tables.h\"\n";
    cout << "\n";
    cout << "namespace rubik {\n";
    render("edge_dist", edge_dist);
    render("corner_dist", corner_dist);
    render("pair0_dist", pair0_dist);
    cout << "}\n";

    return 0;
}
