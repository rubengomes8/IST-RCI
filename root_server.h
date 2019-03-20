#ifndef RCI_ROOT_SERVER_H
#define RCI_ROOT_SERVER_H

#define MAX_TRIES 10 //máximo de tentativas de comunicação falhadas antes de o programa terminar

//Comunicação com o servidor de raízes
#define WIR_LEN 103 //whoisroot length 9+1+64+1+20+1+6+1=103
#define RIS_LEN 100 //rootis length 6+1+64+1+20+1+6+1=100
#define URR_LEN 82 //urroot length 6+1+64+1=82
#define DUMP_LEN 6
#define STREAMS_LEN 10000 //valor arbitrário para o comprimento da mensagem STREAMS, recebida pelo servidor de raízes
#define REMOVE_LEN 72

//Comunicação com o servidor de acessos
#define POPREQ_LEN 68 //comprimento da mensagem POPREQ
#define POPRESP_LEN 95 //comprimento da mensagem POPRESP

//Comunicação entre pares
//Adesão à árvores
#define WELCOME_LEN 68 //comprimento máximo da mensagem WELCOME
#define REDIRECT_LEN 25 //comprimento máximo da mensagem REDIRECT
#define NEWPOP_LEN 	25//comprimento máximo da mensagem NEWPOP 2+1+15+1+5+1

//Interrupção e estabelecimento da stream

//Envapsulamento dos dados da stream

//Descoberta de pontos de acesso
#define POP_QUERY_MIN_LEN 9//comprimento mínimo da mensagem POPQUERY SEM indicar bestpops

//Monitorização da estrutura da árvore de escoamento
#define TQ_LEN 25 //comprimento máximo de uma TREE QUERY
#define TR_MIN_LEN 26 //comprimento mínimo de uma TREE REPLY sem indicar endereços das ligações a jusante
#define TR_LEN_BY_OCCUPIED 22 //comprimento de cada ligação a jusante a enviar na TREE REPLY
#define TR_MAX_LEN 10000 //isto supostamente será para tirar - ver função receive_tree_reply_and_propagate

//Definição do timeout
#define TIMEOUT_SECS 10
#define TIMEOUT_USECS 0

#include "tcp.h"
#include "udp.h"
#include "queue.h"
#include <sys/select.h>

//////////////////////////////////// Comunicação com o servidor de raízes //////////////////////////////////////////////
int dump(int fd_rs, struct addrinfo *res_rs);
char *who_is_root(int fd_rs, struct addrinfo *res_rs, char *streamID, char *rsaddr, char *rsport, char* ipaddr, char* uport);
void remove_stream(int fd_rs, struct addrinfo *res_rs, char *streamID);

//////////////////////////////////// Comunicação com o servidor de acessos /////////////////////////////////////////////
int popreq(int fd_udp, struct addrinfo *res_udp, char *pop_addr, char* pop_tport);
void popresp(int fd_udp, char* streamID, char *ipaddr, char *tport);

////////////////////////////////////////// Comunicação entre pares /////////////////////////////////////////////////////
//Addesão à árvore
int welcome(int tcp_sessions, int *tcp_occupied, int fd_tcp_server, int *fd_array, char *streamID);
char *receive_confirmation(int fd_tcp, char *msg);
void redirect(int fd_tcp_server, char *ip, char *port);
int get_redirect(char *pop_addr, char *pop_tport, char *msg);
int newpop(int fd_pop, char *ipaddr, char *tport);

//Interrupção e estabelecimento da stream
int stream_flowing(int fd);
int broken_stream(int fd);

//Encapsulamento dos dados da stream
//DATA

//Descoberta de pontos de acesso
int pop_query(int query_id, int bestpops, int fd);
int receive_pop_query(char *ptr, int *requested_pops, int *queryID);
int receive_pop_reply(char *ptr, char *ip, char *port, int *available_sessions);
int send_pop_reply(int query_id, int avails, char *ip, char *port, int fd);

//Monitorização da estrutura da árvore de escoamento
int send_tree_query(char *ip, char *tport, int fd);
void receive_tree_query(char *ptr, char *ip, char *tport);
int send_tree_reply(char *ip, char *tport, int tcp_sessions, int tcp_occupied, queue *redirect_queue_head, queue *redirect_queue_tail, int fd);
int receive_tree_reply_and_propagate(char *ptr, int fd_pop, int fd_son);

#endif //RCI_ROOT_SERVER_H
