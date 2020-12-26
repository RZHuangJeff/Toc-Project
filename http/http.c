#include "http.h"

#include <string.h>
#include <stdlib.h>

static const char *str_methods[] = {
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "CONNECT",
    "OPTIONS",
    "TRACE",
    "PATCH",
    NULL
};

static void request_fill_title(http_req_title_t *title, char *line);

http_request_t *http_new_request(char *title){
    http_request_t *request = (http_request_t*)malloc(sizeof(http_request_t));
    request->title = (http_req_title_t*)malloc(sizeof(http_req_title_t));
    request->header = (http_header_t*)json_new_ele(JSON_OBJ);
    request->body = NULL;

    request_fill_title(request->title, title);

    return request;
}

http_response_t *http_new_response(char *version){
    http_response_t *response = (http_response_t*)malloc(sizeof(http_response_t));
    response->title = (http_res_title_t*)malloc(sizeof(http_res_title_t));
    response->title->status = OK;

    response->title->version = (char*)malloc(strlen(version) + 1);
    memcpy(response->title->version, version, strlen(version) + 1);

    response->header = json_new_ele(JSON_OBJ);
    response->body = NULL;

    return response;
}

void http_free_request(http_request_t *request){
    if(request->body)
        free(request->body);
    
    if(request->header)
        json_free_ele((json_ele_t*)request->header);

    if(request->title){
        if(request->title->uri)
            free(request->title->uri);
        
        if(request->title->version)
            free(request->title->version);

        free(request->title);
    }

    free(request);
}

void http_free_resopnse(http_response_t *response){
    if(response->body)
        free(response->body);

    if(response->header)
        json_free_ele((json_ele_t*)response->header);

    if(response->title){
        if(response->title->version)
            free(response->title->version);

        free(response->title);
    }

    free(response);
}

bool http_header_set_from_line(http_header_t *header, char *line){
    char *key = strtok_r(line, ": ", &line);
    char *vstr = strtok_r(line, " \r\n", &line);

    return http_header_set_from_pair(header, key, vstr);
}

bool http_header_set_from_pair(http_header_t *header, const char *key, const char *value){
    json_kvpair_t *h = J_TO_OBJ((json_ele_t*)header);
    size_t count = json_get_length((json_ele_t*)header);

    for(size_t i = 0; i < count; i++){
        char *str = J_TO_STR(h[i].key);
        if(!strcasecmp(str, key)){
            json_free_ele(h[i].value);
            h[i].value = json_new_ele(JSON_STR, value);
            return true;
        }
    }

    json_object_add((json_ele_t*)header, J_STR_TAG(key), json_new_ele(JSON_STR, value));

    return true;
}

char *http_header_get(http_header_t *header, char *key){
    json_kvpair_t *h = J_TO_OBJ((json_ele_t*)header);
    size_t count = json_get_length((json_ele_t*)header);

    for(size_t i = 0; i < count; i++){
        char *str = J_TO_STR(h[i].key);
        if(!strcasecmp(str, key))
            return J_TO_STR(h[i].value);
    }

    return NULL;
}

const char *http_status_to_str(http_status_t status){
    switch(status){
        case OK:
            return "OK";
        default:
            return "OK";
    }
}

static void request_fill_title(http_req_title_t *title, char *line){
    for(char *str = strtok_r(line, " ", &line), i = 0; str_methods[i]; i++)
        if(!strcasecmp(str, str_methods[i])){
            title->method = (http_method_t)i;
            break;
        }

    char *uri = strtok_r(line, " ", &line);
    size_t uri_len = strlen(uri) + 1;
    title->uri = (char*)malloc(uri_len);
    memcpy(title->uri, uri, uri_len);

    char *ver = strtok_r(line, "\r\n", &line);
    size_t ver_len = strlen(ver) + 1;
    title->version = (char*)malloc(ver_len);
    memcpy(title->version, ver, ver_len);
}