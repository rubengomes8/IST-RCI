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
void udp_send(int fd, char *msg, int msg_len, int flags, struct addrinfo *res);
//talvez mais tarde seja melhor pôr udp_receive a retornar o que recebe e ter uma função que apenas escreve
void udp_receive(int fd, int *msg_len, char* buffer, int flags, struct sockaddr_in *addr, unsigned int *addrlen);
void print_sender(struct sockaddr_in addr, unsigned int addrlen, int flags);
void udp_bind(int fd, struct addrinfo *res);

#endif //UDP_UDP_H
