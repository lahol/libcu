/** @file cu-types.h
 *  General types, array wrapper, and structured, serialized data (blob) handler.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/** @brief Called to free memory used by an element.
 *  @param[in] 1 The element to destroy.
 */
typedef void (*CUDestroyNotifyFunc)(void *);

/** @brief Compare two elements together with user defined data.
 *  @param[in] 1 The first element.
 *  @param[in] 2 The second element.
 *  @param[in] 3 Pointer to user defined data.
 *  @return A value larger than 0 if the second element is larger than the first,
 *          smaller than 0 if the second element is smaller, and 0 if both are the same.
 */
typedef int (*CUCompareDataFunc)(void *, void *, void *);

/** @brief Compare two elements.
 *  @param[in] 1 The first element.
 *  @param[in] 2 The second element.
 *  @return A value larger than 0 if the second element is larger than the first,
 *          smaller than 0 if the second element is smaller, and 0 if both are the same.
 */
typedef int (*CUCompareFunc)(void *, void *);

/** @brief Callback for key/value pairs.
 *  @param[in] 1 The key.
 *  @param[in] 2 The value.
 *  @param[in] 3 Pointer to user defined data.
 *  @retval true Continue traversing.
 *  @retval false Cancel traversing.
 */
typedef bool (*CUTraverseFunc)(void *, void *, void *);

/** @brief Callback to call for each element.
 *  @param[in] 1 The element data.
 *  @param[in] 2 Pointer to user defined data.
 *  @retval true Continue.
 *  @retval false Stop traversing.
 */
typedef bool (*CUForeachFunc)(void *, void *);

/** @brief Specify elementary types.
 */
typedef enum {
    CU_TYPE_UNKNOWN = 0, /**< Unknown type. */
    CU_TYPE_UINT, /**< Unsigned 32 bit integer. */
    CU_TYPE_INT, /**< Signed 32 bit integer. */
    CU_TYPE_UINT64, /**< Unsigned 64 bit integer. */
    CU_TYPE_INT64, /**< Signed 64 bit integer. */
    CU_TYPE_DOUBLE, /**< Double value. */
    CU_TYPE_POINTER, /**< Pointer. */
    CU_TYPE_STRING, /**< A string. */
    CU_TYPE_ARRAY, /**< A @a CUArray. */
    CU_TYPE_BLOB /**< A @a CUBlob. */
} CUType;

/** @brief A serializable array.
 */
typedef struct {
    CUType member_type; /**< Type of the members. */
    uint32_t length; /**< Number of elements in the array. */
    void *data; /**< Memory allocated by the array. */
} CUArray;

/** @brief Create a new array of a given type.
 *  @param[in] type The member type.
 *  @param[in] length The number of elements the array can contain.
 *  @return Pointer to the newly allocated array.
 */
CUArray *cu_array_new(CUType type, uint32_t length);

/** @brief Duplicate an array.
 *  @param[in] array The original array.
 *  @return Pointer to the newly created duplicate.
 */
CUArray *cu_array_dup(CUArray *array);

/** @brief Destroy an array and free all used resources.
 *  @param[in] array The array to free.
 */
void cu_array_free(CUArray *array);

/** @brief Initialize an array.
 *  @param[in] array Pointer to the array to initialize.
 *  @param[in] type The member type.
 *  @param[in] length The number of elements the array can contain.
 */
void cu_array_init(CUArray *array, CUType type, uint32_t length);

/** @brief Copy an array.
 *  @param[in] dst The destination array.
 *  @param[in] src The source array.
 */
void cu_array_copy(CUArray *dst, CUArray *src);

/** @brief Clear an array
 *  @details Free all used resources but the array itself.
 *  @param[in] array The array to clear.
 */
void cu_array_clear(CUArray *array);

/** @brief Set an unsigned 32 bit value of the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @param[in] value The new value of the element.
 */
void cu_array_set_value_u32(CUArray *array, uint32_t index, uint32_t value);

/** @brief Set a signed 32 bit value of the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @param[in] value The new value of the element.
 */
void cu_array_set_value_i32(CUArray *array, uint32_t index, int32_t value);

/** @brief Set an unsigned 64 bit value of the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @param[in] value The new value of the element.
 */
void cu_array_set_value_u64(CUArray *array, uint32_t index, uint64_t value);

/** @brief Set a signed 64 bit value of the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @param[in] value The new value of the element.
 */
void cu_array_set_value_i64(CUArray *array, uint32_t index, int64_t value);

/** @brief Set a double value of the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @param[in] value The new value of the element.
 */
void cu_array_set_value_double(CUArray *array, uint32_t index, double value);

/** @brief Set a pointer value of the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @param[in] value The new value of the element.
 */
void cu_array_set_value_pointer(CUArray *array, uint32_t index, void *value);

/** @brief Return an unsigned 32 bit value stored in the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @return The value stored at @a index.
 */
uint32_t cu_array_get_value_u32(CUArray *array, uint32_t index);

/** @brief Return a signed 32 bit value stored in the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @return The value stored at @a index.
 */
int32_t cu_array_get_value_i32(CUArray *array, uint32_t index);

/** @brief Return an unsigned 64 bit value stored in the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @return The value stored at @a index.
 */
uint64_t cu_array_get_value_u64(CUArray *array, uint32_t index);

/** @brief Return a signed 64 bit value stored in the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @return The value stored at @a index.
 */
int64_t cu_array_get_value_i64(CUArray *array, uint32_t index);

/** @brief Return a double value stored in the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @return The value stored at @a index.
 */
double cu_array_get_value_double(CUArray *array, uint32_t index);

/** @brief Return a pointer value stored in the array.
 *  @param[in] array The array.
 *  @param[in] index The offset of the element in the array.
 *  @return The value stored at @a index.
 */
void *cu_array_get_value_pointer(CUArray *array, uint32_t index);

/** @brief General purpose compound object. The user is responsible for adding members.
 */
typedef struct _CUBlob CUBlob;

/** @brief Create a new blob.
 *  @return The newly allocated blob.
 */
CUBlob *cu_blob_new(void);

/** @brief Free all resources used by the blob.
 *  @param[in] blob The blob to destroy.
 */
void cu_blob_destroy(CUBlob *blob);

/** @brief Append a value to a blob.
 *  @param[in] blob The blob to append to.
 *  @param[in] type The type of the element to be appended.
 *  @param[in] value Pointer to the value to set.
 */
void cu_blob_append(CUBlob *blob, CUType type, void *value);

/** @brief Serialize a blob.
 *  @param[in,out] buffer Pointer receiving the newly allocated serialized result.
 *  @param[in] buflen The current size of @a buffer.
 *  @param[in] blob The blob to serialize.
 *  @return The resulting size of the buffer.
 */
size_t cu_blob_serialize(char **buffer, size_t buflen, CUBlob *blob);

/** @brief Get a blob from a serialized buffer.
 *  @param[in,out] blob The blob receiving the deserialized data.
 *  @param[in] buffer The buffer containing the serialized data.
 *  @param[in] buflen The length of @a buffer.
 */
void cu_blob_deserialize(CUBlob *blob, char *buffer, size_t buflen);

/** @brief Callback to call for each element in a blob.
 *  @param[in] 1 The blob.
 *  @param[in] 2 The type of the element.
 *  @param[in] 3 Pointer to the element.
 *  @param[in] 4 The size of the element.
 *  @param[in] 5 Pointer to user defined data.
 *  @retval true Continue traversing.
 *  @retval false Stop traversing.
 */
typedef bool (*CUBlobForeachFunc)(CUBlob *, CUType, void *, size_t, void *);

/** @brief Visit every element of a blob.
 *  @param[in] blob The blob to visit elements for.
 *  @param[in] func The callback to call for each element.
 *  @param[in] userdata User defined data.
 */
void cu_blob_foreach(CUBlob *blob, CUBlobForeachFunc func, void *userdata);
