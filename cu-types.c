#include "cu-types.h"
#include "cu-memory.h"
#include <memory.h>
#include "cu.h"

static size_t _cu_element_sizes[] = {
    0, /* CU_TYPE_UNKNOWN */
    sizeof(uint32_t),
    sizeof(int32_t),
    sizeof(uint64_t),
    sizeof(int64_t),
    sizeof(double),
    sizeof(void *),
    sizeof(char),
    sizeof(CUArray)
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

void cu_array_copy(CUArray *dst, CUArray *src)
{
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

void cu_array_clear(CUArray *array)
{
    if (array) {
        cu_free(array->data);
        memset(array, 0, sizeof(CUArray));
    }
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

typedef struct {
    CUType type;
    size_t offset;
} CUBlobEntry;

#define CU_BLOB_CHUNK_POWER 8
#define CU_BLOB_CHUNK_SIZE (1 << CU_BLOB_CHUNK_POWER) 
#define CU_BLOB_ROUND_TO_CHUNK_SIZE(sz) ((((sz) + ((1 << CU_BLOB_CHUNK_POWER) - 1)) >> CU_BLOB_CHUNK_POWER) << CU_BLOB_CHUNK_POWER)

struct _CUBlob {
    CUList *metadata; /* [CUBlobEntry], stored in reverse order */
    uint32_t member_count;
    size_t alloc_size;
    size_t used_size;
    void *data;
};

void cu_blob_init(CUBlob *blob)
{
    if (blob) {
        memset(blob, 0, sizeof(CUBlob));
    }
}

void cu_blob_clear(CUBlob *blob)
{
    if (cu_unlikely(!blob))
        return;
    cu_list_free_full(blob->metadata, (CUDestroyNotifyFunc)cu_free);
    cu_free(blob->data);
    memset(blob, 0, sizeof(CUBlob));
}

static void _cu_blob_grow_if_needed(CUBlob *blob, size_t required)
{
    if (blob->used_size + required < blob->alloc_size) {
        blob->alloc_size = CU_BLOB_ROUND_TO_CHUNK_SIZE(blob->used_size + required);
        blob->data = cu_realloc(blob->data, blob->alloc_size);
    }
}

static size_t inline _cu_blob_write_uint32(CUBlob *blob, size_t offset, uint32_t *value)
{
    memcpy(blob->data + offset, value, 4);
    return 4;
}

static size_t inline _cu_blob_write_uint64(CUBlob *blob, size_t offset, uint64_t *value)
{
    memcpy(blob->data + offset, value, 8);
    return 8;
}

static size_t inline _cu_blob_write_int32(CUBlob *blob, size_t offset, int32_t *value)
{
    memcpy(blob->data + offset, value, 4);
    return 4;
}

static size_t inline _cu_blob_write_int64(CUBlob *blob, size_t offset, int64_t *value)
{
    memcpy(blob->data + offset, value, 8);
    return 8;
}

static size_t inline _cu_blob_write_double(CUBlob *blob, size_t offset, double *value)
{
    memcpy(blob->data + offset, value, 8);
    return 8;
}

static size_t inline _cu_blob_write_pointer(CUBlob *blob, size_t offset, void *value)
{
    memcpy(blob->data + offset, &value, sizeof(void *));
    return sizeof(void *);
}

static size_t inline _cu_blob_write_string(CUBlob *blob, size_t offset, uint32_t length, char *value)
{
    memcpy(blob->data + offset, &length, 4);
    memcpy(blob->data + offset + 4, value, length);

    if (ROUND_TO_4(length) > length)
        memset(blob->data + offset + 4 + length, 0, ROUND_TO_4(length) - length);

    return (4 + ROUND_TO_4(length));
}

static size_t inline _cu_blob_write_array(CUBlob *blob, size_t offset, uint32_t length, uint32_t type, void *value)
{
    memcpy(blob->data + offset, &type, 4);
    memcpy(blob->data + offset + 4, &length, 4);
    memcpy(blob->data + offset + 8, value, length);
    if (ROUND_TO_4(length) - length)
        memset(blob->data + offset + 8 + length, 0, ROUND_TO_4(length) - length);

    return (8 + ROUND_TO_4(length));
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
            _cu_blob_grow_if_needed(blob, 4);
            blob->used_size += _cu_blob_write_uint32(blob, entry->offset, value);
            break;
        case CU_TYPE_UINT64:
            _cu_blob_grow_if_needed(blob, 8);
            blob->used_size += _cu_blob_write_uint64(blob, entry->offset, value);
            break;
        case CU_TYPE_INT:
            _cu_blob_grow_if_needed(blob, 4);
            blob->used_size += _cu_blob_write_int32(blob, entry->offset, value);
            break;
        case CU_TYPE_INT64:
            _cu_blob_grow_if_needed(blob, 8);
            blob->used_size += _cu_blob_write_int64(blob, entry->offset, value);
            break;
        case CU_TYPE_DOUBLE:
            _cu_blob_grow_if_needed(blob, 8);
            blob->used_size += _cu_blob_write_double(blob, entry->offset, value);
            break;
        case CU_TYPE_POINTER:
            _cu_blob_grow_if_needed(blob, sizeof(void *));
            blob->used_size += _cu_blob_write_pointer(blob, entry->offset, value);
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
                memcpy(&length, data, 4);
                size = _cu_blob_write_string(blob, entry->offset, length, data + 4);
                data += size;
                blob->used_size += size;
                break;
            case 'a':
                memcpy(&type, data, 4);
                memcpy(&length, data + 4, 4);
                size = _cu_blob_write_array(blob, entry->offset, length, type, data + 8);
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
