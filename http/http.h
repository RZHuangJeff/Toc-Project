#ifndef HTTP_H
#define HTTP_H

#include <stdbool.h>

#include "httpdef.h"
#include "../json/json.h"

#define REQ_TYPE_CNT 9

typedef struct {
    http_method_t method;
    char *uri;
    char *version;
} http_req_title_t;

typedef struct {
    http_status_t status;
    char *version;
} http_res_title_t;

typedef json_ele_t http_header_t;

typedef struct {
    http_req_title_t *title;
    http_header_t *header;
    char *body;
} http_request_t;

typedef struct {
    http_res_title_t *title;
    http_header_t *header;
    char *body;
} http_response_t;

http_request_t *http_new_request(char *title);
http_response_t *http_new_response(char *version);

void http_free_request(http_request_t *request);
void http_free_resopnse(http_response_t *response);

bool http_header_set_from_line(http_header_t *header, char *line);
bool http_header_set_from_pair(http_header_t *header, const char *key, const char *value);
char *http_header_get(http_header_t *header, char *key);

bool http_res_title_set_status(http_res_title_t *title, http_status_t status);
const char *http_status_to_str(http_status_t status);

#endif