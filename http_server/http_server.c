#include "http_server.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../server/server.h"

#define MAX_LINE 256
#define GET_LINE(fd, buf, msize) {                  \
        size_t ind = 0;                             \
        do                                          \
            read(fd, buf + ind, 1);                 \
        while(buf[ind++] != '\n' && ind < msize);   \
        buf[ind] = '\0';                            \
    }

typedef struct route_t {
    char *route;
    route_func_t rfunc;

    struct route_t *next;
} route_t;

typedef struct {
    http_response_t http_response;
    int fd;
} response_t;

static route_t *routes[REQ_TYPE_CNT];
static route_func_t def_routes[REQ_TYPE_CNT];

static void conn_reciv(const long fd);
static http_request_t *init_http_request(const int fd);
static http_response_t *init_http_response(char *version);

bool http_server_init(int port, FILE *log){
    char pbuff[16];
    sprintf(pbuff, "%d", port);
    if(!server_init(pbuff, conn_reciv, log))
        return false;

    for(int i = 0; i < REQ_TYPE_CNT; i++){
        routes[i] = NULL;
        def_routes[i] = NULL;
    }

    return true;
}

void http_server_add_route(http_method_t method, const char *route, const route_func_t rfunc){
    if(route == DEFAULT_ROUTE){
        def_routes[method] = rfunc;
        return;
    }

    route_t *nroute = (route_t*)malloc(sizeof(route_t));

    size_t rlen = strlen(route) + 1;
    nroute->route = (char*)malloc(rlen);
    memcpy(nroute->route, route, rlen);

    nroute->rfunc = rfunc;

    nroute->next = routes[method];
    routes[method] = nroute;
}

void http_server_run(){
    server_run();
}

void http_server_send_response(http_response_t *response){
    if(response->title->status == SENDED)
        return;

    response_t *res = (response_t*)response;
    char buff[MAX_LINE];
    sprintf(buff, "%s %d %s\r\n", response->title->version, response->title->status, http_status_to_str(response->title->status));
    write(res->fd, buff, strlen(buff));

    json_ele_t *headers = (json_ele_t*)response->header;
    json_kvpair_t *pairs = J_TO_OBJ(headers);
    size_t size = headers->cont_len;
    for(size_t i = 0; i < size; i++){
        sprintf(buff, "%s: %s\r\n", J_TO_STR(pairs[i].key), J_TO_STR(pairs[i].value));
        write(res->fd, buff, strlen(buff));
    }
    write(res->fd, "\r\n", 2);

    if(response->body)
        write(res->fd, response->body, strlen(response->body));

    res->http_response.title->status = SENDED;
}

static void conn_reciv(const long fd){
    http_request_t *request = init_http_request(fd);
    http_response_t *res = init_http_response(request->title->version);
    response_t *response = (response_t*)realloc(res, sizeof(response_t));
    response->fd = fd;

    route_t *ptr, *list = routes[request->title->method];
    for(ptr = list; ptr; ptr = ptr->next){
        if(!strcmp(request->title->uri, ptr->route)){
            ptr->rfunc(request, (http_response_t*)response);
            break;
        }
    }

    if(!ptr && def_routes[request->title->method])
        def_routes[request->title->method](request, (http_response_t*)response);

    
    http_server_send_response((http_response_t*)response);
    http_free_request(request);
    http_free_resopnse((http_response_t*)response);
}

static http_request_t *init_http_request(const int fd){
    http_request_t *req;
    char line[MAX_LINE];

    GET_LINE(fd, line, MAX_LINE);
    req = http_new_request(line);

    while(1){
        GET_LINE(fd, line, MAX_LINE);
        if(!strncmp(line, "\r\n", 2))
            break;

        http_header_set_from_line(req->header, line);
    }

    char *clstr = http_header_get(req->header, "Content-Length");
    if(clstr){
        size_t clen = atol(clstr);
        req->body = (char*)malloc(clen + 1);
        read(fd, req->body, clen);
        req->body[clen] = '\0';
    }

    return req;
}

static http_response_t *init_http_response(char *version){
    http_response_t *res = http_new_response(version);
    return res;
}