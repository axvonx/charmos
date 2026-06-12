/* @title: Red-Black Interval Tree */
#pragma once
#include <container_of.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define rbit_for_each_safe(pos, tmp, tree)                                     \
    for (pos = rbit_first(tree), tmp = rbit_next(pos); pos != NULL;            \
         pos = tmp, tmp = rbit_next(pos))

#define rbit_for_each_entry_safe(pos, tmp, tree, member)                       \
    for (pos = rbit_entry(rbit_first(tree), typeof(*pos), member),             \
        tmp = rbit_entry(rbit_next(&pos->member), typeof(*pos), member);       \
         pos != NULL; pos = tmp,                                               \
        tmp = rbit_entry(rbit_next(&tmp->member), typeof(*tmp), member))

#define rbit_for_each_safe_reverse(pos, tmp, tree)                             \
    for (pos = rbit_last(tree), tmp = rbit_prev(pos); pos != NULL;             \
         pos = tmp, tmp = rbit_prev(pos))

#define rbit_for_each_entry_safe_reverse(pos, tmp, tree, member)               \
    for (pos = rbit_entry(rbit_last(tree), typeof(*pos), member),              \
        tmp = rbit_entry(rbit_prev(&pos->member), typeof(*pos), member);       \
         pos != NULL; pos = tmp,                                               \
        tmp = rbit_entry(rbit_prev(&tmp->member), typeof(*tmp), member))

#define rbit_for_each(pos, tree)                                               \
    for (pos = rbit_first(tree); pos != NULL; pos = rbit_next(pos))

#define rbit_for_each_entry(pos, tree, member)                                 \
    for (pos = rbit_entry(rbit_first(tree), typeof(*pos), member);             \
         pos != NULL;                                                          \
         pos = rbit_entry(rbit_next(&pos->member), typeof(*pos), member))

#define rbit_for_each_reverse(pos, tree)                                       \
    for (pos = rbit_last(tree); pos != NULL; pos = rbit_prev(pos))

#define rbit_for_each_entry_reverse(pos, tree, member)                         \
    for (pos = rbit_entry(rbit_last(tree), typeof(*pos), member); pos != NULL; \
         pos = rbit_entry(rbit_prev(&pos->member), typeof(*pos), member))

#define rbit_entry(ptr, type, member) container_of(ptr, type, member)
#define rbit_parent(n) ((n)->parent)

enum rbit_color { RBIT_RED, RBIT_BLACK };

struct interval {
    size_t low;
    size_t high;
};

struct rbit_node {
    struct interval interval;
    enum rbit_color color;
    size_t max;
    struct rbit_node *left;
    struct rbit_node *right;
    struct rbit_node *parent;
};

/* Augmentation hook: recompute `node`'s augmented data purely from its own
 * interval and its children, and return whether anything changed */
typedef bool (*rbit_augment_fn)(struct rbit_node *node);

struct rbit {
    struct rbit_node *root;
    rbit_augment_fn augment; /* NULL => default interval-max */
};

static inline size_t rbit_node_max(const struct rbit_node *n) {
    return n ? n->max : 0;
}

#define RBIT_NODE_INIT                                                         \
    (struct rbit_node) {                                                       \
        .interval = {0, 0}, .color = RBIT_BLACK, .max = 0, .left = NULL,       \
        .right = NULL, .parent = NULL                                          \
    }

static inline void rbit_init_node(struct rbit_node *n) {
    n->interval.low = 0;
    n->interval.high = 0;
    n->color = RBIT_BLACK;
    n->max = 0;
    n->left = n->right = n->parent = NULL;
}

static inline struct rbit_node *rbit_first(const struct rbit *tree) {
    struct rbit_node *node = tree->root;
    if (!node)
        return NULL;
    while (node->left)
        node = node->left;
    return node;
}

static inline struct rbit_node *rbit_last(const struct rbit *tree) {
    struct rbit_node *node = tree->root;
    if (!node)
        return NULL;
    while (node->right)
        node = node->right;
    return node;
}

static inline struct rbit_node *rbit_prev(struct rbit_node *node) {
    if (!node)
        return NULL;
    if (node->left) {
        /* predecessor is rightmost node of left subtree */
        node = node->left;
        while (node->right)
            node = node->right;
        return node;
    }

    /* climb up until we come from the right */
    struct rbit_node *parent = node->parent;
    while (parent && node == parent->left) {
        node = parent;
        parent = parent->parent;
    }
    return parent;
}

static inline bool rbit_empty(const struct rbit *tree) {
    return !tree->root;
}

struct rbit *rbit_init(struct rbit *rbit);
struct rbit *rbit_tree_create(void);
struct rbit_node *rbit_find_min(struct rbit_node *node);
struct rbit_node *rbit_find_max(struct rbit_node *node);
struct rbit_node *rbit_min(struct rbit *tree);
struct rbit_node *rbit_max(struct rbit *tree);
struct rbit_node *rbit_next(struct rbit_node *node);
struct rbit_node *rbit_find_predecessor(struct rbit *tree, size_t low);
struct rbit_node *rbit_find_successor(struct rbit *tree, size_t low);
bool rbit_has_node(struct rbit *tree, struct rbit_node *node);
void rbit_delete(struct rbit *tree, struct rbit_node *z);
struct rbit_node *rbit_search(struct rbit_node *root, struct interval iv);
struct rbit_node *rbit_overlap_search(struct rbit_node *root,
                                      struct interval iv);
void rbit_remove(struct rbit *tree, struct interval iv);
void rbit_insert(struct rbit *tree, struct rbit_node *new_node);
