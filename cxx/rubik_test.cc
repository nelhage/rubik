#include "catch/catch.hpp"

#include "rubik.h"

#include <string>

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

TEST_CASE("Inver", "[rubik]") {
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
    }
}
