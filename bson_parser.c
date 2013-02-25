#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#include "hashmap/hashmap.h"
#include "bson_parser.h"

#define INITIAL_BUFFER_SIZE 2

struct bson_container* create_bson_container(enum BSON_TYPE type)
{
    struct bson_container* result = calloc(sizeof(struct bson_container), 1);
    result->type = type;
    return result;
}

void free_bson_object(BSON_Object object)
{
    hashmap_iterate(object, free_map_iter, NULL);
    free(object);
}

int32_t free_map_iter(any_t item, any_t elem) 
{
    if (! elem) {
        return MAP_OK;
    }
    BSON_Container container = (BSON_Container) elem;
    switch(container->type) {
        case BSON_STRING:
            printf("freeing str");
            free(container->data.value_str);
            break;
        case BSON_BINARY:
            free(container->data.value_binary);
            break;
        case BSON_DOCUMENT:
        case BSON_ARRAY:
            hashmap_iterate(container->data.value_document, free_map_iter, NULL);
            hashmap_free(container->data.value_document);
            break;
        default:
            break;
    }
    free(container);
    return MAP_OK;
}

int32_t parse_bson_byte(char* buf, uint32_t* bytes_read, uint32_t buf_size, uint8_t* result)
{
    char* pbuf;
    if (*bytes_read + 1 > buf_size) {
        return -1;
    }
    pbuf = buf + *bytes_read;
    *result = pbuf[0];
    *bytes_read = *bytes_read + 1;
    return 1;
}

int32_t parse_bson_boolean(char* buf, uint32_t* bytes_read, uint32_t buf_size, bool* result)
{
    char* pbuf;
    if (*bytes_read + 1 > buf_size) {
        return -1;
    }
    pbuf = buf + *bytes_read;
    if (pbuf[0] == 0) {
        *result = false;
    } else if (pbuf[0] == 1) {
        *result = true;
    } else {
        return -1;
    }
    return 1;
}

int32_t parse_bson_int32(char* buf, uint32_t* bytes_read, uint32_t buf_size, int32_t* result)
{
    char* pbuf;
    if (*bytes_read + 4 > buf_size) {
        return -1;
    }
    pbuf = buf + *bytes_read;
    memcpy(result, pbuf, 4);
    *bytes_read = *bytes_read + 4;
    return 1;
}

int32_t parse_bson_int64(char* buf, uint32_t* bytes_read, uint32_t buf_size, int64_t* result)
{
    char* pbuf;
    if (*bytes_read + 8 > buf_size) {
        return -1;
    }
    pbuf = buf + *bytes_read;
    memcpy(result, pbuf, 8);
    *bytes_read = *bytes_read + 8;
    return 1;
}

/* Returns number of bytes read including the terminating null
 */
int32_t parse_bson_cstring(char* buf, uint32_t *bytes_read, uint32_t buf_size, char** result)
{
    char* rbuf;
    char* pbuf;
    char c;
    uint32_t cnt;
    uint32_t cur_buf_size = INITIAL_BUFFER_SIZE;
    pbuf = buf + *bytes_read;
    rbuf = calloc(cur_buf_size, sizeof(char));
    cnt = 0;
    do {
        c = pbuf[cnt];
        if (cnt == cur_buf_size) {
            void* tmp = NULL;
            cur_buf_size = cur_buf_size * 2;
            tmp = realloc(rbuf, cur_buf_size);
            if (tmp == NULL) {
                free(rbuf);
                return -1;
            }
            rbuf = tmp;
        }
        rbuf[cnt] = c;
        cnt++;
        if (cnt > buf_size) {
            if (rbuf) {
                free(rbuf);
            }
            return -1;
        }
    } while(c != '\0');
    *result = rbuf;
    *bytes_read = *bytes_read + cnt; 
    return cnt;
}

int32_t parse_bson_binary(char* buf, uint32_t* bytes_read, uint32_t buf_size, char** result)
{
    int32_t bin_length;
    int32_t rval;
    uint8_t bin_subtype;
    char* rbuf;
    char* pbuf = buf + *bytes_read;

    rval = parse_bson_int32(buf, bytes_read, buf_size, &bin_length);
    if (rval < 0) return -1;
    if (*bytes_read + sizeof(bin_subtype) + bin_length > buf_size) {
        return -1;
    }
    rval = parse_bson_byte(buf, bytes_read, buf_size, &bin_subtype);
    if (rval < 0) return -1;
    rbuf = malloc(bin_length);
    memcpy(rbuf, pbuf, bin_length);
    *result = rbuf;
    *bytes_read = *bytes_read + bin_length;
    return bin_length;
}

int32_t parse_bson_double(char* buf, uint32_t* bytes_read, uint32_t buf_size, double* result)
{
    char* pbuf;
    if (*bytes_read + 8 > buf_size) {
        return -1;
    }
    pbuf = buf + *bytes_read;
    memcpy(result, pbuf, 8);
    *bytes_read = *bytes_read + 8;
    return 1;
}

int32_t parse_bson_document(char* buf, uint32_t* bytes_read, uint32_t buf_size, BSON_Object result)
{
    int32_t rval = 0;
    int32_t doc_len = 0;
    char* key;
    uint8_t value_type;
    int32_t initial_bytes;
    int32_t value_len;

    initial_bytes = *bytes_read;
    rval = parse_bson_int32(buf, bytes_read, buf_size, &doc_len);
    if (rval < 0) {
        return -1;
    }

    while (( (*bytes_read - initial_bytes) < doc_len ) && /* guard check for valid encoding */
          (*bytes_read < buf_size)) { /* guard check to prevent buffer over reads */
        BSON_Container cont;

        rval = parse_bson_byte(buf, bytes_read, buf_size, &value_type);
        if (rval < 0)  break;

        if (value_type == 0) {
            return 0;
        }

        rval = parse_bson_cstring(buf, bytes_read, buf_size, &key);
        if (rval < 0) break;

        switch(value_type) {
            case BSON_DOUBLE:
                cont = create_bson_container(BSON_DOUBLE);
                rval = parse_bson_double(buf, bytes_read, buf_size, &cont->data.value_double);
                if (rval < 0) break;
                hashmap_put(result, key, cont);
                break;
            case BSON_STRING:
                cont = create_bson_container(BSON_STRING);
                rval = parse_bson_int32(buf, bytes_read, buf_size, &value_len);
                if (rval < 0) break;
                rval = parse_bson_cstring(buf, bytes_read, buf_size, &cont->data.value_str);
                if (rval < 0) break;
                cont->data_length = rval;
                hashmap_put(result, key, cont);
                break;
            case BSON_ARRAY:
                cont = create_bson_container(BSON_DOCUMENT);
                cont->data.value_document = hashmap_new();
                rval = parse_bson_document(buf, bytes_read, buf_size, cont->data.value_document);
                if (rval < 0) break;
                hashmap_put(result, key, cont);
                break;
            case BSON_DOCUMENT:
                cont = create_bson_container(BSON_DOCUMENT);
                cont->data.value_document = hashmap_new();
                rval = parse_bson_document(buf, bytes_read, buf_size, cont->data.value_document);
                if (rval < 0) break;
                hashmap_put(result, key, cont);
                break;
            case BSON_BINARY: 
                cont = create_bson_container(BSON_BINARY);
                rval = parse_bson_binary(buf, bytes_read, buf_size, &cont->data.value_binary);
                if (rval < 0) break;
                cont->data_length = rval;
                hashmap_put(result, key, cont);
                break;
            case BSON_BOOLEAN:
                cont = create_bson_container(BSON_BOOLEAN);
                rval = parse_bson_boolean(buf, bytes_read, buf_size, &cont->data.value_boolean);
                if (rval < 0) break;
                hashmap_put(result, key, cont);
                break;
            case BSON_NULL:
                cont = create_bson_container(BSON_NULL);
                hashmap_put(result, key, cont);
                break;
            case BSON_INT32:
                cont = create_bson_container(BSON_INT32);
                rval = parse_bson_int32(buf, bytes_read, buf_size, &cont->data.value_int32);
                if (rval < 0) break;
                hashmap_put(result, key, cont);
                break;
            case BSON_UTCDATETIME:
                cont = create_bson_container(BSON_UTCDATETIME);
                rval = parse_bson_int64(buf,bytes_read, buf_size, &cont->data.value_int64);
                if (rval < 0) break;
                hashmap_put(result, key, cont);
                break;
            case BSON_TIMESTAMP:
                cont = create_bson_container(BSON_INT64);
                rval = parse_bson_int64(buf, bytes_read, buf_size, &cont->data.value_int64);
                if (rval < 0) break;
                hashmap_put(result, key, cont);
                break;
            case BSON_INT64:
                cont = create_bson_container(BSON_INT64);
                rval = parse_bson_int64(buf, bytes_read, buf_size, &cont->data.value_int64);
                if (rval < 0) break;
                hashmap_put(result, key, cont);
                break;
            case BSON_MIN_KEY:
                cont = create_bson_container(BSON_MIN_KEY);
                hashmap_put(result, key, cont);
                break;
            case BSON_MAX_KEY:
                cont = create_bson_container(BSON_MAX_KEY);
                hashmap_put(result, key, cont);
                break;
            default:
                rval = -1; /* Parser error*/
                break;
        }
        if (rval < 0) {
            break;
        }
    }
    return rval;
}

