#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include "hashmap/hashmap.h"
#include "bson_parser.h"

int main(int argc, const char *argv[])
{
    int fd;
    char buf[200];
    uint32_t bytes_read;
    memset(buf, 0, 200);
    printf("open %s\n", argv[1]);
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    read(fd, buf,  200);
    map_t* map = hashmap_new();

    parse_bson_document(buf, &bytes_read, map);

    return EXIT_SUCCESS;
}
