#include "cu-avl-tree.h"
#include "cu-memory.h"
#include "cu.h"
#include "cu-fixed-stack.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct _CUAVLTreeNode CUAVLTreeNode;
struct _CUAVLTreeNode {
    void *key;
    void *value;
    CUAVLTreeNode *llink;
    CUAVLTreeNode *rlink;

    uint8_t balance;
};

struct _CUAVLTree {
    CUFixedSizeMemoryPool *node_mem;

    CUAVLTreeNode *root;

    CUCompareDataFunc compare;
    void *compare_data;

    CUDestroyNotifyFunc destroy_key;
    CUDestroyNotifyFunc destroy_value;

    uint32_t height;

    /* Keep stack around for find or delete and do not initialize with every access. */
    uint32_t max_height;
    CUFixedStack node_stack;
};

static int _cu_avl_tree_compare_pointers(void *a, void *b, void *nil)
{
    if (a < b)
        return 1;
    if (a > b)
        return -1;
    return 0;
}

static
CUAVLTreeNode *_cu_avl_tree_alloc(CUFixedSizeMemoryPool *pool)
{
    return (CUAVLTreeNode *)(pool ? cu_fixed_size_memory_pool_alloc(pool) : cu_alloc(sizeof(CUAVLTreeNode)));
}

static
void _cu_avl_tree_free(CUFixedSizeMemoryPool *pool, CUAVLTreeNode *node)
{
    if (pool)
        cu_fixed_size_memory_pool_free(pool, node);
    else
        cu_free(node);
}

static inline
void _cu_avl_tree_node_stack_init(CUAVLTree *tree)
{
    if (tree->height > tree->max_height) {
        cu_fixed_stack_clear(&tree->node_stack);
        tree->max_height = tree->height;
        cu_fixed_pointer_stack_init(&tree->node_stack, tree->max_height);
    }
    else {
        cu_fixed_stack_reset(&tree->node_stack);
    }
}

/* Initialize a tree.
 * compare_data is passed as third argument to compare. 
 * Option to use a fixed size memory pool or use classical alloc/free. */
CUAVLTree *cu_avl_tree_new_full(CUCompareDataFunc compare,
                                void *compare_data,
                                CUDestroyNotifyFunc destroy_key,
                                CUDestroyNotifyFunc destroy_value,
                                bool use_fixed_memory_pool)
{
    CUAVLTree *tree = cu_alloc(sizeof(CUAVLTree));
    if (use_fixed_memory_pool) {
        tree->node_mem = cu_fixed_size_memory_pool_new(sizeof(CUAVLTreeNode), 0);
        cu_fixed_size_memory_pool_release_empty_groups(tree->node_mem, true);
    }
    else {
        tree->node_mem = NULL;
    }

    tree->root = NULL;
    tree->compare = compare ? compare : _cu_avl_tree_compare_pointers;
    tree->compare_data = compare_data;
    tree->destroy_key = destroy_key;
    tree->destroy_value = destroy_value;

    tree->height = 0;

    return tree;
}

CUAVLTree *cu_avl_tree_new(CUCompareDataFunc compare,
                           void *compare_data,
                           CUDestroyNotifyFunc destroy_key,
                           CUDestroyNotifyFunc destroy_value)
{
    return cu_avl_tree_new_full(compare, compare_data, destroy_key, destroy_value, true);
}

void _cu_avl_tree_clear_node(void *key, void *value, CUAVLTree *tree)
{
    if (tree->destroy_key)
        tree->destroy_key(key);
    if (tree->destroy_value)
        tree->destroy_value(value);
}

/* Clear the tree. */
void cu_avl_tree_clear(CUAVLTree *tree)
{
    if (cu_unlikely(!tree))
        return;

    if (tree->destroy_key || tree->destroy_value)
        cu_avl_tree_foreach(tree, (CUTraverseFunc)_cu_avl_tree_clear_node, tree);

    if (tree->node_mem)
        cu_fixed_size_memory_pool_clear(tree->node_mem);

    tree->root = NULL;
}

/* Destroy the tree. */
void cu_avl_tree_destroy(CUAVLTree *tree)
{
    if (cu_unlikely(!tree))
        return;
    cu_avl_tree_clear(tree);
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

#if 0
/* S is the deepest node where an imbalance happend, T it’s parent. Now we insert the node Q. */
static void _cu_avl_tree_rebalance(CUAVLTree *tree, CUAVLTreeNode *S, CUAVLTreeNode *Q, CUAVLTreeNode *T)
{
    int rc = tree->compare(Q->key, S->key, tree->compare_data);
    uint8_t balance = 0;
    CUAVLTreeNode *R = S, *P = S;
    if (rc > 0) { /* Q was inserted in the left subtree of S. */
        balance = BALANCE_LEAN_LEFT;
        R = P = S->llink;
    }
    else if (rc < 0) { /* Q was inserted in the right subtree of S. */
        balance = BALANCE_LEAN_RIGHT;
        R = P = S->rlink;
    }
    /* If neither is tru, we have S=Q and thus P=Q. Since the balance of the node Q is 0, we just
     * increase the height, skipping the next loop. */

    /* Correct the balance to all nodes on the path from S. Before this,
     * the subtrees below S are all balanced, since S was the deepest node where
     * we had imbalance. */
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

    /* The subtree under S is balanced. Since S is the deepest node with an imbalance, all nodes
     * below it were balanced. Inserting the node now unbalances the tree and increases the height. */
    if (S->balance == 0) {
        S->balance = balance;
        ++tree->height;
        return;
    }
    else if ((S->balance | balance) == (BALANCE_LEAN_LEFT | BALANCE_LEAN_RIGHT)) {
        /* The subtree was leaning left and we insert on the right or the other way around.
         * Either way, the subtree is now balanced again and there is nothing left to do. */
        S->balance = 0;
        return;
    }
    else if (S->balance == balance) {
        /* Insert left on a left leaning tree or right on a right leaning tree.
         * Rotate. */
        if (R->balance == balance) {
            /* A8: Single rotation. */
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
            /* R balance is opposite to S balance */
            /* A9: double rotation */
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
 * own_key is needed for create node.
 */
static CUAVLTreeNode *_cu_avl_tree_get_node_for_key(CUAVLTree *tree, void *key, bool create_node, bool own_key)
{
    CUAVLTreeNode *T, *S, *P, *Q, **link;
    T = Q = NULL;
    S = P = tree->root;

    int rc;

    /* Starting at the root, walk down the tree.
     * P is the current parent node.
     * link points to the pointer of the parent node, i.e., represents the arc from P to Q.
     * If a subtree is not balanced, we have T -> S … P -> Q[new], i.e., S is the deepest
     * node where an imbalance happend, and T is it’s parent.
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
             * If own_key is false, we do not own the key. */
            if (own_key && tree->destroy_key && P->key != key)
                tree->destroy_key(key);
            return P;
        }

        /* We are either to the left or to the right of P. */
        if (Q == NULL) { /* We found the right place */
            if (create_node) {
                Q = _cu_avl_tree_alloc(tree->node_mem);
                memset(Q, 0, sizeof(CUAVLTreeNode));
                Q->key = key;
                *link = Q;
                _cu_avl_tree_rebalance(tree, S, Q, T);
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

    /* When we are here, the tree is empty. */
    if (create_node) {
        Q = _cu_avl_tree_alloc(tree->node_mem);
        memset(Q, 0, sizeof(CUAVLTreeNode));
        Q->key = key;
        S = Q;
        tree->root = Q;
        _cu_avl_tree_rebalance(tree, S, Q, T);

        return Q;
    }
    else {
        if (own_key && tree->destroy_key)
            tree->destroy_key(key);
    }

    return NULL;
}
#endif

/* Find the node for a given key and build the stack. */
static
CUAVLTreeNode *_cu_avl_tree_find_node_build_path(CUAVLTree *tree, void *key)
{
    CUAVLTreeNode *node = tree->root;
    int rc;

    _cu_avl_tree_node_stack_init(tree);

    while (node) {
        cu_fixed_pointer_stack_push(&tree->node_stack, node);
        rc = tree->compare(key, node->key, tree->compare_data);
        if (rc > 0) { /* key < node->key, walk left */
            node = node->llink;
        }
        else if (rc < 0) { /* key > node->key, walk right */
            node = node->rlink;
        }
        else
            return node;
    }

    /* The key has not been found. On top of the stack is the potential parent node. */
    return NULL;
}

/* Perform a left rotation on the subtree with root X. Return the new root of the subtree. */
/* FIXME: We know that Z is the right child of X. We could also return the new root in the argument. */
static inline
CUAVLTreeNode *_cu_avl_tree_rotate_left(CUAVLTreeNode *X, CUAVLTreeNode *Z)
{
    /* Exchange X and Z */
    X->rlink = Z->llink;
    Z->llink = X;

    /* Fix balance. */
    if (Z->balance == 0) { /* Only after deletion. */
        X->balance = BALANCE_LEAN_RIGHT;
        Z->balance = BALANCE_LEAN_LEFT;
    }
    else {
        X->balance = 0;
        Z->balance = 0;
    }

    /* Z is the new root. */
    return Z;
}

/* Perform a right rotation on the subtree with root X. Return the new root of the subtree. */
/* FIXME: We know that Z is the left child of X. We could also return the new root in the argument. */
static inline
CUAVLTreeNode *_cu_avl_tree_rotate_right(CUAVLTreeNode *X, CUAVLTreeNode *Z)
{
    /* Exchange X and Z */
    X->llink = Z->rlink;
    Z->rlink = X;

    /* Fix balance. */
    if (Z->balance == 0) { /* Could happen only after deletion. */
        X->balance = BALANCE_LEAN_LEFT;
        Z->balance = BALANCE_LEAN_RIGHT;
    }
    else {
        X->balance = 0;
        Z->balance = 0;
    }

    /* Z is the new root. */
    return Z;
}

/* Perform a right rotation on Z and a left rotation on X. */
static inline
CUAVLTreeNode *_cu_avl_tree_rotate_right_left(CUAVLTreeNode *X, CUAVLTreeNode *Z)
{
    CUAVLTreeNode *Y = Z->llink;
    /* Right rotation around Z. */
    Z->llink = Y->rlink;
    Y->rlink = Z;
    /* Left rotation around X. */
    X->rlink = Y->llink;
    Y->llink = X;

    if (Y->balance == BALANCE_LEAN_LEFT) {
        X->balance = 0;
        Z->balance = BALANCE_LEAN_RIGHT;
    }
    else if (Y->balance == BALANCE_LEAN_RIGHT) {
        X->balance = BALANCE_LEAN_LEFT;
        Z->balance = 0;
    }
    else {
        X->balance = 0;
        Z->balance = 0;
    }

    Y->balance = 0;

    return Y;
}

/* Perform a left rotation on Z and a right rotation on X. */
static inline
CUAVLTreeNode *_cu_avl_tree_rotate_left_right(CUAVLTreeNode *X, CUAVLTreeNode *Z)
{
    CUAVLTreeNode *Y = Z->rlink;
    /* Left rotation around Z. */
    Z->rlink = Y->llink;
    Y->llink = Z;
    /* Right rotation around X. */
    X->llink = Y->rlink;
    Y->rlink = X;

    if (Y->balance == BALANCE_LEAN_RIGHT) {
        X->balance = 0;
        Z->balance = BALANCE_LEAN_LEFT;
    }
    else if (Y->balance == BALANCE_LEAN_LEFT) {
        X->balance = BALANCE_LEAN_RIGHT;
        Z->balance = 0;
    }
    else {
        X->balance = 0;
        Z->balance = 0;
    }

    Y->balance = 0;

    return Y;
}

/* Insert a new element. */
void cu_avl_tree_insert(CUAVLTree *tree,
                        void *key,
                        void *value)
{
#if 0
    CUAVLTreeNode *node = _cu_avl_tree_get_node_for_key(tree, key, true, true);
    node->value = value;
#else
    CUAVLTreeNode *X, *Z, *N, *R;
    /* Find the matching node and build the path to it. */
    Z = _cu_avl_tree_find_node_build_path(tree, key);
    if (Z != NULL) {
        /* The node was already in the tree. Free the key and original value and set the value. */
        if (tree->destroy_key && Z->key != key)
            tree->destroy_key(key);
        if (tree->destroy_value && Z->value != value)
            tree->destroy_value(Z->value);
        Z->value = value; 
        return;
    }

    /* The key was not found in the tree. The head of the stack contains the predecessor of the new node. */
    Z = _cu_avl_tree_alloc(tree->node_mem);
    memset(Z, 0, sizeof(CUAVLTreeNode));
    Z->key = key;
    Z->value = value;

    X = cu_fixed_pointer_stack_peek(&tree->node_stack);
    if (X != NULL) {
        if (tree->compare(key, X->key, tree->compare_data) > 0) {
            /* key < X->key, insert to the left. */
            X->llink = Z;
        }
        else {
            X->rlink = Z;
        }
    }
    else {
        /* There is no node in this tree, i.e., it was empty. Set Z as the new root, increase the height and return. */
        tree->root = Z;
        ++tree->height;
        return;
    }

    /* Find deepest imbalanced node and correct balance on the path from the new node to this one. */
    while (X != NULL && X->balance == 0) {
        if (X->rlink == Z) {
            X->balance = BALANCE_LEAN_RIGHT;
        }
        else {
            X->balance = BALANCE_LEAN_LEFT;
        }

        Z = cu_fixed_pointer_stack_pop(&tree->node_stack);
        X = cu_fixed_pointer_stack_peek(&tree->node_stack);
    }

    /* X is now the (former) deepest imbalanced node, on top of the stack. If X is zero, the height has been increased. */
    if (X == NULL) {
        ++tree->height;
        return;
    }

    /* From here on, the balance of X is either LEFT or RIGHT. Otherwise, we would have continued the former loop
     * up to the point that X == NULL.
     */
    if ((X->rlink == Z && X->balance == BALANCE_LEAN_LEFT) ||
        (X->llink == Z && X->balance == BALANCE_LEAN_RIGHT)) {
        /* This node has one leaf as a child on the opposite side of Z. Thus, this
         * node is now fully balanced, and there is nothing more to do.
         */
        X->balance = 0;
        return;
    }

    /* The imbalance has become too large, we have to apply rotations. However, the subtrees are
     * the same size as they were before the insertion. So the overall height does not change.
     */
    if (X->rlink == Z && X->balance == BALANCE_LEAN_RIGHT) {
        if (Z->balance == BALANCE_LEAN_LEFT)
            N = _cu_avl_tree_rotate_right_left(X, Z);
        else
            N = _cu_avl_tree_rotate_left(X, Z);
    }
    else {
        if (Z->balance == BALANCE_LEAN_RIGHT)
            N = _cu_avl_tree_rotate_left_right(X, Z);
        else
            N = _cu_avl_tree_rotate_right(X, Z);
    }

    cu_fixed_pointer_stack_pop(&tree->node_stack);
    R = cu_fixed_pointer_stack_peek(&tree->node_stack);
    if (R != NULL) {
        /* Place N instead of X on X’s parent node R. */
        if (R->rlink == X)
            R->rlink = N;
        else
            R->llink = N;
    }
    else {
        /* N has become the new root of the whole tree. */
        tree->root = N;
    }
#endif
}

/* Get an element. */
bool cu_avl_tree_find(CUAVLTree *tree,
                      void *key,
                      void **data)
{
    if (cu_unlikely(!tree))
        return false;
    CUAVLTreeNode *node = _cu_avl_tree_find_node_build_path(tree, key);
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
void cu_avl_tree_foreach(CUAVLTree *tree,
                         CUTraverseFunc traverse,
                         void *userdata)
{
    if (!tree || !tree->height || !traverse)
        return;

    _cu_avl_tree_node_stack_init(tree);

    CUAVLTreeNode *node = tree->root;

    while (1) {
        while (node) {
            cu_fixed_pointer_stack_push(&tree->node_stack, node);
            node = node->llink;
        }

        if (!tree->node_stack.length)
            break;

        node = (CUAVLTreeNode *)cu_fixed_pointer_stack_pop(&tree->node_stack);
#ifdef DEBUG_BTREE_DOT
        fprintf(stdout, "n%p [label=\"%u, bal: %u\"];\n", node, CU_POINTER_TO_UINT(node->key), node->balance);
        if (node->llink)
            fprintf(stdout, "n%p -> n%p [label=\"L\"];\n", node, node->llink);
        if (node->rlink)
            fprintf(stdout, "n%p -> n%p [label=\"R\"];\n", node, node->rlink);
#endif
        if (!traverse(node->key, node->value, userdata))
            break;

        node = node->rlink;
    }
}
