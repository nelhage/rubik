#include "catch/catch.hpp"

#include "rubik.h"
#include "rubik_impl.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace rubik;
using namespace std;

namespace {
Rotations rotations;
struct {
  string name;
  Cube rot;
} named_rotations[] = {
    {"L", rotations.L}, {"R", rotations.R}, {"U", rotations.U},
    {"D", rotations.D}, {"F", rotations.F}, {"B", rotations.B},
};
}; // namespace

TEST_CASE("Default cube is constructible", "[rubik]") { Cube cube; }

TEST_CASE("Rotations", "[rubik]") {
  for (auto &tc : named_rotations) {
    INFO("Checking rotations: " << tc.name);
    CHECK(Cube().apply(tc.rot) == tc.rot);
    CHECK(tc.rot.apply(Cube()) == tc.rot);
    CHECK(Cube() == tc.rot.apply(tc.rot).apply(tc.rot).apply(tc.rot));
  }
}

TEST_CASE("Other rotations", "[rubik]") {
  Cube cu;
  cu.apply(rotations.L).apply(rotations.F);
}

TEST_CASE("Invert", "[rubik]") {
  for (auto &tc : named_rotations) {
    INFO("Checking inversions: " << tc.name);
    CHECK(tc.rot.apply(tc.rot.invert()) == Cube());
    CHECK(tc.rot.invert().apply(tc.rot) == Cube());
    CHECK(tc.rot.apply(tc.rot) == tc.rot.invert().apply(tc.rot.invert()));
    auto inv = tc.rot.invert();
    CHECK(inv.apply(inv).apply(inv) == tc.rot);
  }
}

Cube superflip() {
  return get<Cube>(rubik::from_algorithm(
      "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2"));
}

TEST_CASE("Cube::operator==", "[rubik]") { REQUIRE(superflip() != Cube()); }

TEST_CASE("from_facelets", "[rubik]") {
  CHECK(absl::holds_alternative<rubik::Error>(rubik::from_facelets("")));
  CHECK(absl::holds_alternative<rubik::Error>(rubik::from_facelets(
      "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR")));
  auto c = rubik::from_facelets(
      "WWWWWWWWWGGGRRRBBBOOOGGGRRRBBBOOOGGGRRRBBBOOOYYYYYYYYY");
  if (!absl::holds_alternative<rubik::Cube>(c)) {
    FAIL("error: " << get<rubik::Error>(c).error);
  }
  CHECK(get<Cube>(c) == Cube());
}

const rubik::search_node *find_node(const std::vector<search_node> *nodes,
                                    const Cube &rot) {
  auto fnd = find_if(nodes->begin(), nodes->end(),
                     [&](auto &node) { return node.rotation == rot; });
  if (fnd == nodes->end()) {
    return nullptr;
  }
  return &*fnd;
}

TEST_CASE("qtm_tree", "[rubik]") {
  CHECK(find_node(find_node(qtm_root, rotations.L)->next, rotations.Linv) ==
        nullptr);
  CHECK(find_node(find_node(qtm_root, rotations.Linv)->next, rotations.L) ==
        nullptr);
  CHECK(find_node(find_node(qtm_root, rotations.L)->next, rotations.L) !=
        nullptr);

  CHECK(find_node(find_node(qtm_root, rotations.R)->next, rotations.L) !=
        nullptr);
  CHECK(find_node(find_node(qtm_root, rotations.R)->next, rotations.Linv) !=
        nullptr);
  CHECK(find_node(find_node(qtm_root, rotations.L)->next, rotations.R) ==
        nullptr);
  CHECK(find_node(find_node(qtm_root, rotations.L)->next, rotations.Rinv) ==
        nullptr);

  vector<pair<Cube, string>> d2;
  for (auto &node : *qtm_root) {
    for (auto &next : *node.next) {
      vector<Cube> path{node.rotation, next.rotation};
      d2.emplace_back(make_pair(node.rotation.apply(next.rotation),
                                get<string>(to_algorithm(path))));
    }
  }
  for (auto &p1 : d2) {
    for (auto &p2 : d2) {
      if (&p1 == &p2)
        continue;
      INFO("duplicate len2 path first=" << p1.second << " snd=" << p2.second);
      CHECK(p1.first != p2.first);
    }
  }
}

TEST_CASE("Search", "[rubik]") {
  struct {
    string in;
    int depth;
    bool ok;
    vector<Cube> out;
  } tests[] = {
      {
          "R",
          1,
          true,
          {rotations.Rinv},
      },
      {
          "R U",
          1,
          false,
          {},
      },
      {
          "R U",
          2,
          true,
          {rotations.Uinv, rotations.Rinv},
      },
      {
          "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2",
          4,
          false,
          {},
      },
      {
          "R U' B",
          4,
          true,
          {
              rotations.Binv,
              rotations.U,
              rotations.Rinv,
          },
      },
      {
          "R L",
          2,
          true,
          {rotations.Rinv, rotations.Linv},
      },
      {
          "R2",
          4,
          true,
          {rotations.R, rotations.R},
      },
  };
  for (auto &tc : tests) {
    INFO("search(\"" << tc.in << "\", " << tc.depth << ")");
    vector<Cube> path;
    Cube in = get<Cube>(from_algorithm(tc.in));
    bool ok = search(in, path, tc.depth);
    CHECK(ok == tc.ok);
    INFO("-> " << get<string>(to_algorithm(path)));
    CHECK(equal(tc.out.begin(), tc.out.end(), path.begin(), path.end()));
  }
}

/*
  Sadly, this requires googletest

TEST_CASE("Hashing", "[rubik]") {
    CHECK(absl::VerifyTypeImplementsAbslHashCorrectly({
              Cube(),
              get<Cube>(from_algorithm("L R L' R'")),
              rotations.L,
              rotations.Linv,
    }));
}
*/
