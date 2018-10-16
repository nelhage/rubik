#include <chrono>
#include <iostream>

#include "rubik.h"

using namespace rubik;
using namespace std;

constexpr uint64_t N_ITERATIONS = 1 << 24;

template<typename T>
void benchmark(const std::string &name, T body) {
    auto before = chrono::steady_clock::now();
    for (uint64_t i = 0; i < N_ITERATIONS; ++i) {
        body();
    }
    auto after = chrono::steady_clock::now();
    auto ns = chrono::duration_cast<chrono::nanoseconds>(after - before);
    cout << name << ": " << (ns/N_ITERATIONS).count() << "ns/op" << "\n";
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

int main() {
    bench_rotate();
    bench_invert();
    return 0;
}
