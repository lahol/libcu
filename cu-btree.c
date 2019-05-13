#include "cu-btree.h"
#include "cu-memory.h"
#include "cu.h"
#include "cu-fixed-stack.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct _CUBTreeNode CUBTreeNode;
struct _CUBTreeNode {
    void *key;
    void *value;
    CUBTreeNode *llink;
    CUBTreeNode *rlink;

    uint8_t balance;
};

struct _CUBTree {
    CUFixedSizeMemoryPool *node_mem;

    CUBTreeNode *root;

    CUCompareDataFunc compare;
    void *compare_data;

    CUDestroyNotifyFunc destroy_key;
    CUDestroyNotifyFunc destroy_value;

    uint32_t height;
};

static int _cu_btree_compare_pointers(void *a, void *b, void *nil)
{
    if (a < b)
        return 1;
    if (a > b)
        return -1;
    return 0;
}

/* Initialize a tree.
 * compare_data is passed as third argument to compare. */
CUBTree *cu_btree_new(CUCompareDataFunc compare,
                      void *compare_data,
                      CUDestroyNotifyFunc destroy_key,
                      CUDestroyNotifyFunc destroy_value)
{
    CUBTree *tree = cu_alloc(sizeof(CUBTree)); 
    tree->node_mem = cu_fixed_size_memory_pool_new(sizeof(CUBTreeNode), 0);

    tree->root = NULL;
    tree->compare = compare ? compare : _cu_btree_compare_pointers;
    tree->compare_data = compare_data;
    tree->destroy_key = destroy_key;
    tree->destroy_value = destroy_value;

    tree->height = 0;

    return tree;
}

void _cu_btree_clear_node(void *key, void *value, CUBTree *tree)
{
    if (tree->destroy_key)
        tree->destroy_key(key);
    if (tree->destroy_value)
        tree->destroy_value(value);
}

/* Clear the tree. */
void cu_btree_clear(CUBTree *tree)
{
    if (cu_unlikely(!tree))
        return;

    if (tree->destroy_key || tree->destroy_value)
        cu_btree_foreach(tree, (CUTraverseFunc)_cu_btree_clear_node, tree);

    cu_fixed_size_memory_pool_clear(tree->node_mem);

    tree->root = NULL;
}

/* Destroy the tree. */
void cu_btree_destroy(CUBTree *tree)
{
    if (cu_unlikely(!tree))
        return;
    cu_btree_clear(tree);
    cu_fixed_size_memory_pool_destroy(tree->node_mem);
    cu_free(tree);
}

/* Find the node for a given key. If create_node is true, the node is created if no
 * matching key was found.
 * We are adapting Algorithm A from D.E. Knuth, The Art of Computer Programming, Vol. 3,
 * 6.2.3, pp 462–4.
 * Balance is a 2-bit field, 10b means leaning left, 01b means leaning right, and 00b means
 * balanced.
 */
#define BALANCE_LEAN_LEFT    2
#define BALANCE_LEAN_RIGHT   1
static void _cu_btree_rebalance(CUBTree *tree, CUBTreeNode *S, CUBTreeNode *Q, CUBTreeNode *T, void *key)
{
    int rc = tree->compare(key, S->key, tree->compare_data);
    uint8_t balance = 0;
    CUBTreeNode *R = S, *P = S;
    if (rc > 0) {
        balance = BALANCE_LEAN_LEFT;
        R = P = S->llink;
    }
    else if (rc < 0) {
        balance = BALANCE_LEAN_RIGHT;
        R = P = S->rlink;
    }

    while (P != Q) {
        rc = tree->compare(key, P->key, tree->compare_data);
        if (rc > 0) {
            P->balance = BALANCE_LEAN_LEFT;
            P = P->llink;
        }
        else { /* rc < 0; if rc == 0, key(P) == key(Q), i.e., P == Q */
            P->balance = BALANCE_LEAN_RIGHT;
            P = P->rlink;
        }
    }

    if (S->balance == 0) {
        S->balance = balance;
        ++tree->height;
        return;
    }
    else if ((S->balance | balance) == (BALANCE_LEAN_LEFT | BALANCE_LEAN_RIGHT)) {
        S->balance = 0;
        return;
    }
    else if (S->balance == balance) {
        if (R->balance == balance) {
            /* A8 */
            P = R;
            S->balance = R->balance = 0;
            if (balance == BALANCE_LEAN_LEFT) {
                S->llink = R->rlink;
                R->rlink = S;
            }
            else {
                S->rlink = R->llink;
                R->llink = S;
            }
        }
        else if ((R->balance | balance) == (BALANCE_LEAN_LEFT | BALANCE_LEAN_RIGHT)) {
            /* A9 */
            if (balance == BALANCE_LEAN_LEFT) {
                P = R->rlink;
                R->rlink = P->llink;
                P->llink = R;
                S->llink = P->rlink;
                P->rlink = S;
            }
            else {
                P = R->llink;
                R->llink = P->rlink;
                P->rlink = R;
                S->rlink = P->llink;
                P->llink = S;
            }
            if (P->balance == balance) {
                S->balance = 3 ^ balance;
                R->balance = 0;
            }
            else if (P->balance == 0) {
                S->balance = 0;
                R->balance = 0;
            }
            else {
                S->balance = 0;
                R->balance = balance;
            }
            P->balance = 0;
        }
        if (T) {
            if (T->rlink == S)
                T->rlink = P;
            else
                T->llink = P;
        }
        else
            tree->root = P;
    }

}

static CUBTreeNode *_cu_btree_get_node_for_key(CUBTree *tree, void *key, bool create_node)
{
    CUBTreeNode *T, *S, *P, *Q, **link;
    T = Q = NULL;
    S = P = tree->root;

    int rc;

    while (P) {
        rc = tree->compare(key, P->key, tree->compare_data);
        if (rc > 0) { /* key < P->key */
            Q = P->llink;
            link = &P->llink;
        }
        else if (rc < 0) { /* key > P->key */
            Q = P->rlink;
            link = &P->rlink;
        }
        else {
            return P;
        }

        /* We are either to the left or to the right of P. */
        if (Q == NULL) { /* We found the right place */
            if (create_node) {
                Q = (CUBTreeNode *)cu_fixed_size_memory_pool_alloc(tree->node_mem);
                memset(Q, 0, sizeof(CUBTreeNode));
                Q->key = key;
                *link = Q;
                _cu_btree_rebalance(tree, S, Q, T, key);
            }
            return Q;
        }
        else if (Q->balance != 0) {
            T = P;
            S = Q;
        }
        P = Q;
    };

    if (create_node) {
        Q = (CUBTreeNode *)cu_fixed_size_memory_pool_alloc(tree->node_mem);
        memset(Q, 0, sizeof(CUBTreeNode));
        Q->key = key;
        S = Q;
        tree->root = Q;
        _cu_btree_rebalance(tree, S, Q, T, key);

        return Q;
    }

    return NULL;
}

/* Insert a new element. */
void cu_btree_insert(CUBTree *tree,
                     void *key,
                     void *value)
{
    CUBTreeNode *node = _cu_btree_get_node_for_key(tree, key, true);
    node->value = value;
}

/* Callback for each element in the tree. The tree is processed
 * in order.
 */
void cu_btree_foreach(CUBTree *tree,
                      CUTraverseFunc traverse,
                      void *userdata)
{
    if (!tree || !tree->height || !traverse)
        return;

    CUStack stack;
    cu_stack_init(&stack);

    CUBTreeNode *node = tree->root;

    while (1) {
        while (node) {
            cu_stack_push(&stack, node);
            node = node->llink;
        }

        if (!stack.length)
            goto done;

        node = (CUBTreeNode *)cu_stack_pop(&stack);
        if (traverse(node->key, node->value, userdata))
            goto done;

        node = node->rlink;
    }

done:
    cu_stack_clear(&stack, NULL);

}
