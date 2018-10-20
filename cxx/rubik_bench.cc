#include <chrono>
#include <iostream>

#include "rubik.h"

using namespace rubik;
using namespace std;

constexpr uint64_t N_ITERATIONS = 1 << 24;

template<typename To, typename From>
bool try_fmt(std::ostream &out, const std::string &unit, From dur) {
    auto to = chrono::duration_cast<To>(dur);
    if (to.count() < 10000) {
        out << to.count() << unit;
        return true;
    }
    return false;
}

template<typename T>
void format_duration(std::ostream &out, T dur) {
    if (try_fmt<chrono::nanoseconds>(out, "ns", dur)) {
        return;
    }
    if (try_fmt<chrono::microseconds>(out, "us", dur)) {
        return;
    }
    if (try_fmt<chrono::milliseconds>(out, "ms", dur)) {
        return;
    }
    if (try_fmt<chrono::seconds>(out, "s", dur)) {
        return;
    }
    out << dur.count() << "<" << T::period::num << "/" << T::period::den << ">\n";
}

template<typename T>
void benchmark(const std::string &name, T body, uint64_t N = N_ITERATIONS) {
    auto before = chrono::steady_clock::now();
    for (uint64_t i = 0; i < N; ++i) {
        body();
    }
    auto after = chrono::steady_clock::now();
    cout << name << ": ";
    format_duration(cout, (after - before)/N);
    cout << "/op" << "\n";
}

void bench_rotate() {
    Cube cube;
    benchmark("rotate", [cube]() {
            cube.apply(cube);
        });
}

void bench_invert() {
    Cube cube;
    benchmark("invert", [cube]() {
            cube.invert();
        });
}

void bench_search() {
    Cube superflip = Cube().
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
    vector<Cube> out;

    benchmark("search-1", [&]() {
            search(superflip, out, 1);
        }, 1 << 21);
    benchmark("search-2", [&]() {
            search(superflip, out, 2);
        }, 1 << 18);
    benchmark("search-3", [&]() {
            search(superflip, out, 3);
        }, 1 << 15);
    benchmark("search-4", [&]() {
            search(superflip, out, 4);
        }, 1 << 12);
    benchmark("search-8", [&]() {
            search(superflip, out, 8);
        }, 1);
}

int main() {
    bench_rotate();
    bench_invert();
    bench_search();
    return 0;
}
