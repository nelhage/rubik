#ifndef RUBIK_IMPL_H
#define RUBIK_IMPL_H
#include <vector>

namespace rubik {
struct search_node {
    const Cube &rotation;
    std::vector<search_node> *next;
};
extern const std::vector<search_node> *ftm_root;


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

};

#endif
