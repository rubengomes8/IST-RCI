#ifndef TCP_TCP_H
#define TCP_TCP_H

#define max(A,B) ((A)>=(B)?(A):(B))

#define LISTEN_QUEUE 5

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "utils.h"

int tcp_socket_connect(char *host, char *service);
int tcp_send(int nbytes, char *ptr, int fd);
int tcp_receive(int nbytes, char *ptr, int fd);
int tcp_receive2(int nbytes, char *ptr, int fd);

int tcp_bind(char *service, int tcp_sessions);
int tcp_accept(int fd_tcp_server);
int *fd_array_init(int tcp_sessions);
void fd_array_set(int *fd_array, fd_set *fdSet, int *maxfd, int tcp_sessions);
int new_connection(int fd, int *fd_array, int tcp_sessions);

#endif //TCP_TCP_H
