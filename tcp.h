#ifndef TCP_TCP_H
#define TCP_TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

int tcp_socket_connect(char *host, char *service);
void tcp_send(int nbytes, char *ptr, int fd);
int tcp_receive(int nbytes, char *ptr, int fd);

#endif //TCP_TCP_H
