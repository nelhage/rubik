#include <iostream>
#include <vector>
#include <assert.h>

#include "rubik.h"
#include "rubik_impl.h"

using namespace std;

void floyd_warshall(size_t n, int32_t *grid) {
    for (size_t k = 0; k < n; ++k) {
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                grid[i*n+j] = min(grid[i*n+j], grid[i*n+k] + grid[k*n+j]);
            }
        }
    }
}

void render(const string &name, array<int32_t, 32*32> &vals) {
    cout << "int8_t " << name << "[] = {\n";
    for (int i = 0; i < 32; i++) {
        cout << "    ";
        for (int j = 0; j < 32; j++) {
            auto v = vals[i*32+j];
            if (v == 1000) {
                cout << "-1";
            } else {
                cout << v;
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

    array<int32_t, 32*32> edges;
    fill(edges.begin(), edges.end(), 1000);
    for (const auto &m : moves) {
        rubik::edge_union eu;
        eu.mm = m.getEdges();
        for (uint i = 0; i < eu.arr.size(); ++i) {
            for (int a = 0; a < 2; a++) {
                uint from = (a << rubik::Cube::kEdgeAlignShift) | i;
                uint to   = eu.arr[i] ^ (a << rubik::Cube::kEdgeAlignShift);
                edges[from*32 + to] = 1;
            }
        }
    }
    array<int32_t, 32*32> corners;
    fill(corners.begin(), corners.end(), 1000);
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
                corners[from*32 + to] = 1;
            }
        }
    }
    for (int i = 0; i < 32; ++i) {
        edges[i*32 + i] = 0;
        corners[i*32 + i] = 0;
    }

    cout << "#include \"tables.h\"\n";
    cout << "\n";
    cout << "namespace rubik {\n";
    floyd_warshall(32, &edges[0]);
    render("edge_dist", edges);
    floyd_warshall(32, &corners[0]);
    render("corner_dist", corners);
    cout << "}\n";

    return 0;
}
