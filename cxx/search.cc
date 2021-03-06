#include "rubik.h"
#include "rubik_impl.h"
#include "tables.h"

#include <iostream>
#include <vector>

#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>

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
  auto yaw =
      must_parse("GGGGWGGGGYYYRRRWWWOOOYGYRRRWBWOOOYYYRRRWWWOOOBBBBYBBBB");
  auto pitch =
      must_parse("RRRRWRRRRGGGYYYBBBWWWGGGYRYBBBWOWGGGYYYBBBWWWOOOOYOOOO");
  auto roll =
      must_parse("WWWWWWWWWOOOGGGRRRBBBOGOGRGRBRBOBOOOGGGRRRBBBYYYYYYYYY");

  vector<Cube> symmetries{
      yaw,   yaw.apply(yaw),     yaw.invert(),
      pitch, pitch.apply(pitch), pitch.invert(),
      roll,  roll.apply(roll),   roll.invert(),
  };

  if (debug_mode) {
    for (const auto &s1 : symmetries) {
      for (const auto &s2 : symmetries) {
        if (&s1 == &s2)
          continue;
        if (s1 == s2) {
          cerr << "identical symmetries: " << (&s1 - &symmetries.front()) << "/"
               << (&s2 - &symmetries.front()) << "\n";
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

bool prune_two(const Cube &pos, int depth) __attribute__((used));
bool prune_two(const Cube &pos, int depth) {
  auto inv = pos.invert();
  edge_union eu;
  corner_union cu;
  eu.mm = inv.getEdges();
  cu.mm = inv.getCorners();
  auto d = pair0_dist[(eu.arr[0] << 5) | cu.arr[0]];
  if (d > depth) {
    return true;
  }
  for (auto &p : symmetries) {
    auto c = p.second.apply(inv.apply(p.first));
    eu.mm = c.getEdges();
    cu.mm = c.getCorners();
    if (pair0_dist[(eu.arr[0] << 5) | cu.arr[0]] > depth) {
      return true;
    }
  }
  return false;
}

bool prune_quad(const Cube &pos, int depth) {
  auto inv = pos.invert();
  edge_union eu;
  corner_union cu;
  eu.mm = inv.getEdges();
  cu.mm = inv.getCorners();
  int d = rubik::quad01_dist[(eu.arr[0] << 15) | (eu.arr[1] << 10) |
                             (cu.arr[0] << 5) | (cu.arr[1])];
  assert(d >= 0);
  if (d > depth) {
    return true;
  }
  for (auto &p : symmetries) {
    auto c = p.second.apply(inv.apply(p.first));
    eu.mm = c.getEdges();
    cu.mm = c.getCorners();
    auto d = rubik::quad01_dist[(eu.arr[0] << 15) | (eu.arr[1] << 10) |
                                (cu.arr[0] << 5) | (cu.arr[1])];
    if (d > depth) {
      return true;
    }
  }
  return false;
}

}; // namespace

const vector<pair<Cube, Cube>> symmetries = compute_symmetries();

int flip_heuristic(const Cube &pos) {
  auto mask = _mm_slli_epi16(pos.getEdges(), 3);
  int flipped = __builtin_popcount(_mm_movemask_epi8(mask) & 0x0fff);
  assert(flipped >= 0 && flipped <= 12);
  static const int depths[13] = {
      0, 1, 1, 1, 1, 2, 2, 3, 3, 5, 5, 6, 6,
  };
  return depths[flipped];
}

int edge_heuristic(const Cube &pos) {
  auto mask = _mm_cmpeq_epi8(pos.getEdges(), solved.getEdges());
  int inplace = __builtin_popcount(_mm_movemask_epi8(mask) & 0x0fff);
  int missing = 12 - inplace;
  static const int lookup[13] = {
      0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4,
  };
  return lookup[missing];
}

namespace {
constexpr bool kCollectStats =
#ifdef COLLECT_STATS
    true
#else
    false
#endif
    ;

struct stats {
  using counter = uint64_t;

  counter visit = 0;
  counter prune = 0;
  counter surplus = 0;
};

template <bool collect = kCollectStats> class collect_stats;

template <> class collect_stats<true> {
  stats store;

public:
  void inc(stats::counter stats::*ptr) { ++(store.*ptr); }

  void report(int depth, int ok) {
    cerr << "# search depth=" << depth << " ok=" << ok << "\n";
    cerr << "visit:   " << store.visit << "\n";
    cerr << "prune:   " << store.prune << "\n";
    cerr << "surplus: " << store.surplus << "\n";
  }
};

template <> class collect_stats<false> {
public:
  void inc(stats::counter stats::*ptr) {}
  void report(int depth, int ok) {}
};

} // namespace

bool search(Cube start, vector<Cube> &path, int max_depth) {
  collect_stats<> collect;
  path.resize(0);

  bool ok = search(
      start, *qtm_root, max_depth,
      [&](const Cube &pos, int) {
        collect.inc(&stats::visit);

        return (pos == solved);
      },
      [&](const Cube &pos, int depth) {
        if (prune_quad(pos, depth)) {
          collect.inc(&stats::prune);
          return true;
        };
        return false;
      },
      [&](int depth, const Cube &rot) { path.push_back(rot); });

  collect.report(max_depth, ok);

  if (ok) {
    reverse(path.begin(), path.end());
  }
  return ok;
}

}; // namespace rubik
