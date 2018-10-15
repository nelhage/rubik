#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "rubik.h"

using namespace rubik;

TEST_CASE("Default cube is constructible", "[rubik]") {
    Cube cube;
}
