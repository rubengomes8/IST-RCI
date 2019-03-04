#ifndef RCI_ROOT_SERVER_H
#define RCI_ROOT_SERVER_H

#define REMOVE_LEN 72
#define DUMP_LEN 6
#define STREAMS_LEN 10000 //valor arbitrário para o comprimento da mensagem STREAMS, recebida pelo servidor de raízes
#define WIR_LEN 103 //whoisroot length 9+1+64+1+20+1+6+1=103
#define RIS_LEN 100 //rootis length 6+1+64+1+20+1+6+1=100
#define URR_LEN 82 //urroot length 6+1+64+1=82
#define POPREQ_LEN 8 //comprimento da mensagem POPREQ
#define POPRESP_LEN 95 //comprimento da mensagem POPRESP

#include "udp.h"

//Este .c e .h implementam a comunicação com o servidor de raízes

void dump(int fd_rs, struct addrinfo *res_rs);
char *who_is_root(int fd_rs, struct addrinfo *res_rs, char *streamID, char *rsaddr, char *rsport, char* ipaddr, char* uport);
void popreq(int fd_udp, struct addrinfo *res_udp, char *pop_addr, char* pop_tport);
void popresp(int fd_udp, char* streamID);
void remove_stream(int fd_rs, struct addrinfo *res_rs, char *streamID);

#endif //RCI_ROOT_SERVER_H
