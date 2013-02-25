#ifndef BSON_PARSER_H

#define BSON_PARSER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "hashmap/hashmap.h"

enum BSON_TYPE {
    BSON_DOUBLE     = 0x1,
    BSON_STRING     = 0x2,
    BSON_DOCUMENT   = 0x3,
    BSON_ARRAY      = 0x4,
    BSON_BINARY     = 0x5,
    BSON_UNKNOWN_6  = 0x6,
    BSON_OBJECTID   = 0x7,
    BSON_BOOLEAN    = 0x8,
    BSON_UTCDATETIME = 0x9,
    BSON_NULL       = 0xA,
    BSON_REGEXP     = 0xB,
    BSON_UNKNOWN_12 = 0xC,
    BSON_JSCODE     = 0xD,
    BSON_UNKNOWN_14 = 0xE,
    BSON_JSCODE_W_S = 0xF,
    BSON_INT32      = 0x10,
    BSON_TIMESTAMP  = 0x11,
    BSON_INT64      = 0x12,
    BSON_MAX_KEY    = 0x7F,
    BSON_MIN_KEY    = 0xFF,
};

struct bson_container {
    enum BSON_TYPE type;
    uint32_t data_length;
    union _data {
        double value_double;
        char* value_str;
        char* value_binary;
        map_t* value_document;
        bool value_boolean;
        int32_t value_int32;
        int64_t value_int64;
    } data;
};

typedef struct bson_container* BSON_Container;
typedef map_t* BSON_Object;

BSON_Container create_bson_container(enum BSON_TYPE);
void free_bson_container(BSON_Container);
void free_bson_object(BSON_Object);
int32_t free_map_iter(any_t item, any_t elem);

int32_t parse_bson_document(char* buf, uint32_t* bytes_read, uint32_t buffer_size, BSON_Object result);

#endif /* end of include guard: BSON_PARSER_H */
