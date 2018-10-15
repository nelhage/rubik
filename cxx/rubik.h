#include <stdint.h>

namespace rubik {
class CubeDetails;
class Cube {
    const uint8_t kEdgePermMask    = 0x0f;
    const uint8_t kEdgeAlignMask   = 0x10;
    const uint8_t kEdgeAlignShift  = 4;
    const uint8_t kCornerPermMask  = 0x07;
    const uint8_t kCornerAlignMask = 0x30;
    const uint8_t kCornerAlignShift  = 4;

    uint8_t edges[12];
    uint8_t corners[8];

    Cube(uint8_t edges[12], uint8_t corners[8]);
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
};
