#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdio.h>
#include <stdbool.h>

#include "../http/http.h"

#define DEFAULT_ROUTE NULL

typedef void (*route_func_t)(http_request_t *req, http_response_t *res);

bool http_server_init(int port, FILE *log);

void http_server_add_route(http_method_t method, const char *route, const route_func_t rfunc);

void http_server_run();

void http_server_send_response(http_response_t *response);

#endif