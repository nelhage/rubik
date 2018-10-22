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

vector<pair<string, const Cube&>> moves = {
    {"R", Rotations::R},
    {"R'", Rotations::Rinv},
    {"R2", Rotations::R2},
    {"L", Rotations::L},
    {"L'", Rotations::Linv},
    {"L2", Rotations::L2},
    {"F", Rotations::F},
    {"F'", Rotations::Finv},
    {"F2", Rotations::F2},
    {"B", Rotations::B},
    {"B'", Rotations::Binv},
    {"B2", Rotations::B2},
    {"U", Rotations::U},
    {"U'", Rotations::Uinv},
    {"U2", Rotations::U2},
    {"D", Rotations::D},
    {"D'", Rotations::Dinv},
    {"D2", Rotations::D2},
};

Cube from_algorithm(const string &str) {
    Cube out;
    auto it = str.begin();
    while (it != str.end()) {
        auto next = find(it, str.end(), ' ');
        string word(it, next);
        auto fnd = find_if(moves.begin(),
                           moves.end(),
                           [&](auto &ent) {
                               return ent.first == word;
                           });
        if (fnd == moves.end()) {
            cerr << "unknown move: " << word << "\n";
            abort();
        }
        out = out.apply(fnd->second);
        it = next;
        while (it != str.end() && *it == ' ') {
            ++it;
        }
    }
    return out;
}

string to_algorithm(const vector<Cube> &path) {
    stringstream out;
    for (auto &cube : path) {
        auto fnd = find_if(moves.begin(),
                           moves.end(),
                           [&](auto &ent) {
                               return ent.second == cube;
                           });
        assert(fnd != moves.end());
        if (&cube != &path.front()) {
            out << ' ';
        }
        out << fnd->first;
    }
    return out.str();
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
            {Rotations::Linv, Rotations::Rinv},
        },
        {
            "R2", 2,
            true,
            {Rotations::R, Rotations::R},
        },
    };
    for (auto &tc: tests) {
        INFO("search(\"" << tc.in << "\", " << tc.depth << ")");
        vector<Cube> path;
        Cube in = from_algorithm(tc.in);
        bool ok = search(in, path, tc.depth);
        CHECK(ok == tc.ok);
        INFO("-> " << to_algorithm(path));
        CHECK(equal(tc.out.begin(), tc.out.end(),
                    path.begin(), path.end()));
    }
}
