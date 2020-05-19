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

static
CUBTreeNode *_cu_btree_alloc(CUFixedSizeMemoryPool *pool)
{
    return (CUBTreeNode *)(pool ? cu_fixed_size_memory_pool_alloc(pool) : cu_alloc(sizeof(CUBTreeNode)));
}

static
void _cu_btree_free(CUFixedSizeMemoryPool *pool, CUBTreeNode *node)
{
    if (pool)
        cu_fixed_size_memory_pool_free(pool, node);
    else
        cu_free(node);
}

/* Initialize a tree.
 * compare_data is passed as third argument to compare. 
 * Option to use a fixed size memory pool or use classical alloc/free. */
CUBTree *cu_btree_new_full(CUCompareDataFunc compare,
                           void *compare_data,
                           CUDestroyNotifyFunc destroy_key,
                           CUDestroyNotifyFunc destroy_value,
                           bool use_fixed_memory_pool)
{
    CUBTree *tree = cu_alloc(sizeof(CUBTree));
    if (use_fixed_memory_pool) {
        tree->node_mem = cu_fixed_size_memory_pool_new(sizeof(CUBTreeNode), 0);
        cu_fixed_size_memory_pool_release_empty_groups(tree->node_mem, true);
    }
    else {
        tree->node_mem = NULL;
    }

    tree->root = NULL;
    tree->compare = compare ? compare : _cu_btree_compare_pointers;
    tree->compare_data = compare_data;
    tree->destroy_key = destroy_key;
    tree->destroy_value = destroy_value;

    tree->height = 0;

    return tree;
}

CUBTree *cu_btree_new(CUCompareDataFunc compare,
                      void *compare_data,
                      CUDestroyNotifyFunc destroy_key,
                      CUDestroyNotifyFunc destroy_value)
{
    return cu_btree_new_full(compare, compare_data, destroy_key, destroy_value, true);
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

    if (tree->node_mem)
        cu_fixed_size_memory_pool_clear(tree->node_mem);

    tree->root = NULL;
}

/* Destroy the tree. */
void cu_btree_destroy(CUBTree *tree)
{
    if (cu_unlikely(!tree))
        return;
    cu_btree_clear(tree);
    if (tree->node_mem)
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
static void _cu_btree_rebalance(CUBTree *tree, CUBTreeNode *S, CUBTreeNode *Q, CUBTreeNode *T)
{
    int rc = tree->compare(Q->key, S->key, tree->compare_data);
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
        rc = tree->compare(Q->key, P->key, tree->compare_data);
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

/* own_key: we took responsibility for the key. Thus, we have to destroy it, if we do not use it anymore.
 */
static CUBTreeNode *_cu_btree_get_node_for_key(CUBTree *tree, void *key, bool create_node, bool own_key)
{
    CUBTreeNode *T, *S, *P, *Q, **link;
    T = Q = NULL;
    S = P = tree->root;

    int rc;

    /* Starting at the root, walk down the tree.
     * P is the current parent node.
     * link points to the pointer of the parent node, i.e., represents the arc from P to Q.
     */
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
            /* The key was already set. Thus release the memory.
             * If create_node is false, we do not own the key. */
            if (own_key && tree->destroy_key && P->key != key)
                tree->destroy_key(key);
            return P;
        }

        /* We are either to the left or to the right of P. */
        if (Q == NULL) { /* We found the right place */
            if (create_node) {
                Q = _cu_btree_alloc(tree->node_mem);
                memset(Q, 0, sizeof(CUBTreeNode));
                Q->key = key;
                *link = Q;
                _cu_btree_rebalance(tree, S, Q, T);
            }
            else {
                if (own_key && tree->destroy_key)
                    tree->destroy_key(key);
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
        Q = _cu_btree_alloc(tree->node_mem);
        memset(Q, 0, sizeof(CUBTreeNode));
        Q->key = key;
        S = Q;
        tree->root = Q;
        _cu_btree_rebalance(tree, S, Q, T);

        return Q;
    }
    else {
        if (own_key && tree->destroy_key)
            tree->destroy_key(key);
    }

    return NULL;
}

/* Insert a new element. */
void cu_btree_insert(CUBTree *tree,
                     void *key,
                     void *value)
{
    CUBTreeNode *node = _cu_btree_get_node_for_key(tree, key, true, true);
    node->value = value;
}

/* Get an element. */
bool cu_btree_find(CUBTree *tree,
                   void *key,
                   void **data)
{
    CUBTreeNode *node = _cu_btree_get_node_for_key(tree, key, false, false);
    if (node) {
        if (data)
            *data = node->value;
        return true;
    }
    return false;
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

    static CUFixedStackClass fscls = {
        .element_size = sizeof(CUBTreeNode *),
        .align = 0,
        .clear_func = NULL,
        .copy_func = NULL,
        .setup_proc = NULL,
        .extra_data_size = 0
    };

    CUFixedStack stack;
    cu_fixed_stack_init(&stack, &fscls, tree->height);

    CUBTreeNode *node = tree->root;

    while (1) {
        while (node) {
            *((CUBTreeNode **)cu_fixed_stack_fetch_next(&stack)) = node;
            cu_fixed_stack_push(&stack);
            node = node->llink;
        }

        if (!stack.length)
            goto done;

        node = *((CUBTreeNode **)cu_fixed_stack_pop(&stack));
        if (!traverse(node->key, node->value, userdata))
            goto done;

        node = node->rlink;
    }

done:
    cu_fixed_stack_clear(&stack);
}
