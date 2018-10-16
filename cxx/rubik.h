#include <stdint.h>
#include <array>

namespace rubik {
class Rotations;

class Cube {
    friend class Rotations;

    static constexpr uint8_t kEdgePermMask    = 0x0f;
    static constexpr uint8_t kEdgeAlignMask   = 0x10;
    static constexpr uint8_t kEdgeAlignShift  = 4;
    static constexpr uint8_t kCornerPermMask  = 0x07;
    static constexpr uint8_t kCornerAlignMask = 0x30;
    static constexpr uint8_t kCornerAlignShift  = 4;

    struct storage {
        union {
            std::array<uint8_t, 12> edges;
            struct {
                uint64_t low;
                uint32_t high;
            } edge_bits;
        };
        union {
            std::array<uint8_t, 8> corners;
            uint64_t corner_bits;
        };
    };
    storage store;

    Cube(std::array<uint8_t, 12> edges, std::array<uint8_t, 8> corners);
public:
    Cube();
    Cube apply(const Cube &rhs) const;
    Cube invert() const;

    void sanityCheck() const;

    Cube(const Cube &) = default;
    Cube& operator=(const Cube &) = default;

    bool operator==(const Cube &other) const;
    bool operator!=(const Cube &other) const {
        return !(*this == other);
    };
};

class Rotations {
    Rotations();
    static constexpr uint8_t E = Cube::kEdgeAlignMask;
    static constexpr uint8_t C0 = 0 << Cube::kCornerAlignShift;
    static constexpr uint8_t C1 = 1 << Cube::kCornerAlignShift;
    static constexpr uint8_t C2 = 2 << Cube::kCornerAlignShift;
public:
    static const Cube L, L2, Linv;
    static const Cube R, R2, Rinv;
    static const Cube U, U2, Uinv;
    static const Cube D, D2, Dinv;
    static const Cube F, F2, Finv;
    static const Cube B, B2, Binv;
};

};
