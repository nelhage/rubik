#include <array>
#include <stdint.h>

namespace rubik {
extern std::array<int8_t, 32 * 32> edge_dist;
extern std::array<int8_t, 32 * 32> corner_dist;
extern std::array<int8_t, 32 * 32> pair0_dist;
extern std::array<int8_t, 32 * 32 * 32 * 32> quad01_dist;
}; // namespace rubik
