/* A balanced tree. */
#pragma once

#include <cu-types.h>

typedef struct _CUBTree CUBTree;

/* Initialize a tree.
 * compare_data is passed as third argument to compare. 
 * Option to use a fixed size memory pool or use classical alloc/free. */
CUBTree *cu_btree_new_full(CUCompareDataFunc compare,
                           void *compare_data,
                           CUDestroyNotifyFunc destroy_key,
                           CUDestroyNotifyFunc destroy_value,
                           bool use_fixed_memory_pool);

/* Initialize a tree.
 * compare_data is passed as third argument to compare. */
CUBTree *cu_btree_new(CUCompareDataFunc compare,
                      void *compare_data,
                      CUDestroyNotifyFunc destroy_key,
                      CUDestroyNotifyFunc destroy_value);

/* Clear the tree. */
void cu_btree_clear(CUBTree *tree);

/* Destroy the tree. */
void cu_btree_destroy(CUBTree *tree);

/* Insert a new element. */
void cu_btree_insert(CUBTree *tree,
                     void *key,
                     void *value);

/* Get an element. */
bool cu_btree_find(CUBTree *tree,
                   void *key,
                   void **data);

/* Callback for each element in the tree. The tree is processed
 * in order.
 */
void cu_btree_foreach(CUBTree *tree,
                      CUTraverseFunc traverse,
                      void *userdata);
