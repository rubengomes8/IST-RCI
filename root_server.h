#ifndef RCI_ROOT_SERVER_H
#define RCI_ROOT_SERVER_H

#define DUMP_LEN 5
#define STREAMS_LEN 10000 //valor arbitrário para o comprimento da mensagem STREAMS, recebida pelo servidor de raízes
#define WIR_LEN 103 //whoisroot length 9+1+64+1+20+1+6+1=103
#define RIS_LEN 100 //rootis length 6+1+64+1+20+1+6+1=100
#define URR_LEN 82 //urroot length 6+1+64+1=82

#include "udp.h"

//Este .c e .h implementam a comunicação com o servidor de raízes

void dump(int fd_rs, struct addrinfo *res_rs);
char *who_is_root(int fd_rs, struct addrinfo *res_rs, char *streamID, char *rsaddr, char *rsport);

#endif //RCI_ROOT_SERVER_H
