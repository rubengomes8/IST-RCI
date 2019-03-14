#ifndef RCI_ROOT_SERVER_H
#define RCI_ROOT_SERVER_H


#define MAX_TRIES 10 //máximo de tentativas de comunicação falhadas antes de o programa terminar

#define REMOVE_LEN 72
#define DUMP_LEN 6
#define STREAMS_LEN 10000 //valor arbitrário para o comprimento da mensagem STREAMS, recebida pelo servidor de raízes
#define WIR_LEN 103 //whoisroot length 9+1+64+1+20+1+6+1=103
#define RIS_LEN 100 //rootis length 6+1+64+1+20+1+6+1=100
#define URR_LEN 82 //urroot length 6+1+64+1=82
#define POPREQ_LEN 8 //comprimento da mensagem POPREQ
#define POPRESP_LEN 95 //comprimento da mensagem POPRESP
#define WELCOME_LEN 68 //comprimento máximo da mensagem WELCOME
#define REDIRECT_LEN 25 //comprimento máximo da mensagem REDIRECT
#define NEWPOP_LEN 	25//comprimento máximo da mensagem NEWPOP 2+1+15+1+5+1
#define POP_QUERY_MIN_LEN 9//comprimento mínimo da mensagem POPQUERY SEM indicar bestpops
#define TQ_LEN 25 //comprimento máximo de uma TREE QUERY
#define TR_MIN_LEN 26 //comprimento mínimo de uma TREE REPLY sem indicar endereços das ligações a jusante
#define TR_LEN_BY_OCCUPIED 22 //comprimento de cada ligação a jusante a enviar na TREE REPLY



#define TIMEOUT_SECS 10
#define TIMEOUT_USECS 0

#include "tcp.h"
#include "udp.h"
#include "queue.h"
#include <sys/select.h>

//Este .c e .h implementam a comunicação com o servidor de raízes

int dump(int fd_rs, struct addrinfo *res_rs);
char *who_is_root(int fd_rs, struct addrinfo *res_rs, char *streamID, char *rsaddr, char *rsport, char* ipaddr, char* uport);
int popreq(int fd_udp, struct addrinfo *res_udp, char *pop_addr, char* pop_tport);
void popresp(int fd_udp, char* streamID, char *ipaddr, char *tport);
void remove_stream(int fd_rs, struct addrinfo *res_rs, char *streamID);
char *receive_confirmation(int fd_tcp, char *msg);
int newpop(int fd_pop, char *ipaddr, char *tport);
int pop_query(int query_id, int bestpops, int fd);
void receive_pop_query(char *ptr, int *requested_pops, int *queryID);
int receive_pop_reply(char *ptr, char *ip, char *port, int *available_sessions);
int send_pop_reply(int query_id, int avails, char *ip, char *port, int fd);
int send_tree_query(char *ip, char *tport, int fd);
void receive_tree_query(char *ptr, char *ip, char *tport);
int send_tree_reply(char *ip, char *tport, int tcp_sessions, int tcp_occupied, queue *redirect_queue_head, queue *redirect_queue_tail, int fd);
void receive_tree_reply(char *ptr, char *ip, char *tport, int tcp_sessions);

#endif //RCI_ROOT_SERVER_H
