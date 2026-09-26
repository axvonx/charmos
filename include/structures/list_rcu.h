/* @title: RCU list operations */
#pragma once
#include <compiler/atomic.h>
#include <compiler/core.h>
#include <container_of.h>
#include <stdbool.h>
#include <stddef.h>
#include <structures/hlist.h>
#include <structures/list.h>
#include <sync/rcu.h>

static inline void INIT_LIST_HEAD_RCU(struct list_head *list) {
    ca_write_once(list->next, list);
    ca_write_once(list->prev, list);
}

#define list_next_rcu(list) (*((struct list_head **) &(list)->next))
#define list_bidir_prev_rcu(list) (*((struct list_head **) &(list)->prev))
#define list_tail_rcu(head) (*((struct list_head **) &(head)->prev))

static inline void __list_add_rcu(struct list_head *new, struct list_head *prev,
                                  struct list_head *next) {
    if (cc_unlikely(!__list_add_valid(new, prev, next)))
        return;

    new->next = next;
    new->prev = prev;
    rcu_assign_pointer(list_next_rcu(prev), new);
    next->prev = new;
}

static inline void list_add_rcu(struct list_head *new, struct list_head *head) {
    __list_add_rcu(new, head, head->next);
}

static inline void list_add_tail_rcu(struct list_head *new,
                                     struct list_head *head) {
    __list_add_rcu(new, head->prev, head);
}

static inline void list_del_rcu(struct list_head *entry) {
    if (cc_unlikely(!__list_del_entry_valid(entry)))
        return;

    __list_del(entry->prev, entry->next);
    entry->prev = NULL;
}

static inline void list_bidir_del_rcu(struct list_head *entry) {
    if (cc_unlikely(!__list_del_entry_valid(entry)))
        return;

    __list_del(entry->prev, entry->next);
}

static inline void list_replace_rcu(struct list_head *old,
                                    struct list_head *new) {
    new->next = old->next;
    new->prev = old->prev;
    rcu_assign_pointer(list_next_rcu(new->prev), new);
    new->next->prev = new;
    old->prev = NULL;
}

static inline void __list_splice_rcu(const struct list_head *list,
                                     struct list_head *prev,
                                     struct list_head *next) {
    if (cc_unlikely(!__list_splice_valid(list, prev, next)))
        return;

    struct list_head *first = list->next;
    struct list_head *last = list->prev;

    last->next = next;
    first->prev = prev;
    next->prev = last;
    rcu_assign_pointer(list_next_rcu(prev), first);
}

static inline void list_splice_rcu(struct list_head *list,
                                   struct list_head *head) {
    if (!list_empty(list))
        __list_splice_rcu(list, head, head->next);
}

static inline void __list_splice_init_rcu(struct list_head *list,
                                          struct list_head *prev,
                                          struct list_head *next,
                                          void (*sync)(void)) {
    struct list_head *first = list->next;
    struct list_head *last = list->prev;

    INIT_LIST_HEAD_RCU(list);

    if (sync)
        sync();

    last->next = next;
    rcu_assign_pointer(list_next_rcu(prev), first);
    first->prev = prev;
    next->prev = last;
}

static inline void list_splice_init_rcu(struct list_head *list,
                                        struct list_head *head,
                                        void (*sync)(void)) {
    if (!list_empty(list))
        __list_splice_init_rcu(list, head, head->next, sync);
}

static inline void list_splice_tail_init_rcu(struct list_head *list,
                                             struct list_head *head,
                                             void (*sync)(void)) {
    if (!list_empty(list))
        __list_splice_init_rcu(list, head->prev, head, sync);
}

#define list_entry_rcu(ptr, type, member)                                      \
    container_of(rcu_dereference(ptr), type, member)

#define list_first_or_null_rcu(ptr, type, member)                              \
    ({                                                                         \
        struct list_head *__ptr = (ptr);                                       \
        struct list_head *__next = rcu_dereference(__ptr->next);               \
        cc_likely(__ptr != __next) ? list_entry(__next, type, member) : NULL;  \
    })

#define list_next_or_null_rcu(head, ptr, type, member)                         \
    ({                                                                         \
        struct list_head *__head = (head);                                     \
        struct list_head *__ptr = (ptr);                                       \
        struct list_head *__next = rcu_dereference(__ptr->next);               \
        cc_likely(__next != __head) ? list_entry(__next, type, member) : NULL; \
    })

#define list_for_each_rcu(pos, head)                                           \
    for (pos = rcu_dereference((head)->next); pos != (head);                   \
         pos = rcu_dereference(pos->next))

#define list_for_each_entry_rcu(pos, head, member)                             \
    for (pos = list_entry_rcu((head)->next, typeof(*pos), member);             \
         &pos->member != (head);                                               \
         pos = list_entry_rcu(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_continue_rcu(pos, head, member)                    \
    for (pos = list_entry_rcu(pos->member.next, typeof(*pos), member);         \
         &pos->member != (head);                                               \
         pos = list_entry_rcu(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_from_rcu(pos, head, member)                        \
    for (; &pos->member != (head);                                             \
         pos = list_entry_rcu(pos->member.next, typeof(*pos), member))

#define hlist_first_rcu(head) (*((struct hlist_node **) &(head)->first))
#define hlist_next_rcu(node) (*((struct hlist_node **) &(node)->next))
#define hlist_pprev_rcu(node) (*((struct hlist_node **) ((node)->pprev)))

#define hlist_entry_rcu(ptr, type, member)                                     \
    container_of(rcu_dereference(ptr), type, member)

static inline void __hlist_del(struct hlist_node *n) {
    struct hlist_node *next = n->next;
    struct hlist_node **pprev = n->pprev;

    *pprev = next;
    if (next)
        next->pprev = pprev;
}

static inline void hlist_del_rcu(struct hlist_node *n) {
    __hlist_del(n);
    n->pprev = NULL;
}

static inline void hlist_del_init_rcu(struct hlist_node *n) {
    if (!hlist_unhashed(n)) {
        __hlist_del(n);
        ca_write_once(n->pprev, NULL);
    }
}

static inline void hlist_replace_rcu(struct hlist_node *old,
                                     struct hlist_node *new) {
    struct hlist_node *next = old->next;

    new->next = next;
    new->pprev = old->pprev;
    rcu_assign_pointer(*(struct hlist_node **) new->pprev, new);
    if (next)
        new->next->pprev = &new->next;
    old->pprev = NULL;
}

static inline void hlists_swap_heads_rcu(struct hlist_head *left,
                                         struct hlist_head *right) {
    struct hlist_node *node1 = left->first;
    struct hlist_node *node2 = right->first;

    rcu_assign_pointer(left->first, node2);
    rcu_assign_pointer(right->first, node1);
    if (node2)
        ca_write_once(node2->pprev, &left->first);
    if (node1)
        ca_write_once(node1->pprev, &right->first);
}

static inline void hlist_add_head_rcu(struct hlist_node *n,
                                      struct hlist_head *h) {
    struct hlist_node *first = h->first;

    n->next = first;
    n->pprev = &h->first;
    rcu_assign_pointer(hlist_first_rcu(h), n);
    if (first)
        first->pprev = &n->next;
}

static inline void hlist_add_tail_rcu(struct hlist_node *n,
                                      struct hlist_head *h) {
    struct hlist_node *i, *last = NULL;

    for (i = h->first; i; i = i->next)
        last = i;

    if (last) {
        n->next = last->next;
        n->pprev = &last->next;
        rcu_assign_pointer(hlist_next_rcu(last), n);
    } else {
        hlist_add_head_rcu(n, h);
    }
}

static inline void hlist_add_before_rcu(struct hlist_node *n,
                                        struct hlist_node *next) {
    n->pprev = next->pprev;
    n->next = next;
    rcu_assign_pointer(hlist_pprev_rcu(n), n);
    next->pprev = &n->next;
}

static inline void hlist_add_behind_rcu(struct hlist_node *n,
                                        struct hlist_node *prev) {
    n->next = prev->next;
    n->pprev = &prev->next;
    rcu_assign_pointer(hlist_next_rcu(prev), n);
    if (n->next)
        n->next->pprev = &n->next;
}

#define __hlist_for_each_rcu(pos, head)                                        \
    for (pos = rcu_dereference(hlist_first_rcu(head)); pos;                    \
         pos = rcu_dereference(hlist_next_rcu(pos)))

#define hlist_for_each_entry_rcu(pos, head, member)                            \
    for (pos = hlist_entry_safe(rcu_dereference(hlist_first_rcu(head)),        \
                                typeof(*(pos)), member);                       \
         pos; pos = hlist_entry_safe(                                          \
                  rcu_dereference(hlist_next_rcu(&(pos)->member)),             \
                  typeof(*(pos)), member))

#define hlist_for_each_entry_continue_rcu(pos, member)                         \
    for (pos =                                                                 \
             hlist_entry_safe(rcu_dereference(hlist_next_rcu(&(pos)->member)), \
                              typeof(*(pos)), member);                         \
         pos; pos = hlist_entry_safe(                                          \
                  rcu_dereference(hlist_next_rcu(&(pos)->member)),             \
                  typeof(*(pos)), member))

#define hlist_for_each_entry_from_rcu(pos, member)                             \
    for (; pos; pos = hlist_entry_safe(                                        \
                    rcu_dereference(hlist_next_rcu(&(pos)->member)),           \
                    typeof(*(pos)), member))
