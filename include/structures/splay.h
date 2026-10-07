/* @title: Splay tree */
#pragma once
#include <container_of.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct splay_node {
    struct splay_node *left;
    struct splay_node *right;
    struct splay_node *parent;
};

struct splay_node_ops {
    int (*cmp)(const struct splay_node *a, const struct splay_node *b);
    int (*cmp_key)(const struct splay_node *node, const void *key);
};

struct splay_tree {
    struct splay_node *root;
    const struct splay_node_ops *ops;
};

#define SPLAY_NODE_INIT                                                        \
    (struct splay_node) {                                                      \
        .left = NULL, .right = NULL, .parent = NULL                            \
    }

static inline void splay_init_node(struct splay_node *n) {
    n->left = n->right = n->parent = NULL;
}

static inline bool splay_tree_empty(const struct splay_tree *tree) {
    return tree->root == NULL;
}

void splay_tree_init(struct splay_tree *tree, const struct splay_node_ops *ops);

void splay(struct splay_tree *tree, struct splay_node *node);

void splay_insert(struct splay_tree *tree, struct splay_node *node);

void splay_remove(struct splay_tree *tree, struct splay_node *node);

struct splay_node *splay_find(struct splay_tree *tree, const void *key);

struct splay_node *splay_first(const struct splay_tree *tree);
struct splay_node *splay_last(const struct splay_tree *tree);
struct splay_node *splay_next(const struct splay_node *node);
struct splay_node *splay_prev(const struct splay_node *node);

#define splay_entry(ptr, type, member) container_of(ptr, type, member)

#define splay_for_each(pos, tree)                                              \
    for ((pos) = splay_first(tree); (pos); (pos) = splay_next(pos))

#define splay_for_each_safe(pos, n, tree)                                      \
    for ((pos) = splay_first(tree), (n) = (pos) ? splay_next(pos) : NULL;      \
         (pos); (pos) = (n), (n) = (pos) ? splay_next(pos) : NULL)

#define splay_for_each_entry(pos, tree, member)                                \
    for (typeof(splay_first(tree)) __n = splay_first(tree);                    \
         __n != NULL &&                                                        \
         (((pos) = splay_entry(__n, typeof(*(pos)), member)), true);           \
         __n = splay_next(__n))

#define splay_for_each_entry_safe(pos, tmp, tree, member)                      \
    for (typeof(splay_first(tree)) __n = splay_first(tree), __nx = NULL;       \
         __n != NULL &&                                                        \
         (((pos) = splay_entry(__n, typeof(*(pos)), member)),                  \
          (__nx = splay_next(__n)),                                            \
          ((tmp) = __nx ? splay_entry(__nx, typeof(*(tmp)), member) : NULL),   \
          true);                                                               \
         __n = __nx)

#define splay_for_each_reverse(pos, tree)                                      \
    for ((pos) = splay_last(tree); (pos); (pos) = splay_prev(pos))

#define splay_for_each_safe_reverse(pos, n, tree)                              \
    for ((pos) = splay_last(tree), (n) = (pos) ? splay_prev(pos) : NULL;       \
         (pos); (pos) = (n), (n) = (pos) ? splay_prev(pos) : NULL)

#define splay_for_each_entry_reverse(pos, tree, member)                        \
    for (typeof(splay_last(tree)) __n = splay_last(tree);                      \
         __n != NULL &&                                                        \
         (((pos) = splay_entry(__n, typeof(*(pos)), member)), true);           \
         __n = splay_prev(__n))

#define splay_for_each_entry_safe_reverse(pos, tmp, tree, member)              \
    for (typeof(splay_last(tree)) __n = splay_last(tree), __nx = NULL;         \
         __n != NULL &&                                                        \
         (((pos) = splay_entry(__n, typeof(*(pos)), member)),                  \
          (__nx = splay_prev(__n)),                                            \
          ((tmp) = __nx ? splay_entry(__nx, typeof(*(tmp)), member) : NULL),   \
          true);                                                               \
         __n = __nx)
