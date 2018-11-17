#ifndef RUBIK_IMPL_H
#define RUBIK_IMPL_H
#include <vector>

namespace rubik {
struct search_node {
    const Cube &rotation;
    std::vector<search_node> *next;
};
extern const std::vector<search_node> *ftm_root;

union edge_union {
    __m128i mm;
    struct {
        std::array<uint8_t, 12> arr;
        uint32_t pad;
    };
};

union corner_union {
    __m128i mm;
    struct {
        std::array<uint8_t, 8> arr;
        uint32_t pad;
    };
};

template <typename Check, typename Prune, typename Unwind>
bool search(const Cube &pos,
            const std::vector<search_node> &moves,
            int depth,
            const Check &check, const Prune &prune, const Unwind &unwind) {
    if (check(pos, depth)) {
        return true;
    }
    if (depth <= 0) {
        return false;
    }
    if (prune(pos, depth)) {
        return false;
    }
    for (auto &rot: moves) {
        Cube next = pos.apply(rot.rotation);
        if (search(next, *rot.next, depth-1, check, prune, unwind)) {
            unwind(depth, rot.rotation);
            return true;
        }
    }
    return false;
}

template <typename Visit>
void search(const Cube &pos,
            const std::vector<search_node> &moves,
            int depth,
            const Visit &visit) {
    search(pos, moves, depth,
           [&](const Cube &pos, int depth) {
               visit(pos, depth);
               return false;
           },
           [&](const Cube&, int) { return false; },
           [&](int, const Cube&) {});
}

int flip_heuristic(const Cube &pos);
int edge_heuristic(const Cube &pos);

constexpr bool debug_mode =
#ifdef NDEBUG
    0
#else
    1
#endif
    ;

};

#endif
