#include "catch/catch.hpp"

#include "rubik.h"

#include <string>
#include <algorithm>

using namespace rubik;
using namespace std;

TEST_CASE("Default cube is constructible", "[rubik]") {
    Cube cube;
}

TEST_CASE("Rotations", "[rubik]") {
    struct {
        string name;
        Cube rot;
    } rotations[] = {
        {"L", Rotations::L},
        {"R", Rotations::R},
        {"U", Rotations::U},
        {"D", Rotations::D},
        {"F", Rotations::F},
        {"B", Rotations::B},
    };
    for (auto &tc : rotations) {
        INFO("Checking rotations: " << tc.name);
        CHECK(Cube().apply(tc.rot) == tc.rot);
        CHECK(tc.rot.apply(Cube()) == tc.rot);
        CHECK(Cube() == tc.rot.apply(tc.rot).apply(tc.rot).apply(tc.rot));
    }
}

TEST_CASE("Other rotations", "[rubik]") {
    Cube cu;
    cu.apply(Rotations::L).apply(Rotations::F);
}

TEST_CASE("Invert", "[rubik]") {
    struct {
        string name;
        Cube rot;
    } rotations[] = {
        {"L", Rotations::L},
        {"R", Rotations::R},
        {"U", Rotations::U},
        {"D", Rotations::D},
        {"F", Rotations::F},
        {"B", Rotations::B},
    };
    for (auto &tc : rotations) {
        INFO("Checking inversions: " << tc.name);
        CHECK(tc.rot.apply(tc.rot.invert()) == Cube());
        CHECK(tc.rot.invert().apply(tc.rot) == Cube());
        CHECK(tc.rot.apply(tc.rot) ==
              tc.rot.invert().apply(tc.rot.invert()));
        auto inv = tc.rot.invert();
        CHECK(inv.apply(inv).apply(inv) == tc.rot);
    }
}

Cube superflip() {
    return Cube().
        apply(Rotations::U)
        .apply(Rotations::R2)
        .apply(Rotations::F)
        .apply(Rotations::B)
        .apply(Rotations::R)
        .apply(Rotations::B2)
        .apply(Rotations::R)
        .apply(Rotations::U2)
        .apply(Rotations::L)
        .apply(Rotations::B2)
        .apply(Rotations::R)
        .apply(Rotations::Uinv)
        .apply(Rotations::Dinv)
        .apply(Rotations::R2)
        .apply(Rotations::F)
        .apply(Rotations::Rinv)
        .apply(Rotations::L)
        .apply(Rotations::B2)
        .apply(Rotations::U2)
        .apply(Rotations::F2);
}

TEST_CASE("Cube::operator==", "[rubik]") {
    REQUIRE(superflip() != Cube());
}

TEST_CASE("Search", "[rubik]") {
    struct {
        Cube in;
        int depth;
        bool ok;
        vector<Cube> out;
    } tests[] = {
        {
            Rotations::R, 1,
            true, {Rotations::Rinv},
        },
        {
            Rotations::R.apply(Rotations::U), 1,
            false, {},
        },
        {
            Rotations::R.apply(Rotations::U), 2,
            true,
            {Rotations::Uinv, Rotations::Rinv},
        },
        {
            superflip(), 2,
            false, {},
        }
    };
    for (auto &tc: tests) {
        vector<Cube> path;
        bool ok = search(tc.in, path, tc.depth);
        CHECK(ok == tc.ok);
        CHECK(equal(tc.out.begin(), tc.out.end(),
                    path.begin(), path.end()));
    }
}
