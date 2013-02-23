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

int free_map_iter(any_t item, any_t elem) 
{
    if (! elem) {
        return MAP_OK;
    }
    BSON_Container container = (BSON_Container) elem;
    switch(container->type) {
        case BSON_STRING:
            printf("freeing %s\n", container->data.value_str);
            free(container->data.value_str);
            break;
        case BSON_BINARY:
            free(container->data.value_binary);
            break;
        case BSON_DOCUMENT:
        case BSON_ARRAY:
            printf("freeing subdocument\n");
            hashmap_iterate(container->data.value_document, free_map_iter, NULL);
            hashmap_free(container->data.value_document);
            break;
        default:
            break;
    }
    return MAP_OK;
}

int parse_bson_byte(char* buf, uint32_t* bytes_read, unsigned char* result)
{
    if (buf == NULL)
        return -1;
    char* pbuf;
    pbuf = buf + *bytes_read;
    *result = pbuf[0];
    *bytes_read = *bytes_read + 1;
    return 1;
}

int parse_bson_boolean(char* buf, uint32_t* bytes_read, bool* result)
{
    char* pbuf;
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

int parse_bson_int(char* buf, uint32_t* bytes_read, int* result)
{
    char* pbuf;
    pbuf = buf + *bytes_read;
    memcpy(result, pbuf, 4);
    *bytes_read = *bytes_read + 4;
    return 1;
}

/* Returns number of bytes read including the terminating null
 */
int parse_bson_cstring(char* buf, uint32_t *bytes_read, char** result)
{
    char* rbuf;
    char* pbuf;
    char c;
    uint32_t cnt;
    uint32_t buf_size = INITIAL_BUFFER_SIZE;
    pbuf = buf + *bytes_read;
    rbuf = calloc(buf_size, sizeof(char));
    cnt = 0;
    do {
        c = pbuf[cnt];
        if (cnt == buf_size) {
            void* tmp = NULL;
            buf_size = buf_size * 2;
            tmp = realloc(rbuf, buf_size);
            if (tmp == NULL) {
                free(rbuf);
                return -1;
            }
            rbuf = tmp;
        }
        rbuf[cnt] = c;
        cnt++;
    } while(c != '\0');
    *result = rbuf;
    *bytes_read = *bytes_read + cnt; 
    return cnt;
}

int parse_bson_binary(char* buf, uint32_t* bytes_read, char** result)
{
    int32_t bin_length;
    unsigned char bin_subtype;
    char* rbuf;
    char* pbuf = buf + *bytes_read;
    parse_bson_int(buf, bytes_read, &bin_length);
    parse_bson_byte(buf, bytes_read, &bin_subtype);
    rbuf = malloc(bin_length);
    memcpy(rbuf, pbuf, bin_length);
    *result = rbuf;
    *bytes_read = *bytes_read + bin_length;
    return bin_length;
}

int parse_bson_double(char* buf, uint32_t* bytes_read, double* result)
{
    char* pbuf;
    pbuf = buf + *bytes_read;
    memcpy(result, pbuf, 8);
    *bytes_read = *bytes_read + 8;
    return 1;
}

int parse_bson_document(char* buf, uint32_t* bytes_read, BSON_Object result)
{
    int32_t error;
    int32_t doc_len;
    char* key;
    unsigned char value_type;
    uint32_t initial_bytes;
    int32_t value_len;
    double value_double;

    initial_bytes = *bytes_read;
    error = parse_bson_int(buf, bytes_read, &doc_len);
    printf("Document length: %d bytes\n", doc_len);

    while ((*bytes_read - initial_bytes) < doc_len) {
        BSON_Container cont;
        parse_bson_byte(buf, bytes_read, &value_type);
        if (value_type == 0) {
            printf("return");
            return 1;
        }
        parse_bson_cstring(buf, bytes_read, &key);

        printf("type: '%d' key: '%s'\n", value_type, key);

        switch(value_type) {
            case BSON_DOUBLE: 
                printf("double: ");
                parse_bson_double(buf, bytes_read, &value_double);
                printf("%f\n", value_double);
                break;
            case BSON_STRING:
                printf("string\n");
                parse_bson_int(buf, bytes_read, &value_len);
                cont = create_bson_container(BSON_STRING);
                parse_bson_cstring(buf, bytes_read, &cont->data.value_str);
                hashmap_put(result, key, cont);
                printf("%d '%s'\n", value_len, cont->data.value_str);
                break;
            case BSON_ARRAY:
                printf("array\n");
                cont = create_bson_container(BSON_DOCUMENT);
                cont->data.value_document = hashmap_new();
                parse_bson_document(buf, bytes_read, cont->data.value_document);
                hashmap_put(result, key, cont);
                break;
            case BSON_DOCUMENT:
                printf("object\n");
                cont = create_bson_container(BSON_DOCUMENT);
                cont->data.value_document = hashmap_new();
                parse_bson_document(buf, bytes_read, cont->data.value_document);
                hashmap_put(result, key, cont);
                break;
            case BSON_BINARY: 
                printf("binary");
                cont = create_bson_container(BSON_BINARY);
                parse_bson_binary(buf, bytes_read, &cont->data.value_binary);
                hashmap_put(result, key, cont);
                break;
            case BSON_BOOLEAN:
                printf("boolean\n");
                cont = create_bson_container(BSON_BOOLEAN);
                parse_bson_boolean(buf, bytes_read, &cont->data.value_boolean);
                hashmap_put(result, key, cont);
                break;
            case BSON_NULL:
                printf("null\n");
                cont = create_bson_container(BSON_NULL);
                hashmap_put(result, key, cont);
                break;
            case BSON_INT32:
                printf("int32: ");
                cont = create_bson_container(BSON_INT32);
                parse_bson_int(buf, bytes_read, &cont->data.value_int32);
                printf("parsed int: %d\n", cont->data.value_int32);
                hashmap_put(result, key, cont);
                break;
            case BSON_TIMESTAMP:
                printf("timestamp\n");
                break;
            case BSON_INT64:
                printf("int64\n");
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
                printf("unknown type: %d\n", value_type);
                assert(0);
                break;
        }
    }
    return -1;
}

