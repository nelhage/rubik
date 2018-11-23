#include "catch/catch.hpp"

#include "rubik.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
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
    return get<Cube>(rubik::from_algorithm("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2"));
}

TEST_CASE("Cube::operator==", "[rubik]") {
    REQUIRE(superflip() != Cube());
}

TEST_CASE("Search", "[rubik]") {
    struct {
        string in;
        int depth;
        bool ok;
        vector<Cube> out;
    } tests[] = {
        {
            "R", 1,
            true, {Rotations::Rinv},
        },
        {
            "R U", 1,
            false, {},
        },
        {
            "R U", 2,
            true,
            {Rotations::Uinv, Rotations::Rinv},
        },
        {
            "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", 4,
            false, {},
        },
        {
            "R U' B", 4,
            true,
            {
                Rotations::Binv,
                Rotations::U,
                Rotations::Rinv,
            },
        },
        {
            "R L", 2,
            true,
            {Rotations::Rinv, Rotations::Linv},
        },
        {
            "R2", 4,
            true,
            {Rotations::R, Rotations::R},
        },
    };
    for (auto &tc: tests) {
        INFO("search(\"" << tc.in << "\", " << tc.depth << ")");
        vector<Cube> path;
        Cube in = get<Cube>(from_algorithm(tc.in));
        bool ok = search(in, path, tc.depth);
        CHECK(ok == tc.ok);
        INFO("-> " << get<string>(to_algorithm(path)));
        CHECK(equal(tc.out.begin(), tc.out.end(),
                    path.begin(), path.end()));
    }
}
