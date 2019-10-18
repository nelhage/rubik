#ifndef RUBIK_H
#define RUBIK_H

#include <array>
#include <emmintrin.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "absl/types/variant.h"

namespace rubik {
class Rotations;

enum class Color : char {
  Red = 'R',
  White = 'W',
  Green = 'G',
  Blue = 'B',
  Orange = 'O',
  Yellow = 'Y',
};

enum class Face : char {
  Up = 'U',
  Down = 'D',
  Left = 'L',
  Right = 'R',
  Front = 'F',
  Back = 'B',
};

class Cube {
  friend class Rotations;

  __m128i edges;
  __m128i corners;

public:
  static constexpr uint8_t kEdgePermMask = 0x0f;
  static constexpr uint8_t kEdgeAlignShift = 4;
  static constexpr uint8_t kEdgeAlignMask = 0x1 << kEdgeAlignShift;
  static constexpr uint8_t kCornerPermMask = 0x07;
  static constexpr uint8_t kCornerAlignShift = 3;
  static constexpr uint8_t kCornerAlignMask = 3 << kCornerAlignShift;

  Cube();
  Cube(__m128i edges, __m128i corners);
  Cube(std::array<uint8_t, 12> edges, std::array<uint8_t, 8> corners);

  Cube apply(const Cube &rhs) const;
  Cube invert() const;

  void sanityCheck() const;

  Cube(const Cube &) = default;
  Cube &operator=(const Cube &) = default;

  bool operator==(const Cube &other) const;
  bool operator!=(const Cube &other) const { return !(*this == other); };

  const __m128i &getEdges() const { return edges; }

  const __m128i &getCorners() const { return corners; }

  template <typename H> friend H AbslHashValue(H h, const Cube &c) {
    union {
      __m128i mm;
      struct {
        uint64_t u1;
        uint32_t u2;
        uint32_t pad;
      };
    } eu;
    eu.mm = c.edges;
    union {
      __m128i mm;
      struct {
        uint64_t u1;
        uint64_t pad;
      };
    } cu;
    cu.mm = c.corners;

    return H::combine(std::move(h), eu.u1, eu.u2, cu.u1);
  }
};

struct search_node;
template <typename Check, typename Prune, typename Unwind>
bool search(const Cube &pos, const std::vector<search_node> &moves, int depth,
            const Check &check, const Prune &prune, const Unwind &unwind);
template <typename Visit>
void search(const Cube &pos, const std::vector<search_node> &moves, int depth,
            const Visit &visit);
bool search(Cube start, std::vector<Cube> &path, int max_depth);

template <typename Ok, typename Err> using Result = absl::variant<Ok, Err>;

struct Error {
  std::string error;
};

Result<Cube, Error> from_algorithm(const std::string &str);
Result<Cube, Error> from_facelets(const std::string &notation);
Result<std::string, Error> to_algorithm(const std::vector<Cube> &path);

class Rotations {
public:
  const Cube L, L2, Linv;
  const Cube R, R2, Rinv;
  const Cube U, U2, Uinv;
  const Cube D, D2, Dinv;
  const Cube F, F2, Finv;
  const Cube B, B2, Binv;

  Rotations();
};

}; // namespace rubik
#endif
