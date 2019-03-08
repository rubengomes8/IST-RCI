#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H

#define BUFFER_SIZE 100

// Definição de tamanho dos vetores que iram alojar o nome da stream, o ip e a porta correspondente
#define STREAM_ID_SIZE 64
#define IP_LEN 15
#define PORT_LEN 5
#define STREAM_NAME_LEN 42
//16+5+40+2 = 63

//Tempo mínimo aceitado para o período associado ao registo periódico da raiz no servidor de raízes
#define MIN_TSECS 1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "tcp.h"
#include "udp.h"
#include "root_server.h"


void sinopse();
int count_specific_char(char *string, char ch);
int arguments_reading(int argc, char *argv[], char ipaddr[], char tport[], char uport[], char rsaddr[],
        char rsport[], int *tcp_sessions, int *bestpops, int *tsecs, int *flag_h, char *streamID, char *streamNAME,
        char *streamIP, char *streamPORT);
int validate_stream(int argc, char *argv, char* streamID, char* streamNAME, char *streamIP, char* streamPORT);
void stream_id_to_lowercase(char *streamID);
void get_root_access_server(char *rasaddr, char *rasport, char *msg, struct addrinfo *res_rs, int fd_rs);
int get_redirect(char *pop_addr, char *pop_tport, char *msg);
void free_and_close(int is_root, int fd_rs, int fd_udp, int fd_pop, int fd_ss, struct addrinfo *res_rs, struct addrinfo *res_udp, struct addrinfo *res_pop, struct addrinfo *res_ss, int *fd_array);

//Funções de coisas que estavam no iamroot.c
int source_server_connect(int fd_rs, struct addrinfo *res_rs, struct addrinfo *res_ss, char *streamIP, char *streamPORT);
int install_tcp_server(char *tport, int fd_rs, struct addrinfo *res_rs, int fd_ss, struct addrinfo *res_ss, char *ipaddr);
int *create_fd_array(int tcp_sessions, int fd_rs, int fd_ss, struct addrinfo *res_rs, struct addrinfo *res_ss);
int install_access_server(int fd_rs, int fd_ss, struct addrinfo *res_rs, struct addrinfo *res_ss,
                          struct addrinfo **res_udp, char *uport, int *fd_array);
int get_access_point(char *rasaddr, char *rasport, struct addrinfo **res_udp, int fd_rs, struct addrinfo *res_rs,
                     char *pop_addr, char *pop_tport);


#endif //UTILS_UTILS_H
