#ifndef BSON_PARSER_H

#define BSON_PARSER_H

enum BSON_TYPE {
    BSON_UNKNOWN__,
    BSON_DOUBLE,
    BSON_STRING,
    BSON_DOCUMENT,
    BSON_ARRAY,
    BSON_BINARY,
    BSON_UNKNOWN_6,
    BSON_OBJECTID,
    BSON_BOOLEAN,
    BSON_UTCDATETIME,
    BSON_NULL,
    BSON_REGEXP,
    BSON_UNKNOWN_12,
    BSON_JSCODE,
    BSON_UNKNOWN_14,
    BSON_JSCODE_W_S,
    BSON_INT32,
    BSON_TIMESTAMP,
    BSON_INT64
};

struct iotmsg {
};

int parse_bson_document(char* buf, ssize_t len, struct iotmsg* result);

#endif /* end of include guard: BSON_PARSER_H */
