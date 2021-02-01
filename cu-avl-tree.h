/* A balanced tree. */
#pragma once

#include <cu-types.h>

typedef struct _CUAVLTree CUAVLTree;

/* Initialize a tree.
 * compare_data is passed as third argument to compare.
 * Option to use a fixed size memory pool or use classical alloc/free. */
CUAVLTree *cu_avl_tree_new_full(CUCompareDataFunc compare,
                                void *compare_data,
                                CUDestroyNotifyFunc destroy_key,
                                CUDestroyNotifyFunc destroy_value,
                                bool use_fixed_memory_pool);

/* Initialize a tree.
 * compare_data is passed as third argument to compare. */
CUAVLTree *cu_avl_tree_new(CUCompareDataFunc compare,
                           void *compare_data,
                           CUDestroyNotifyFunc destroy_key,
                           CUDestroyNotifyFunc destroy_value);

/* Clear the tree. */
void cu_avl_tree_clear(CUAVLTree *tree);

/* Destroy the tree. */
void cu_avl_tree_destroy(CUAVLTree *tree);

/* Insert a new element. */
void cu_avl_tree_insert(CUAVLTree *tree,
                        void *key,
                        void *value);

/* Remove an element from the tree. Return true, if the element was in the tree. */
bool cu_avl_tree_remove(CUAVLTree *tree, void *key);

/* Get an element. */
bool cu_avl_tree_find(CUAVLTree *tree,
                      void *key,
                      void **data);

/* Callback for each element in the tree. The tree is processed
 * in order.
 */
void cu_avl_tree_foreach(CUAVLTree *tree,
                         CUTraverseFunc traverse,
                         void *userdata);
