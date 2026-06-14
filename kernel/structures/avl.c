#include <console/panic.h>
#include <structures/avl.h>

static inline int avl_height(const struct avl_tree_node *node) {
    return node ? node->height : 0;
}

static inline int avl_balance_factor(const struct avl_tree_node *node) {
    return node ? avl_height(node->left) - avl_height(node->right) : 0;
}

static inline void avl_update_height(struct avl_tree_node *node) {
    int lh = avl_height(node->left);
    int rh = avl_height(node->right);
    node->height = 1 + (lh > rh ? lh : rh);
}

static void avl_check_cycle(struct avl_tree_node *node, const char *caller) {
    struct avl_tree_node *tortoise = node;
    struct avl_tree_node *hare = node;

    while (hare && hare->left != NULL) {
        tortoise = tortoise ? tortoise->left : NULL;
        hare = hare->left->left;

        if (tortoise == NULL || hare == NULL)
            break;

        if (tortoise == hare) {
            panic("Cycle detected in %s (left spine): tortoise == hare at "
                  "node=%p",
                  caller, (void *) tortoise);
            break;
        }
    }

    tortoise = node;
    hare = node;

    while (hare && hare->right != NULL) {
        tortoise = tortoise ? tortoise->right : NULL;
        hare = hare->right->right;

        if (tortoise == NULL || hare == NULL)
            break;

        if (tortoise == hare) {
            panic("Cycle detected in %s (right spine): tortoise == hare at "
                  "node=%p",
                  caller, (void *) tortoise);
            break;
        }
    }
}

/* rotations
 *
 * left_rotate(x):
 *      x              y
 *       \            / \
 *        y    =>    x   c
 *       / \          \
 *      b   c          b
 */
static void avl_left_rotate(struct avl_tree *tree, struct avl_tree_node *x) {
    struct avl_tree_node *y = x->right;
    x->right = y->left;
    if (y->left)
        y->left->parent = x;

    y->parent = x->parent;
    if (!x->parent)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    y->left = x;
    x->parent = y;

    avl_update_height(x);
    avl_update_height(y);
}

static void avl_right_rotate(struct avl_tree *tree, struct avl_tree_node *y) {
    struct avl_tree_node *x = y->left;
    y->left = x->right;
    if (x->right)
        x->right->parent = y;

    x->parent = y->parent;
    if (!y->parent)
        tree->root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;

    x->right = y;
    y->parent = x;

    avl_update_height(y);
    avl_update_height(x);
}

/* Walk up from `node` rebalancing as needed. */
static void avl_rebalance(struct avl_tree *tree, struct avl_tree_node *node) {
    while (node) {
        struct avl_tree_node *parent = node->parent;
        avl_update_height(node);
        int bf = avl_balance_factor(node);

        if (bf > 1) {
            if (avl_balance_factor(node->left) < 0)
                avl_left_rotate(tree, node->left);
            avl_right_rotate(tree, node);
        } else if (bf < -1) {
            if (avl_balance_factor(node->right) > 0)
                avl_right_rotate(tree, node->right);
            avl_left_rotate(tree, node);
        }

        node = parent;
    }
}

void avl_tree_init(struct avl_tree *tree, const struct avl_tree_node_ops *ops) {
    tree->root = NULL;
    tree->ops = ops;
}

void avl_tree_insert(struct avl_tree *tree, struct avl_tree_node *new_node) {
    avl_check_cycle(tree->root, __func__);
    new_node->left = new_node->right = NULL;
    new_node->height = 1;

    if (!tree->root) {
        new_node->parent = NULL;
        tree->root = new_node;
        return;
    }

    struct avl_tree_node *current = tree->root;
    struct avl_tree_node *parent = NULL;
    int last_cmp = 0;
    while (current) {
        parent = current;
        /* ties go right: cmp == 0 => take right branch */
        last_cmp = tree->ops->cmp(new_node, current);
        current = (last_cmp < 0) ? current->left : current->right;
    }

    new_node->parent = parent;
    if (last_cmp < 0)
        parent->left = new_node;
    else
        parent->right = new_node;

    avl_rebalance(tree, parent);
}

static void avl_transplant(struct avl_tree *tree, struct avl_tree_node *u,
                           struct avl_tree_node *v) {
    if (!u->parent)
        tree->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    if (v)
        v->parent = u->parent;
}

static struct avl_tree_node *avl_min_node(struct avl_tree_node *node) {
    while (node && node->left)
        node = node->left;
    return node;
}

void avl_tree_remove(struct avl_tree *tree, struct avl_tree_node *node) {
    avl_check_cycle(tree->root, __func__);
    struct avl_tree_node *rebalance_start;

    if (!node->left) {
        rebalance_start = node->parent;
        avl_transplant(tree, node, node->right);
    } else if (!node->right) {
        rebalance_start = node->parent;
        avl_transplant(tree, node, node->left);
    } else {
        /* splice in the in-order successor. */
        struct avl_tree_node *succ = avl_min_node(node->right);

        if (succ->parent == node) {
            rebalance_start = succ;
        } else {
            rebalance_start = succ->parent;
            avl_transplant(tree, succ, succ->right);
            succ->right = node->right;
            succ->right->parent = succ;
        }

        avl_transplant(tree, node, succ);
        succ->left = node->left;
        succ->left->parent = succ;
    }

    if (rebalance_start)
        avl_rebalance(tree, rebalance_start);
}

struct avl_tree_node *avl_tree_find(const struct avl_tree *tree,
                                    const void *key) {
    avl_check_cycle(tree->root, __func__);
    struct avl_tree_node *node = tree->root;
    while (node) {
        int c = tree->ops->cmp_key(node, key);
        if (c == 0)
            return node;
        node = (c > 0) ? node->left : node->right;
    }
    return NULL;
}

struct avl_tree_node *avl_tree_first(const struct avl_tree *tree) {
    avl_check_cycle(tree->root, __func__);
    return avl_min_node(tree->root);
}

struct avl_tree_node *avl_tree_last(const struct avl_tree *tree) {
    avl_check_cycle(tree->root, __func__);
    struct avl_tree_node *node = tree->root;
    while (node && node->right)
        node = node->right;
    return node;
}

struct avl_tree_node *avl_tree_next(const struct avl_tree_node *node) {
    if (!node)
        return NULL;
    avl_check_cycle((struct avl_tree_node *) node, __func__);
    if (node->right)
        return avl_min_node(node->right);

    /* climb until we come up from a left child */
    const struct avl_tree_node *p = node->parent;
    while (p && node == p->right) {
        node = p;
        p = p->parent;
    }
    return (struct avl_tree_node *) p;
}

struct avl_tree_node *avl_tree_prev(const struct avl_tree_node *node) {
    if (!node)
        return NULL;
    avl_check_cycle((struct avl_tree_node *) node, __func__);
    if (node->left) {
        struct avl_tree_node *n = node->left;
        while (n->right)
            n = n->right;
        return n;
    }

    const struct avl_tree_node *p = node->parent;
    while (p && node == p->left) {
        node = p;
        p = p->parent;
    }
    return (struct avl_tree_node *) p;
}

static int avl_validate_node(const struct avl_tree *tree,
                             const struct avl_tree_node *node,
                             const struct avl_tree_node *parent,
                             int *height_out) {
    if (!node) {
        *height_out = 0;
        return 1;
    }

    if (node->parent != parent) {
        panic("AVL parent link broken");
        return 0;
    }

    if (node->left && tree->ops->cmp(node->left, node) > 0) {
        panic("AVL ordering violation: left child > parent");
        return 0;
    }
    if (node->right && tree->ops->cmp(node->right, node) < 0) {
        panic("AVL ordering violation: right child < parent");
        return 0;
    }

    int lh = 0, rh = 0;
    if (!avl_validate_node(tree, node->left, node, &lh))
        return 0;
    if (!avl_validate_node(tree, node->right, node, &rh))
        return 0;

    int expected = 1 + (lh > rh ? lh : rh);
    if (node->height != expected) {
        panic("AVL height mismatch: stored %d, actual %d", node->height,
              expected);
        return 0;
    }

    int bf = lh - rh;
    if (bf < -1 || bf > 1) {
        panic("AVL balance violation: bf = %d", bf);
        return 0;
    }

    *height_out = expected;
    return 1;
}

int avl_tree_validate(const struct avl_tree *tree) {
    avl_check_cycle(tree->root, __func__);
    int h = 0;
    return avl_validate_node(tree, tree->root, NULL, &h);
}
