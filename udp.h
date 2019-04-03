#ifndef UDP_UDP_H
#define UDP_UDP_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>


int udp_socket(char *host, char *port, struct addrinfo **res);
int udp_send(int fd, char *msg, int msg_len, int flags, struct addrinfo *res);
int udp_receive(int fd, int *msg_len, char* buffer, int flags, struct sockaddr_in *addr, unsigned int *addrlen);
int udp_bind(int fd, struct addrinfo *res);
int udp_answer(int fd, char* msg, int msg_len, int flags, struct sockaddr *addr, int addrlen);

#endif //UDP_UDP_H
