#include "cu-avl-tree.h"
#include "cu-memory.h"
#include "cu.h"
#include "cu-fixed-stack.h"
#include <stdbool.h>
#include <stdio.h>

/** @brief Balance of a node.
 *  @details Balance is a 2-bit field, 10b means leaning left, 01b means leaning right,
 *           and 00b means balanced.
 */
typedef enum {
    BALANCE_BALANCED = 0, /**< The left and right subtrees have the same height. */
    BALANCE_LEAN_RIGHT = 1, /**< The right subtree has a height of one more than the left. */
    BALANCE_LEAN_LEFT = 2 /**< The left subtree has a height of one more than the right. */
} CUAVLTreeNodeBalance;

typedef struct _CUAVLTreeNode CUAVLTreeNode;
/** @internal
 *  @brief A node in the tree.
 */
struct _CUAVLTreeNode {
    void *key; /**< Pointer to the key of the node, unique in the tree. */
    void *value; /**< Pointer to the value of the node. */
    CUAVLTreeNode *llink; /**< Reference to the left node. */
    CUAVLTreeNode *rlink; /**< Reference to the right node. */

    CUAVLTreeNodeBalance balance;
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

/** @internal 
 *  @brief Compare the raw pointer values.
 *  @details Used as a fallback if no @a compare function is passed to cu_avl_tree_new().
 *  @param[in] a The first value.
 *  @param[in] b The second value.
 *  @param[in] nil Unused.
 *  @retval 1 The value of @a b is larger than @a a.
 *  @retval 0 The values of @a a and @a b are the same.
 *  @retval -1 The value of @a a is larger than @a b.
 */
static
int _cu_avl_tree_compare_pointers(void *a, void *b, __attribute__((unused))void *nil)
{
    if (a < b)
        return 1;
    if (a > b)
        return -1;
    return 0;
}

/** @internal
 *  @brief Wrapper to allocate memory for a single node.
 *  @details If the tree was created with a fixd size memory pool, get the memory from there,
 *           otherwise from cu_alloc.
 *  @param[in] pool The memory pool of the tree, or @a NULL.
 *  @return Pointer to a newly allocated node.
 */
static
CUAVLTreeNode *_cu_avl_tree_alloc(CUFixedSizeMemoryPool *pool)
{
    return (CUAVLTreeNode *)(pool ? cu_fixed_size_memory_pool_alloc(pool) : cu_alloc(sizeof(CUAVLTreeNode)));
}

/** @internal
 *  @brief Wrapper to free memory of a node and return it to the pool, if present.
 *  @param[in] pool The memory pool of the tree, or @a NULL.
 *  @param[in] node The node to free.
 */
static
void _cu_avl_tree_free(CUFixedSizeMemoryPool *pool, CUAVLTreeNode *node)
{
    if (pool)
        cu_fixed_size_memory_pool_free(pool, node);
    else
        cu_free(node);
}

/** @internal
 *  @brief Initialize a stack for the tree, holding at most max height many nodes.
 *  @details If the height of the tree is larger than the current stack, resize the stack,
 *           otherwise just reset the present one.
 *  @param[in] tree The tree for which we initialize the stack.
 */
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

/** @internal
 *  @brief Clear a single node of the tree, freeing resources of key and value.
 *  @param[in] key The key to free.
 *  @param[in] value The value to free.
 *  @param[in] tree The tree in which the node is a member of.
 */
void _cu_avl_tree_clear_node(void *key, void *value, CUAVLTree *tree)
{
    if (tree->destroy_key)
        tree->destroy_key(key);
    if (tree->destroy_value)
        tree->destroy_value(value);
}

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

void cu_avl_tree_destroy(CUAVLTree *tree)
{
    if (cu_unlikely(!tree))
        return;
    cu_avl_tree_clear(tree);
    if (tree->node_mem)
        cu_fixed_size_memory_pool_destroy(tree->node_mem);
    cu_free(tree);
}

/** @internal
 *  @brief Find the node for a given key and build the stack.
 *  @param[in] tree The tree in which we look for the key.
 *  @param[in] key The key we look for.
 *  @return Pointer to the node specified by @a key or @a NULL if not found.
 */
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

/** @internal
 *  @brief Get rightmost child of left subtree.
 *  @param[in] tree The tree.
 *  @param[in] node The node for which we search the predecessor.
 *  @return Pointer to the direct predecessor node.
 */
static
CUAVLTreeNode *_cu_avl_tree_build_path_to_predecessor(CUAVLTree *tree, CUAVLTreeNode *node)
{
    CUAVLTreeNode *N = node->llink;
    while (N) {
        cu_fixed_pointer_stack_push(&tree->node_stack, N);
        N = N->rlink;
    }
    return cu_fixed_pointer_stack_peek(&tree->node_stack);
}

/** @internal
 *  @brief Get leftmost child of right subtree.
 *  @param[in] tree The tree.
 *  @param[in] node The node for which we search the successor.
 *  @return Pointer to the direct successor of the node.
 */
static
CUAVLTreeNode *_cu_avl_tree_build_path_to_successor(CUAVLTree *tree, CUAVLTreeNode *node)
{
    CUAVLTreeNode *N = node->rlink;
    while (N) {
        cu_fixed_pointer_stack_push(&tree->node_stack, N);
        N = N->llink;
    }
    return cu_fixed_pointer_stack_peek(&tree->node_stack);
}

/** @internal
 *  @brief Perform a left rotation on the subtree with root X.
 *  @param[in] X The root of the sub tree.
 *  @param[in] Z The right child of @a X.
 *  @return The new root of the subtree.
 */
/* FIXME: We know that Z is the right child of X. We could also return the new root in the argument. */
static inline
CUAVLTreeNode *_cu_avl_tree_rotate_left(CUAVLTreeNode *X, CUAVLTreeNode *Z)
{
#ifdef DEBUG
    fprintf(stderr, "rotate LEFT\n");
#endif
    /* Exchange X and Z */
    X->rlink = Z->llink;
    Z->llink = X;

    /* Fix balance. */
    if (Z->balance == BALANCE_BALANCED) { /* Only after deletion. */
        X->balance = BALANCE_LEAN_RIGHT;
        Z->balance = BALANCE_LEAN_LEFT;
    }
    else {
        X->balance = BALANCE_BALANCED;
        Z->balance = BALANCE_BALANCED;
    }

    /* Z is the new root. */
    return Z;
}

/** @internal
 *  @brief Perform a right rotation on the subtree with root X.
 *  @param[in] X The root of the sub tree.
 *  @param[in] Z The left child of @a X.
 *  @return The new root of the subtree.
 */
/* FIXME: We know that Z is the left child of X. We could also return the new root in the argument. */
static inline
CUAVLTreeNode *_cu_avl_tree_rotate_right(CUAVLTreeNode *X, CUAVLTreeNode *Z)
{
#ifdef DEBUG
    fprintf(stderr, "rotate RIGHT\n");
#endif
    /* Exchange X and Z */
    X->llink = Z->rlink;
    Z->rlink = X;

    /* Fix balance. */
    if (Z->balance == BALANCE_BALANCED) { /* Could happen only after deletion. */
        X->balance = BALANCE_LEAN_LEFT;
        Z->balance = BALANCE_LEAN_RIGHT;
    }
    else {
        X->balance = BALANCE_BALANCED;
        Z->balance = BALANCE_BALANCED;
    }

    /* Z is the new root. */
    return Z;
}

/** @internal
 *  @brief Perform a right rotation on Z and a left rotation on X.
 *  @param[in] X The root of the subtree.
 *  @param[in] Z The right node of the subtree.
 *  @return The new root of the subtree.
 */
static inline
CUAVLTreeNode *_cu_avl_tree_rotate_right_left(CUAVLTreeNode *X, CUAVLTreeNode *Z)
{
#ifdef DEBUG
    fprintf(stderr, "rotate RIGHT LEFT\n");
#endif
    CUAVLTreeNode *Y = Z->llink;
    /* Right rotation around Z. */
    Z->llink = Y->rlink;
    Y->rlink = Z;
    /* Left rotation around X. */
    X->rlink = Y->llink;
    Y->llink = X;

    if (Y->balance == BALANCE_LEAN_LEFT) {
        X->balance = BALANCE_BALANCED;
        Z->balance = BALANCE_LEAN_RIGHT;
    }
    else if (Y->balance == BALANCE_LEAN_RIGHT) {
        X->balance = BALANCE_LEAN_LEFT;
        Z->balance = BALANCE_BALANCED;
    }
    else {
        X->balance = BALANCE_BALANCED;
        Z->balance = BALANCE_BALANCED;
    }

    Y->balance = BALANCE_BALANCED;

    return Y;
}

/** @internal
 *  @brief Perform a left rotation on Z and a right rotation on X.
 *  @param[in] X The root of the subtree.
 *  @param[in] Z The left node of the subtree.
 *  @return The new root of the subtree.
 */
static inline
CUAVLTreeNode *_cu_avl_tree_rotate_left_right(CUAVLTreeNode *X, CUAVLTreeNode *Z)
{
#ifdef DEBUG
    fprintf(stderr, "rotate LEFT RIGHT\n");
#endif
    CUAVLTreeNode *Y = Z->rlink;
    /* Left rotation around Z. */
    Z->rlink = Y->llink;
    Y->llink = Z;
    /* Right rotation around X. */
    X->llink = Y->rlink;
    Y->rlink = X;

    if (Y->balance == BALANCE_LEAN_RIGHT) {
        X->balance = BALANCE_BALANCED;
        Z->balance = BALANCE_LEAN_LEFT;
    }
    else if (Y->balance == BALANCE_LEAN_LEFT) {
        X->balance = BALANCE_LEAN_RIGHT;
        Z->balance = BALANCE_BALANCED;
    }
    else {
        X->balance = BALANCE_BALANCED;
        Z->balance = BALANCE_BALANCED;
    }

    Y->balance = BALANCE_BALANCED;

    return Y;
}

void cu_avl_tree_insert(CUAVLTree *tree,
                        void *key,
                        void *value)
{
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
    while (X != NULL && X->balance == BALANCE_BALANCED) {
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
        X->balance = BALANCE_BALANCED;
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
}

bool cu_avl_tree_remove(CUAVLTree *tree, void *key)
{
    if (cu_unlikely(!tree))
        return false;

    CUAVLTreeNode *N, *X, *Z;
    /* Find the node containing the key and build the path to it. */
    N = _cu_avl_tree_find_node_build_path(tree, key);
    if (cu_unlikely(N == NULL))
        return false;
    /* We found the node we want to remove. Free key and value. */
    if (tree->destroy_key)
        tree->destroy_key(N->key);
    if (tree->destroy_value)
        tree->destroy_value(N->value);
    /* N can have 0, 1, or 2 children.
     *  0 -> This is a leaf. Delete the parent’s link and free the node.
     *  1 -> The child is a leaf. Set N(key,value) to child(key,value), free child node.
     *  2 -> Get predecessor or successor and build path to it. Move this node’s key/value to N’s key/value.
     *       This one becomes the new N. Now only 0 and 1 are possible.
     */
    if (N->llink && N->rlink) {
        /* We have two children. */
        X = N;
        if (N->balance == BALANCE_LEAN_LEFT)
            N = _cu_avl_tree_build_path_to_predecessor(tree, X);
        else
            N = _cu_avl_tree_build_path_to_successor(tree, X);
        X->key = N->key;
        X->value = N->value;
    }
    /* N is a leaf or a half-leaf and on top of the stack. X its parent. */
    N = cu_fixed_pointer_stack_pop(&tree->node_stack);
    X = cu_fixed_pointer_stack_peek(&tree->node_stack);

    /* In the following, X, the parent of N, will always be on top of the stack. */
    /* N has at most one child Z. Find it. It will become the new child of X in N’s position. */
    if (N->llink)
        Z = N->llink;
    else
        Z = N->rlink;
    if (X) {
        if (X->llink == N)
            X->llink = Z;
        else
            X->rlink = Z;
    }
    else {
        /* If N was the root of the root of the tree. */
        tree->root = Z;
    }
    /* Free the resources of node N. */
    _cu_avl_tree_free(tree->node_mem, N);
    N = Z;

    uint8_t balance;
    while ((X = cu_fixed_pointer_stack_pop(&tree->node_stack)) != NULL) {
        if (X->balance == BALANCE_BALANCED) {
            /* The subtree with root X has been balanced. The subtree with root N has reduced
             * its height, thus making the tree under X leaning left or right, but still in limits.
             * Everything upwards remains unchanged.
             */
            if (X->llink == N)
                X->balance = BALANCE_LEAN_RIGHT;
            else
                X->balance = BALANCE_LEAN_LEFT;

            /* The total height of the tree remains unchanged. */
            return true;
        }
        if ((X->llink == N && X->balance == BALANCE_LEAN_LEFT) ||
            (X->rlink == N && X->balance == BALANCE_LEAN_RIGHT)) {
            /* The tree under N has a smaller height. If N was the child the node X was leaning to,
             * the node X is now balanced but its height also is smaller. We have to continue with
             * X’s parent.
             */
            X->balance = BALANCE_BALANCED;
            N = X;
        }
        else {
            /* The subtree N, which reduced its height, was on the other side to which the node
             * X was leaning to. We have to apply rotations. Z is the other sibling, to which side X is leaning to.
             * If Z was balanced before, we are done.
             */
            if (X->balance == BALANCE_LEAN_LEFT) {
                Z = X->llink;
                balance = Z->balance;
                if (balance == BALANCE_LEAN_RIGHT)
                    N = _cu_avl_tree_rotate_left_right(X, Z);
                else
                    N = _cu_avl_tree_rotate_right(X, Z);
            }
            else {
                Z = X->rlink;
                balance = Z->balance;
                if (balance == BALANCE_LEAN_LEFT)
                    N = _cu_avl_tree_rotate_right_left(X, Z);
                else
                    N = _cu_avl_tree_rotate_left(X, Z);
            }
            /* Z is now used as the parent of X, N is the new root in this rotated tree. */
            Z = cu_fixed_pointer_stack_peek(&tree->node_stack);
            if (Z) {
                if (Z->llink == X)
                    Z->llink = N;
                else
                    Z->rlink = N;
            }
            else {
                /* The loop will terminate in the next iteration. The total height was reduced by one,
                 * unless balance of Z was 0. */
                tree->root = N;
            }

            /* If the node Z was balanced before, we caught the height change and we are done. */
            if (balance == BALANCE_BALANCED)
                return true;
        }
    }

    /* Once we’re here, the height of the whole tree is one smaller. */
    --tree->height;

    return true;
}

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
        fprintf(stdout, "n%p [label=\"%p, bal: %u\"];\n", node, node->key, node->balance);
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
