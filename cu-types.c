#include "cu-types.h"
#include "cu-memory.h"
#include <memory.h>
#include "cu.h"

/** @internal
 *  @brief Size of the element types.
 */
static const size_t _cu_element_sizes[] = {
    0, /* CU_TYPE_UNKNOWN */ /**< Size of unknown type. */
    sizeof(uint32_t), /**< Size of unsigned 32 bit integer. */
    sizeof(int32_t), /**< Size of signed 32 bit integer. */
    sizeof(uint64_t), /**< Size of unsigned 64 bit integer. */
    sizeof(int64_t), /**< Size of signed 64 bit integer. */
    sizeof(double), /**< Size of double. */
    sizeof(void *), /**< Size of a pointer. */
    sizeof(char), /**< Size of a character of a string. */
    sizeof(CUArray) /**< Size of an array. */
};

void cu_array_init(CUArray *array, CUType type, uint32_t length)
{
    if (array && type <= CU_TYPE_ARRAY) {
        array->member_type = type;
        array->length = length;
        if (length)
            array->data = cu_alloc0(length * _cu_element_sizes[type]);
        else
            array->data = NULL;
    }
}

CUArray *cu_array_new(CUType type, uint32_t length)
{
    CUArray *array = cu_alloc0(sizeof(CUArray));
    cu_array_init(array, type, length);
    return array;
}

void cu_array_copy(CUArray *dst, CUArray *src)
{
    if (dst == src)
        return;
    if (dst && src) {
        cu_free(dst->data);
        dst->member_type = src->member_type;
        dst->length = src->length;
        if (dst->length) {
            dst->data = cu_alloc(dst->length * _cu_element_sizes[dst->member_type]);
            memcpy(dst->data, src->data, dst->length * _cu_element_sizes[dst->member_type]);
        }
        else
            dst->data = NULL;
    }
    else if (dst) {
        cu_array_init(dst, 0, 0);
    }
}

CUArray *cu_array_dup(CUArray *array)
{
    if (!array)
        return NULL;
    CUArray *result = cu_alloc0(sizeof(CUArray));
    cu_array_copy(result, array);
    return result;
}

void cu_array_clear(CUArray *array)
{
    if (array) {
        cu_free(array->data);
        memset(array, 0, sizeof(CUArray));
    }
}

void cu_array_free(CUArray *array)
{
    cu_array_clear(array);
    cu_free(array);
}

void cu_array_set_value_u32(CUArray *array, uint32_t index, uint32_t value)
{
    if (array && array->member_type == CU_TYPE_UINT && index < array->length)
        ((uint32_t *)array->data)[index] = value;
}

void cu_array_set_value_i32(CUArray *array, uint32_t index, int32_t value)
{
    if (array && array->member_type == CU_TYPE_INT && index < array->length)
        ((int32_t *)array->data)[index] = value;
}

void cu_array_set_value_u64(CUArray *array, uint32_t index, uint64_t value)
{
    if (array && array->member_type == CU_TYPE_UINT64 && index < array->length)
        ((uint64_t *)array->data)[index] = value;
}

void cu_array_set_value_i64(CUArray *array, uint32_t index, int64_t value)
{
    if (array && array->member_type == CU_TYPE_INT64 && index < array->length)
        ((int64_t *)array->data)[index] = value;
}

void cu_array_set_value_double(CUArray *array, uint32_t index, double value)
{
    if (array && array->member_type == CU_TYPE_DOUBLE && index < array->length)
        ((double *)array->data)[index] = value;
}

void cu_array_set_value_pointer(CUArray *array, uint32_t index, void *value)
{
    if (array && array->member_type == CU_TYPE_POINTER && index < array->length)
        ((void **)array->data)[index] = value;
}

uint32_t cu_array_get_value_u32(CUArray *array, uint32_t index)
{
    if (array && array->member_type == CU_TYPE_UINT && index < array->length)
        return ((uint32_t *)array->data)[index];
    return 0;
}

int32_t cu_array_get_value_i32(CUArray *array, uint32_t index)
{
    if (array && array->member_type == CU_TYPE_INT && index < array->length)
        return ((int32_t *)array->data)[index];
    return 0;
}

uint64_t cu_array_get_value_u64(CUArray *array, uint32_t index)
{
    if (array && array->member_type == CU_TYPE_UINT64 && index < array->length)
        return ((uint64_t *)array->data)[index];
    return 0;
}

int64_t cu_array_get_value_i64(CUArray *array, uint32_t index)
{
    if (array && array->member_type == CU_TYPE_INT64 && index < array->length)
        return ((int64_t *)array->data)[index];
    return 0;
}

double cu_array_get_value_double(CUArray *array, uint32_t index)
{
    if (array && array->member_type == CU_TYPE_DOUBLE && index < array->length)
        return ((double *)array->data)[index];
    return 0;
}

void *cu_array_get_value_pointer(CUArray *array, uint32_t index)
{
    if (array && array->member_type == CU_TYPE_POINTER && index < array->length)
        return ((void **)array->data)[index];
    return NULL;
}

/** @internal
 *  @brief Descriptor of a single blob entry.
 */
typedef struct {
    CUType type; /**< The type of the element. */
    size_t offset; /**< The offset in the data. */
} CUBlobEntry;

/** @internal 
 *  @brief Power of the size of a blob chunk.
 */
#define CU_BLOB_CHUNK_POWER 8

/** @internal
 *  @brief Size of a blob chunk.
 */
#define CU_BLOB_CHUNK_SIZE (1 << CU_BLOB_CHUNK_POWER)

/** @internal
 *  @brief Round up to a multiple of chunk sizes.
 *  @param[in] sz The actual size to round up.
 */
#define CU_BLOB_ROUND_TO_CHUNK_SIZE(sz) ((((sz) + ((1 << CU_BLOB_CHUNK_POWER) - 1)) >> CU_BLOB_CHUNK_POWER) << CU_BLOB_CHUNK_POWER)

/** @internal
 *  @brief The blob data.
 */
struct _CUBlob {
    CUList *metadata; /**< Contains [CUBlobEntry] descriptors, stored in reverse order. */
    uint32_t member_count; /**< Number of elements in the blob. */
    size_t alloc_size; /**< Total size available in @a data. */
    size_t used_size; /**< Size currently used of @a data. */
    void *data; /**< Pointer to the memory used by the blob. */
};

CUBlob *cu_blob_new(void)
{
    return cu_alloc0(sizeof(CUBlob));
}

void cu_blob_destroy(CUBlob *blob)
{
    if (cu_unlikely(!blob))
        return;
    cu_list_free_full(blob->metadata, (CUDestroyNotifyFunc)cu_free);
    cu_free(blob->data);
    cu_free(blob);
}

/** @internal
 *  @brief Grow the blob if required to fit the data.
 *  @param[in] blob The blob.
 *  @param[in] required The required size.
 */
static
void _cu_blob_grow_if_needed(CUBlob *blob, size_t required)
{
    if (blob->used_size + required < blob->alloc_size) {
        blob->alloc_size = CU_BLOB_ROUND_TO_CHUNK_SIZE(blob->used_size + required);
        blob->data = cu_realloc(blob->data, blob->alloc_size);
    }
}

/** @internal
 *  @brief Write data to the blob.
 *  @param[in] blob The blob to write the data to.
 *  @param[in] value_type The type of the value.
 *  @param[in] offset The offset in the data part of the blob.
 *  @param[in] value The value to write.
 *  @return The size written.
 */
static
size_t inline _cu_blob_write_value(CUBlob *blob, CUType value_type, size_t offset, void *value)
{
    memcpy(blob->data + offset, value, _cu_element_sizes[value_type]);
    return _cu_element_sizes[value_type];
}

#define _cu_blob_write_uint32(blob, offset, value)\
    (_cu_blob_write_value((blob), CU_TYPE_UINT, (offset), (void *)(value)))
#define _cu_blob_write_uint64(blob, offset, value)\
    (_cu_blob_write_value((blob), CU_TYPE_UINT64, (offset), (void *)(value)))
#define _cu_blob_write_int32(blob, offset, value)\
    (_cu_blob_write_value((blob), CU_TYPE_INT, (offset), (void *)(value)))
#define _cu_blob_write_int64(blob, offset, value)\
    (_cu_blob_write_value((blob), CU_TYPE_INT64, (offset), (void *)(value)))
#define _cu_blob_write_double(blob, offset, value)\
    (_cu_blob_write_value((blob), CU_TYPE_DOUBLE, (offset), (void *)(value)))
#define _cu_blob_write_pointer(blob, offset, value)\
    (_cu_blob_write_value((blob), CU_TYPE_POINTER, (offset), (value)))

/** @internal
 *  @brief Write a string to a blob.
 *  @param[in] blob The blob to write to.
 *  @param[in] offset The offset in the data part of the blob.
 *  @param[in] length The length of the string.
 *  @param[in] value The string to write.
 *  @return The number of bytes written.
 */
static
size_t inline _cu_blob_write_string(CUBlob *blob, size_t offset, uint32_t length, char *value)
{
    memcpy(blob->data + offset, &length, _cu_element_sizes[CU_TYPE_UINT]);
    memcpy(blob->data + offset + _cu_element_sizes[CU_TYPE_UINT], value, length);

    if (ROUND_TO_4(length) > length)
        memset(blob->data + offset + _cu_element_sizes[CU_TYPE_UINT] + length,
               0,
               ROUND_TO_4(length) - length);

    return (_cu_element_sizes[CU_TYPE_UINT] + ROUND_TO_4(length));
}

/** @internal
 *  @brief Write an array to a blob.
 *  @param[in] blob The blob to write to.
 *  @param[in] offset The offset in the data part of the blob.
 *  @param[in] length The length of the array data.
 *  @param[in] type The type of the member values.
 *  @param[in] value Pointer to the array data.
 *  @return The number of bytes written, including padding.
 */
static size_t inline _cu_blob_write_array(CUBlob *blob, size_t offset, uint32_t length, uint32_t type, void *value)
{
    memcpy(blob->data + offset, &type, _cu_element_sizes[CU_TYPE_UINT]);
    memcpy(blob->data + offset + _cu_element_sizes[CU_TYPE_UINT], &length, _cu_element_sizes[CU_TYPE_UINT]);
    memcpy(blob->data + offset + 2 * _cu_element_sizes[CU_TYPE_UINT], value, length);
    if (ROUND_TO_4(length) - length)
        memset(blob->data + offset + 2 * _cu_element_sizes[CU_TYPE_UINT] + length,
               0,
               ROUND_TO_4(length) - length);

    return (2 * _cu_element_sizes[CU_TYPE_UINT] + ROUND_TO_4(length));
}

void cu_blob_append(CUBlob *blob, CUType type, void *value)
{
    if (cu_unlikely(!blob))
        return;

    CUBlobEntry *entry = cu_alloc(sizeof(CUBlobEntry *));
    entry->type = type;
    entry->offset = blob->used_size;

    size_t required = 0;
    uint32_t real_length;

    switch (type) {
        case CU_TYPE_UINT:
        case CU_TYPE_UINT64:
        case CU_TYPE_INT:
        case CU_TYPE_INT64:
        case CU_TYPE_DOUBLE:
        case CU_TYPE_POINTER:
            _cu_blob_grow_if_needed(blob, _cu_element_sizes[type]);
            blob->used_size += _cu_blob_write_value(blob, type, entry->offset, (void *)value);
            break;
        case CU_TYPE_STRING:
            /* 4 byte strlen + string, filled up to multiple of 4 */
            real_length = strlen((char *)value);
            required = 4 + ROUND_TO_4(real_length);
            _cu_blob_grow_if_needed(blob, required);

            blob->used_size += _cu_blob_write_string(blob, entry->offset, real_length, (char *)value);
            break;
        case CU_TYPE_ARRAY:
            /* type, length, data */
            real_length = ((CUArray *)value)->length * _cu_element_sizes[((CUArray *)value)->member_type];
            required = 8 + ROUND_TO_4(real_length);
            _cu_blob_grow_if_needed(blob, required);

            blob->used_size += _cu_blob_write_array(blob, entry->offset, real_length,
                                                    ((CUArray *)value)->member_type,
                                                    ((CUArray *)value)->data);
            break;
        default:
            cu_free(entry);
            return;
    }

    blob->metadata = cu_list_prepend(blob->metadata, entry);
    ++blob->member_count;
}

/* Build a data buffer containing a starting, zero-terminated string with a signature of the
 * blob.
 * u uint32_t
 * i int32_t
 * U uint64_t
 * I int64_t
 * f double
 * p void *
 * s string
 * a array
 */
size_t cu_blob_serialize(char **buffer, size_t buflen, CUBlob *blob)
{
    if (cu_unlikely(!blob || !buffer))
        return 0;

    /* We reserve a multiple of 4 bytes for the signature to ensure some alignment.
     * The signature will be null-terminated. */
    size_t signature_length = ROUND_TO_4(blob->member_count + 1);

    /* Ensure that the buffer is large enough to hold the serialized data. */
    if (buflen < signature_length + blob->used_size) {
        cu_free(*buffer);
        *buffer = cu_alloc(signature_length + blob->used_size);
    }
    buflen = signature_length + blob->used_size;

    /* Initialize the signature with zeros. So the result will be null-terminated and
     * we do not have to care about the overlap. */
    memset(*buffer, 0, signature_length);

    /* Metadata is stored in reverse order. We determine the tail of this list to
     * put it in the right order in the serialized blob. */
    CUList *tail = cu_list_last(blob->metadata);
    CUList *tmp;

    /* Write the signature of the blob. */
    uint32_t j;
    for (tmp = tail, j = 0; tmp; tmp = cu_list_previous(tmp), ++j) {
        switch (((CUBlobEntry *)tmp->data)->type) {
            case CU_TYPE_UINT:    (*buffer)[j] = 'u'; break;
            case CU_TYPE_INT:     (*buffer)[j] = 'i'; break;
            case CU_TYPE_UINT64:  (*buffer)[j] = 'U'; break;
            case CU_TYPE_INT64:   (*buffer)[j] = 'I'; break;
            case CU_TYPE_DOUBLE:  (*buffer)[j] = 'f'; break;
            case CU_TYPE_POINTER: (*buffer)[j] = 'p'; break;
            case CU_TYPE_STRING:  (*buffer)[j] = 's'; break;
            case CU_TYPE_ARRAY:   (*buffer)[j] = 'a'; break;
            default:
                goto error;
        }
    }

    /* Actually, the data is already serialized. We just had to preserve the signature.
     * So, we just copy the rest of the data. */
    memcpy(*buffer + signature_length, blob->data, blob->used_size);

    return buflen;

error:
    return 0;
}

void cu_blob_deserialize(CUBlob *blob, char *buffer, size_t buflen)
{
    /* The signature is a multiple of 4 bytes. Therefore, we read as long as there is no terminating zero.
     * Afterwards, we parse the actual data, especially arrays and strings, which differ in length.
     */
    if (cu_unlikely(!blob || !buffer))
        return;

    /* Determine the signature length. */
    size_t j;
    size_t signature_length;
    for (j = 0; j < buflen && buffer[j] != '\0'; ++j) {
        /* nothing to do, just advance the pointer */
    }
    signature_length = ROUND_TO_4(j + 1);

    /* Initially grow the data buffer to the buffer size, excluding the signature. */
    _cu_blob_grow_if_needed(blob, buflen - signature_length);

    CUBlobEntry *entry;
    blob->used_size = 0;
    void *data = buffer + signature_length;
    size_t size;
    uint32_t length;
    uint32_t type;

    for (j = 0; buffer[j] != '\0'; ++j) {
        entry = cu_alloc(sizeof(CUBlobEntry *));
        entry->offset = blob->used_size;
        switch (buffer[j]) {
            case 'u':
                entry->type = CU_TYPE_UINT;
                size = _cu_blob_write_uint32(blob, entry->offset, data);
                data += size;
                blob->used_size += size;
                break;
            case 'i':
                entry->type = CU_TYPE_INT;
                size = _cu_blob_write_int32(blob, entry->offset, data);
                data += size;
                blob->used_size += size;
                break;
            case 'U':
                entry->type = CU_TYPE_UINT64;
                size = _cu_blob_write_uint64(blob, entry->offset, data);
                data += size;
                blob->used_size += size;
                break;
            case 'I':
                entry->type = CU_TYPE_INT64;
                size = _cu_blob_write_int64(blob, entry->offset, data);
                data += size;
                blob->used_size += size;
                break;
            case 'f':
                entry->type = CU_TYPE_DOUBLE;
                size = _cu_blob_write_double(blob, entry->offset, data);
                data += size;
                blob->used_size += size;
                break;
            case 'p':
                entry->type = CU_TYPE_POINTER;
                size = _cu_blob_write_pointer(blob, entry->offset, data);
                data += size;
                blob->used_size += size;
                break;
            case 's':
                memcpy(&length, data, _cu_element_sizes[CU_TYPE_UINT]);
                size = _cu_blob_write_string(blob, entry->offset, length, data + _cu_element_sizes[CU_TYPE_UINT]);
                data += size;
                blob->used_size += size;
                break;
            case 'a':
                memcpy(&type, data, _cu_element_sizes[CU_TYPE_UINT]);
                memcpy(&length, data + _cu_element_sizes[CU_TYPE_UINT], _cu_element_sizes[CU_TYPE_UINT]);
                size = _cu_blob_write_array(blob, entry->offset, length, type, data + 2 * _cu_element_sizes[CU_TYPE_UINT]);
                data += size;
                blob->used_size += size;
                break;
            default:
                cu_free(entry);
                continue;
        }

        blob->metadata = cu_list_prepend(blob->metadata, entry);
        ++blob->member_count;
    }
}

void cu_blob_foreach(CUBlob *blob, CUBlobForeachFunc func, void *userdata)
{
}
