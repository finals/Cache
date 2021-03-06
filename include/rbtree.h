#ifndef __RBTREE_H__
#define __RBTREE_H__

#include <stdint.h>

typedef uintptr_t rbtree_key_t;
typedef intptr_t  rbtree_key_int_t;

typedef struct rbtree_node_s {
    rbtree_key_t    key;
    struct rbtree_node_s *left;
    struct rbtree_node_s *right;
    struct rbtree_node_s *parent;
    int8_t color;
    int8_t data;
}rbtree_node_t;
#define RbtreeNodeSize (sizeof(rbtree_node_t))

typedef void * (*rbtree_insert_pt) (rbtree_node_t *root,
    rbtree_node_t *node, rbtree_node_t *sentinel);

typedef struct rbtree_s {
    rbtree_node_t     *root;
    rbtree_node_t     *sentinel;
    rbtree_insert_pt   insert;
}rbtree_t;
#define RbtreeSize (sizeof(rbtree_t))

#define rbtree_init(tree, s, i)                                               \
    rbtree_sentinel_init(s);                                                  \
    (tree)->root = s;                                                         \
    (tree)->sentinel = s;                                                     \
    (tree)->insert = i

void * rbtree_insert(volatile rbtree_t *tree, rbtree_node_t *node);
void rbtree_delete(volatile rbtree_t *tree, rbtree_node_t *node);
void rbtree_insert_value(rbtree_node_t *root, rbtree_node_t *node,
    rbtree_node_t *sentinel);
void rbtree_insert_timer_value(rbtree_node_t *root,
    rbtree_node_t *node, rbtree_node_t *sentinel);

#define rbt_red(node)               ((node)->color = 1)
#define rbt_black(node)             ((node)->color = 0)
#define rbt_is_red(node)            ((node)->color)
#define rbt_is_black(node)          (!rbt_is_red(node))
#define rbt_copy_color(n1, n2)      (n1->color = n2->color)


/* a sentinel must be black */
#define rbtree_sentinel_init(node)  rbt_black(node)

static inline rbtree_node_t *
rbtree_min(rbtree_node_t *node, rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }

    return node;
}

#endif /* __RBTREE_H__ */