#include <iostream>
#include <vector>
#include <assert.h>

#include "rubik.h"
#include "rubik_impl.h"

using namespace std;

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

namespace rubik {
array<int8_t, 32*32> corner_dist;
array<int8_t, 32*32> edge_dist;
};

using namespace rubik;

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

    fill(edge_dist.begin(), edge_dist.end(), kInfinity);
    for (const auto &m : moves) {
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
    fill(corner_dist.begin(), corner_dist.end(), kInfinity);
    for (const auto &m : moves) {
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
        edge_dist[i*32 + i] = 0;
        corner_dist[i*32 + i] = 0;
    }

    cout << "#include \"tables.h\"\n";
    cout << "\n";
    cout << "namespace rubik {\n";
    floyd_warshall(32, edge_dist);
    render("edge_dist", edge_dist);
    floyd_warshall(32, corner_dist);
    render("corner_dist", corner_dist);
    cout << "}\n";

    return 0;
}
