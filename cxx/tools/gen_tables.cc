#include <assert.h>
#include <iostream>
#include <vector>

#include "rubik.h"
#include "rubik_impl.h"

#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>

using namespace std;
using namespace rubik;

namespace rubik {
array<int8_t, 32 * 32> corner_dist;
array<int8_t, 32 * 32> edge_dist;
array<int8_t, 32 * 32> pair0_dist;
array<int8_t, 32 * 32 * 32 * 32> quad01_dist;
}; // namespace rubik

void floyd_warshall(size_t n, array<int8_t, 32 * 32> &grid) {
  for (size_t k = 0; k < n; ++k) {
    for (size_t i = 0; i < n; ++i) {
      for (size_t j = 0; j < n; ++j) {
        grid[i * n + j] =
            min<int8_t>(grid[i * n + j], grid[i * n + k] + grid[k * n + j]);
      }
    }
  }
}

const int8_t kInfinity = 50;

void render(const string &name, int8_t *vals, size_t n) {
  cout << "std::array<int8_t, " << n << "> " << name << " {\n";
  for (size_t i = 0; i < n;) {
    cout << "    ";
    for (int j = 0; j < 32 && i < n; j++, i++) {
      auto v = vals[i];
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

template <size_t n> void render(const string &name, array<int8_t, n> &vals) {
  render(name, &vals[0], n);
}

void compute_edge_dist(vector<Cube> &all_moves) {
  fill(edge_dist.begin(), edge_dist.end(), kInfinity);
  for (const auto &m : all_moves) {
    rubik::edge_union eu;
    eu.mm = m.getEdges();
    for (uint i = 0; i < eu.arr.size(); ++i) {
      for (int a = 0; a < 2; a++) {
        uint from = (a << rubik::Cube::kEdgeAlignShift) | i;
        uint to = eu.arr[i] ^ (a << rubik::Cube::kEdgeAlignShift);
        edge_dist[from * 32 + to] = 1;
      }
    }
  }
  for (int i = 0; i < 32; ++i) {
    edge_dist[i * 32 + i] = 0;
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
        uint to = cu.arr[i];
        to += (a << rubik::Cube::kCornerAlignShift);
        while (to >= (3 << rubik::Cube::kCornerAlignShift)) {
          to -= (3 << rubik::Cube::kCornerAlignShift);
        }
        assert(from < 32);
        assert(to < 32);
        corner_dist[from * 32 + to] = 1;
      }
    }
  }
  for (int i = 0; i < 32; ++i) {
    corner_dist[i * 32 + i] = 0;
  }
  floyd_warshall(32, corner_dist);
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
          if (prefix_prune(pos, n, depth)) {
            return true;
          }
          if (n >= 2) {
            auto inv = pos.invert();
            edge_union eu;
            corner_union cu;
            eu.mm = inv.getEdges();
            cu.mm = inv.getCorners();
            auto d = pair0_dist[(eu.arr[0] << 5) | cu.arr[0]];
            if (d > depth) {
              return true;
            }
          }
          return false;
        },
        [&](int, const Cube &) {});
    if (ok) {
      return depth;
    }
  }
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
    for (auto c : all_corner) {
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

void compute_quad01_dist() {
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

  fill(quad01_dist.begin(), quad01_dist.end(), kInfinity);

  for (auto e0 : all_edge) {
    for (auto e1 : all_edge) {
      if ((e0 & rubik::Cube::kEdgePermMask) ==
          (e1 & rubik::Cube::kEdgePermMask)) {
        continue;
      }
      for (auto c0 : all_corner) {
        for (auto c1 : all_corner) {
          if ((c0 & rubik::Cube::kCornerPermMask) ==
              (c1 & rubik::Cube::kCornerPermMask)) {
            continue;
          }

          rubik::edge_union eu;
          rubik::corner_union cu;
          eu.mm = solved.getEdges();
          cu.mm = solved.getCorners();

          eu.arr[e0 & rubik::Cube::kEdgePermMask] = 0;
          eu.arr[0] = e0;

          int idx = e1 & rubik::Cube::kEdgePermMask;
          if (idx == 0) {
            idx = e0 & rubik::Cube::kEdgePermMask;
          }
          eu.arr[idx] = eu.arr[1];
          eu.arr[1] = e1;

          cu.arr[c0 & rubik::Cube::kCornerPermMask] = 0;
          cu.arr[0] = c0;

          idx = c1 & rubik::Cube::kCornerPermMask;
          if (idx == 0) {
            idx = c0 & rubik::Cube::kCornerPermMask;
          }
          cu.arr[idx] = cu.arr[1];
          cu.arr[1] = c1;

          Cube pos(eu.mm, cu.mm);
          int d = prefix_search(pos.invert(), 2);
          quad01_dist[(e0 << 15) | (e1 << 10) | (c0 << 5) | c1] = d;
          // cerr << "(" << (int)e0 << "," << (int)e1 << "," << (int)c0 << ","
          // << (int)c1 << "): " << d << " (bound: " <<
          // (int)rubik::pair0_dist[(e0<<5)|c0] << ")\n";
        }
      }
      cerr << "." << std::flush;
    }
    cerr << "\n";
  }
}

int main(int argc, char **argv) {
  bool do_quad = false;
  if (argc == 2 && string(argv[1]) == "--quad") {
    do_quad = true;
  }

  vector<rubik::Cube> moves;
  for (auto &node : *qtm_root) {
    moves.emplace_back(node.rotation);
  }

  compute_edge_dist(moves);
  compute_corner_dist(moves);
  compute_pair0_dist();

  if (do_quad) {
    compute_quad01_dist();
  }

  cout << "#include \"tables.h\"\n";
  cout << "\n";
  cout << "namespace rubik {\n";
  if (do_quad) {
    render("quad01_dist", quad01_dist);
  } else {
    render("edge_dist", edge_dist);
    render("corner_dist", corner_dist);
    render("pair0_dist", pair0_dist);
  }
  cout << "}\n";

  return 0;
}
