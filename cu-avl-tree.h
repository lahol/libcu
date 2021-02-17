/** @file cu-avl-tree.h
 *  Provide binary tree that is always balanced.
 */
#pragma once

#include <cu-types.h>

/** @brief Handle to an AVL tree.
 *  @details An AVL tree is a binary tree, that is always balanced.
 */
typedef struct _CUAVLTree CUAVLTree;

/** @brief Create a new AVL tree, with full control.
 *  @param[in] compare Pointer to a function that compares two keys. If not specified,
 *                     the pointer values are compared.
 *  @param[in] compare_data User defined data passed as third argument to @a compare.
 *  @param[in] destroy_key Function to free resources used by the key.
 *  @param[in] destroy_value Function to free resources used by the value.
 *  @param[in] use_fixed_memory_pool Whether to use a fixed size memory pool or cu_alloc()/cu_free().
 *  @return Pointer to a newly created AVL tree.
 */
CUAVLTree *cu_avl_tree_new_full(CUCompareDataFunc compare,
                                void *compare_data,
                                CUDestroyNotifyFunc destroy_key,
                                CUDestroyNotifyFunc destroy_value,
                                bool use_fixed_memory_pool);

/** @brief Create a new AVL tree with fixed sized memory pool.
 *  @param[in] compare Pointer to a function that compares two keys. If not specified,
 *                     the pointer values are compared.
 *  @param[in] compare_data User defined data passed as third argument to compare.
 *  @param[in] destroy_key Function to free resources used by the key.
 *  @param[in] destroy_value Function to free resources used by the value.
 *  @return Pointer to a newly created AVL tree.
 */
CUAVLTree *cu_avl_tree_new(CUCompareDataFunc compare,
                           void *compare_data,
                           CUDestroyNotifyFunc destroy_key,
                           CUDestroyNotifyFunc destroy_value);

/** @brief Clear an AVL tree and free resources of keys/values.
 *  @details Only the keys and values are destroyed. The tree is still initialized
 *           and may be used further.
 *  @param[in] tree The tree to clear.
 */
void cu_avl_tree_clear(CUAVLTree *tree);

/** @brief Destroy an AVL tree and free all resources.
 *  @param[in] tree The tree to destroy.
 */
void cu_avl_tree_destroy(CUAVLTree *tree);

/** @brief Insert a new element into a tree
 *  @details If an element with the given @a key is already in the tree, the @a key and the old
 *           value are destroyed. Ownership is passed to the tree.
 *  @param[in] tree The tree.
 *  @param[in] key The key of the element.
 *  @param[in] value The value to be inserted for @a key.
 */
void cu_avl_tree_insert(CUAVLTree *tree,
                        void *key,
                        void *value);

/** @brief Remove an element from the tree and free its resources.
 *  @param[in] tree The tree.
 *  @param[in] key The key of the element to destroy.
 *  @retval true The element was present in the tree and was destroyed.
 *  @retval false The element was not found in the tree.
 */
bool cu_avl_tree_remove(CUAVLTree *tree, void *key);

/** @brief Find an element in the tree.
 *  @param[in] tree The tree.
 *  @param[in] key The key of the element we search.
 *  @param[out] data Gets filled with a pointer to the value, if @a key was found.
 *  @retval true The element specified by @a key was found.
 *  @retval false The element is not in the tree.
 */
bool cu_avl_tree_find(CUAVLTree *tree,
                      void *key,
                      void **data);

/** @brief Call a function for each element in the tree.
 *  @details The tree is processed in order.
 *  @param[in] tree The tree.
 *  @param[in] traverse Function to call for each element.
 *  @param[in] userdata Pointer passed as third argument to \a traverse.
 */
void cu_avl_tree_foreach(CUAVLTree *tree,
                         CUTraverseFunc traverse,
                         void *userdata);
