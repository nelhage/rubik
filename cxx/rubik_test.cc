#define CATCH_CONFIG_MAIN
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
        REQUIRE(Cube().apply(tc.rot) == tc.rot);
        REQUIRE(tc.rot.apply(Cube()) == tc.rot);
        REQUIRE(Cube() == tc.rot.apply(tc.rot).apply(tc.rot).apply(tc.rot));
    }
}
