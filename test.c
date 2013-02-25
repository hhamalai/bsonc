#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "hashmap/hashmap.h"
#include "bson_parser.h"

int main(int argc, const char *argv[])
{
    int fd;
    char buf[200];
    uint32_t bytes_read = 0;
    uint32_t document_size;
    if (argc < 3) {
        printf("./debug file.bson key\n");
        return EXIT_FAILURE;
    }
    memset(buf, 0, 200);
    printf("open %s\n", argv[1]);
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    
    document_size = read(fd, buf,  200);
    BSON_Object map = hashmap_new();
    parse_bson_document(buf, &bytes_read, document_size, map);
    BSON_Container foo;
    if (hashmap_get(map, (char *)argv[2], (any_t)&foo) == MAP_MISSING) {
        printf("Key not found\n");
        free_bson_object(map);
        return EXIT_FAILURE;
    }
    printf("type: %d value: ", foo->type);
    switch(foo->type) {
        case BSON_INT32:
            printf("%d\n", foo->data.value_int32);
            break;
        case BSON_STRING:
            printf("%s\n", foo->data.value_str);
            break;
        default:
            printf("no debug mapping\n");
            break;
    }

    free_bson_object(map);
    return EXIT_SUCCESS;
}
