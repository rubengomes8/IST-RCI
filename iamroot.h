#ifndef RCI_IAMROOT_H
#define RCI_IAMROOT_H

#include "udp.h"
#include "tcp.h"
#include "utils.h"
#include "root_server.h"
#include "interface.h"
#include <signal.h>

//Defines relativos aos valores default
#define RS_IP "193.136.138.142"
#define RS_PORT "59000"
#define IPADDR "127.0.0.1"
#define TPORT "58000"
#define UPORT "58000"
#define RSADDR "193.136.138.142"
#define RSPORT "59000"
#define TCP_SESSIONS 1
#define BESTPOPS 1
#define TSECS 5
#define FLAG_H 0
#define FLAG_B 1
#define FLAG_D 0
#define ASCII 1

//Funções para ligação à árvore
char *find_whoisroot(struct addrinfo *res_rs, int fd_rs, char *streamID, char *rsaddr, char *rsport, char *ipaddr, char *uport);
void get_root_access_server(char *rasaddr, char *rasport, char *msg, struct addrinfo *res_rs, int fd_rs);
int get_access_point(char *rasaddr, char *rasport, struct addrinfo **res_udp, int fd_rs, struct addrinfo *res_rs,
                     char *pop_addr, char *pop_tport);
int connect_to_peer(char *pop_addr, char *pop_tport, int fd_rs, int fd_udp, struct addrinfo *res_rs);
int wait_for_confirmation(char *pop_addr, char* pop_tport, int fd_rs, struct addrinfo *res_rs, int fd_udp, int fd_pop, char *streamID);


int source_server_connect(int fd_rs, struct addrinfo *res_rs, char *streamIP, char *streamPORT);
int install_tcp_server(char *tport, int fd_rs, struct addrinfo *res_rs, int fd_ss, char *ipaddr, int tcp_sessions);
int *create_fd_array(int tcp_sessions, int fd_rs, int fd_ss, struct addrinfo *res_rs);
int install_access_server(char *ipaddr, int fd_rs, int fd_ss, struct addrinfo *res_rs, struct addrinfo **res_udp, char *uport, int *fd_array);

void send_new_pop(int fd_pop, char *ipaddr, char *tport, int fd_rs, struct addrinfo *res_rs, int fd_tcp_server, int *fd_array);
void free_and_close(int is_root, int fd_rs, int fd_udp, int fd_pop, int fd_ss, struct addrinfo *res_rs, int *fd_array);





#endif //RCI_IAMROOT_H
