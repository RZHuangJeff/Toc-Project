#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>

#define MAX_LINE 256
#define LOG(tag, str, ...) {                                                \
        time_t cur_time = time(NULL);                                       \
        char *t = ctime(&cur_time), buf[MAX_LINE];                          \
        strncpy(buf, t, strlen(t) - 1);                                     \
        buf[strlen(t) - 1] = '\0';                                          \
        fprintf(serv_log, "[" tag "] <%s> " str "\n", buf, ## __VA_ARGS__); \
    }

static char *serv_port;
static conn_cb_func_t serv_cbfunc;
static FILE *serv_log;


static int open_listenfd(char *port);
static void *thread(void *args);

bool server_init(char *port, conn_cb_func_t cbfunc, FILE *log){
    if(!port || !cbfunc){
        fprintf(stderr, "[SERVER] port or cbfunc should not be NULL\n");
        return false;
    }
    
    size_t plen = strlen(port) + 1;
    serv_port = (char*)malloc(plen);
    memcpy(serv_port, port, plen);

    serv_cbfunc = cbfunc;
    serv_log = log ? log : stdout;

    return true;
}

void server_run(){
    long listenfd, connfd;
    char hostname[MAX_LINE], port[MAX_LINE], rline[MAX_LINE];
    struct sockaddr_storage c_addr;
    socklen_t c_len;

    listenfd = open_listenfd(serv_port);
    if(listenfd == -1)
        return;

    LOG("LOG", "Listening on port %s", serv_port);
    while(1){
        c_len = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (struct sockaddr*)&c_addr, &c_len);
        getnameinfo((struct sockaddr*)&c_addr, c_len, hostname, MAX_LINE, port, MAX_LINE, 0);

        LOG("LOG", "Accept a connection from (%s, %s)", hostname, port);
        
        pthread_t pid;
        pthread_create(&pid, NULL, thread, (void*)connfd);
    }
}

static int open_listenfd(char *port){
    struct addrinfo hints, *listp, *p;
    int listenfd;
    long optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
    getaddrinfo(NULL, port, &hints, &listp);

    for(p = listp; p; p = p->ai_next){
        if((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (const void*)optval, sizeof(int));

        if(!bind(listenfd, p->ai_addr, p->ai_addrlen))
            break;

        close(listenfd);
    }

    freeaddrinfo(listp);

    if(!p){
        LOG("FAILED", "Failed to get listen fd");
        return -1;
    }

    if(listen(listenfd, 10) < 0){
        LOG("FAILED", "Failed to start listen");
        close(listenfd);
        return -1;
    }

    return listenfd;
}

static void *thread(void *args){
    pthread_detach(pthread_self());

    long connfd = (long)args;
    serv_cbfunc(connfd);

    close(connfd);

    pthread_exit(0);
}