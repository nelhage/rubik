#include <chrono>
#include <iostream>
#include <algorithm>

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
void benchmark(const std::string &name, T body) {
    for (uint8_t order = 0; ; ++order) {
        auto before = chrono::steady_clock::now();
        uint64_t N = (1ul << order);
        for (uint64_t i = 0; i < N; ++i) {
            body();
        }
        auto after = chrono::steady_clock::now();
        if ((after - before) < chrono::seconds(1)) {
            continue;
        }

        cout << name << ": ";
        format_duration(cout, (after - before)/N);
        cout << "/op [order=" << (int)order << "]" << "\n";
        break;
    }
}

void bench_rotate() {
    Cube cube = Rotations::R;
    benchmark("rotate", [&]() {
            cube = cube.apply(Rotations::L);
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
            if (search(superflip, out, 1)) {
                abort();
            }
        });
    benchmark("search-2", [&]() {
            if (search(superflip, out, 2)) {
                abort();
            }
        });
    benchmark("search-3", [&]() {
            if (search(superflip, out, 3)) {
                abort();
            }
        });
    benchmark("search-4", [&]() {
            if (search(superflip, out, 4)) {
                abort();
            }
        });
    benchmark("search-8", [&]() {
            if (search(superflip, out, 8)) {
                abort();
            }
        });
    benchmark("search-10", [&]() {
            if (search(superflip, out, 10)) {
                abort();
            }
        });
}

int main() {
    bench_rotate();
    bench_invert();
    bench_search();

    return 0;
}
