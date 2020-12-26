#ifndef HTTPDEF_H
#define HTTPDEF_H

typedef enum {
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
    PATCH
} http_method_t;

typedef enum {
    SENDED,

    OK = 200,

    BAD_REQUEST = 400,
    UNAUTHORIZED,
    PAYMENT_REQUIRED,
    FORBIDDEN,
    NOT_FOUND
} http_status_t;

#endif