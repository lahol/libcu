/* A balanced tree. */
#pragma once

#include <cu-types.h>

typedef struct _CUBTree CUBTree;


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

/* Callback for each element in the tree. The tree is processed
 * in order.
 */
void cu_btree_foreach(CUBTree *tree,
                      CUTraverseFunc traverse,
                      void *userdata);
