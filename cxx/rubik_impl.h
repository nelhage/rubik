#ifndef RUBIK_IMPL_H
#define RUBIK_IMPL_H
#include <vector>

namespace rubik {
struct search_node {
    const Cube &rotation;
    std::vector<search_node> *next;
};
extern const std::vector<search_node> *ftm_root;

};

#endif
