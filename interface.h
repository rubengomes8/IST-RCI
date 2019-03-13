#ifndef RCI_INTERFACE_H
#define RCI_INTERFACE_H

#include "root_server.h"
#include "tcp.h"
#include "queue.h"

void interface_root(int fd_rs, struct addrinfo *res_rs, char* streamID, int is_root, char * ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied, int fd_udp, int fd_tcp_server, int *fd_array, int bestpops);
void interface_not_root(int fd_rs, struct addrinfo *res_rs, char* streamID, int is_root, char* ipaddr, char *uport,
                        char *tport, int tcp_sessions, int tcp_occupied, int fd_udp, int fd_tcp_server, int *fd_array,
                        int bestpops, int fd_pop);
int read_terminal(int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char *ipaddr, char* uport, char* tport,
                   int tcp_sessions, int tcp_occupied);
int welcome(int tcp_sessions, int *tcp_occupied, int fd_tcp_server, int *fd_array, char *streamID);
void redirect(int fd_tcp_server, char *ip, char *port);
queue *receive_newpop(queue *redirect_queue_head, queue **redirect_queue_tail, int i, int *fd_array, int *empty_queue);
queue *pop_query_peers(int tcp_sessions, int *fd_array, int query_id, int bestpops, queue *redirect_queue_head, queue **redirect_queue_tail);
queue *get_data_pop_reply(queue *pops_queue_head, queue **pops_queue_tail, char *ptr, int *empty_pops_queue);


#endif //RCI_INTERFACE_H
