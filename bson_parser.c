#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#include "bson_parser.h"

#define INITIAL_BUFFER_SIZE 2

int parse_bson_byte(char* buf, uint32_t *bytes_read, char* result)
{
    if (buf == NULL)
        return -1;
    *result = buf[0];
    *bytes_read = *bytes_read + 1;
    return 1;
}

int parse_bson_int(char* buf, uint32_t *bytes_read, int* result)
{
    int32_t a,b,c,d;
    a = buf[0]; b = buf[1]; c = buf[2]; d = buf[3];
    *result = a + (b << 8) + (c << 16) + (d << 24);
    *bytes_read = *bytes_read + 4;
    return 1;
}

/* Returns number of bytes read including the terminating null
 */
int parse_bson_cstring(char* buf, uint32_t *bytes_read, char** result)
{
    char* rbuf;
    char c;
    uint32_t cnt;
    uint32_t buf_size = INITIAL_BUFFER_SIZE;
    rbuf = calloc(buf_size, sizeof(char));
    cnt = 0;
    do {
        c = buf[cnt];
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

int parse_bson_double(char* buf, uint32_t* bytes_read, double* result)
{
    /* TODO */
    *bytes_read = 8;
}

int parse_bson_array(char* buf, ssize_t len, struct iomsg* result)
{
    
}

int parse_bson_document(char* buf, ssize_t len, struct iotmsg* result)
{
    char *pbuf;
    int32_t error;
    uint32_t total_bytes_read;
    uint32_t bytes_read;

    uint32_t doc_len;
    char* key;
    char value_type;
    char* value_str;
    uint32_t value_int32;
    uint32_t value_len;
    double value_double;

    pbuf = (char *) buf;
    error = parse_bson_int(pbuf, &bytes_read, &doc_len);
    pbuf = buf + bytes_read;
    printf("Document length: %d bytes\n", doc_len);

    while (bytes_read < doc_len) {
        parse_bson_byte(pbuf, &bytes_read, &value_type);
        pbuf = buf + bytes_read;
        if (value_type == 0) {
            return;
        }
        parse_bson_cstring(pbuf, &bytes_read, &key);
        pbuf = buf + bytes_read;
        printf("type: '%d' key: '%s'\n", value_type, key);
        switch(value_type) {
            case BSON_DOUBLE: 
                printf("double\n");
                parse_bson_double(pbuf, &bytes_read, &value_double);
                pbuf = buf + bytes_read;
                break;
            case BSON_STRING:
                printf("string\n");
                parse_bson_int(pbuf, &bytes_read, &value_len);
                pbuf = buf + bytes_read;
                parse_bson_cstring(pbuf, &bytes_read, &value_str);
                pbuf = buf + bytes_read;
                printf("%d '%s'\n", value_len, value_str);
                break;
            /* Arrays and objects are */
            case BSON_ARRAY:
                parse_bson_array(pbuf, len - bytes_read, result);
            case BSON_DOCUMENT:
                printf("object or array\n");
                parse_bson_document(pbuf, len - bytes_read, result);
                break;
            case BSON_BINARY: 
                printf("binary");
                break;
            case BSON_BOOLEAN:
                printf("boolean\n");
                break;
            case BSON_NULL:
                printf("null\n");
                break;
            case BSON_INT32:
                printf("int32\n");
                parse_bson_int(pbuf, &bytes_read, &value_int32);
                pbuf = buf + bytes_read;
                break;
            case BSON_TIMESTAMP:
                printf("timestamp\n");
                break;
            case BSON_INT64:
                printf("int64\n");
                break;
            default:
                assert(0);
                break;
        }


    }
}

int main(int argc, const char *argv[])
{
    int fd;
    char buf[200];
    memset(buf, 0, 200);
    printf("open %s\n", argv[1]);
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    read(fd, buf,  20);
    parse_bson_document(buf, 20, NULL);
    return EXIT_SUCCESS;
}
