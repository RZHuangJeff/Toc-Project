#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdbool.h>

typedef void(*conn_cb_func_t)(const long);

bool server_init(char *port, conn_cb_func_t cbfunc, FILE *log);
void server_run();

#endif