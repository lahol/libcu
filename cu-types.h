#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*CUDestroyNotifyFunc)(void *);

/* a, b, data,
 * return < 0 if a > b, > 0 if a < b */
typedef int (*CUCompareDataFunc)(void *, void *, void *);

/* a, b,
 * return < 0 if a > b, > 0 if a < b, 0 if a == b */
typedef int (*CUCompareFunc)(void *, void *);

/* key, value, data */
typedef bool (*CUTraverseFunc)(void *, void *, void *);

/* data, userdata */
typedef bool (*CUForeachFunc)(void *, void *);

typedef enum {
    CU_TYPE_UNKNOWN = 0,
    CU_TYPE_UINT,
    CU_TYPE_INT,
    CU_TYPE_UINT64,
    CU_TYPE_INT64,
    CU_TYPE_DOUBLE,
    CU_TYPE_POINTER,
    CU_TYPE_STRING,
    CU_TYPE_ARRAY,
    CU_TYPE_BLOB
} CUType;

typedef struct {
    CUType member_type;
    uint32_t length;
    void *data;
} CUArray;

CUArray *cu_array_new(CUType type, uint32_t length);
CUArray *cu_array_dup(CUArray *array);
void cu_array_free(CUArray *array);
void cu_array_init(CUArray *array, CUType type, uint32_t length);
void cu_array_copy(CUArray *dst, CUArray *src);

void cu_array_clear(CUArray *array);

void cu_array_set_value_u32(CUArray *array, uint32_t index, uint32_t value);
void cu_array_set_value_i32(CUArray *array, uint32_t index, int32_t value);
void cu_array_set_value_u64(CUArray *array, uint32_t index, uint64_t value);
void cu_array_set_value_i64(CUArray *array, uint32_t index, int64_t value);
void cu_array_set_value_double(CUArray *array, uint32_t index, double value);
void cu_array_set_value_pointer(CUArray *array, uint32_t index, void *value);

uint32_t cu_array_get_value_u32(CUArray *array, uint32_t index);
int32_t cu_array_get_value_i32(CUArray *array, uint32_t index);
uint64_t cu_array_get_value_u64(CUArray *array, uint32_t index);
int64_t cu_array_get_value_i64(CUArray *array, uint32_t index);
double cu_array_get_value_double(CUArray *array, uint32_t index);
void *cu_array_get_value_pointer(CUArray *array, uint32_t index);

/* General purpose compound object. The user is responsible for adding members. */
typedef struct _CUBlob CUBlob;

void cu_blob_init(CUBlob *blob);
void cu_blob_clear(CUBlob *blob);
void cu_blob_append(CUBlob *blob, CUType type, void *value);
size_t cu_blob_serialize(char **buffer, size_t buflen, CUBlob *blob);
void cu_blob_deserialize(CUBlob *blob, char *buffer, size_t buflen);

/* blob, type, dataptr, size, userdata */
typedef void (*CUBlobForeachFunc)(CUBlob *, CUType, void *, size_t, void *);
void cu_blob_foreach(CUBlob *blob, CUBlobForeachFunc func, void *userdata);
