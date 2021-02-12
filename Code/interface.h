#ifndef RCI_INTERFACE_H
#define RCI_INTERFACE_H

#include "root_server.h"
#include "tcp.h"
#include "queue.h"
#include "iamroot.h"
#include <time.h>
#include "query_list.h"

#define MAX_AUX_POPS 10
#define POP_QUERY_TIMEOUT 100
#define SONS_BUFFER 64
#define SF_TIMEOUT 30
#define DADS_BUFFER 64

void interface_root(int fd_ss, int fd_rs, struct addrinfo *res_rs, char* streamID, int is_root, char * ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied, int fd_udp, int fd_tcp_server, int *fd_array, int bestpops, queue *redirect_queue_head,
        queue *redirect_queue_tail, queue *redirect_queue_aux, int empty_redirect_queue, int tsecs, char *rsaddr,
        char *rsport, char **aux_buffer_sons, char **aux_ptr_sons, int *nread_sons, int is_flowing, int query_id,
        char *streamIP, char *streamPORT);
void interface_not_root(int fd_rs, struct addrinfo *res_rs, char* streamID, char *streamIP, char *streamPORT,
        int *is_root, char* ipaddr, char *uport, char *tport, int tcp_sessions, int tcp_occupied, int fd_tcp_server,
        int *fd_array, int bestpops, int fd_pop, char *pop_addr, char *pop_tport, char *rsaddr, char *rsport, int tsecs);
int read_terminal(int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char *ipaddr, char* uport, char* tport,
                   int tcp_sessions, int tcp_occupied, queue *redirect_queue_head, char *pop_addr, char *pop_tport,
                   int is_flowing);
queue *receive_newpop(queue *redirect_queue_head, queue **redirect_queue_tail, int i, int *fd_array, int *empty_queue, char *msg);
queue *pop_query_peers(int tcp_sessions, int *fd_array, int query_id, int bestpops, queue *redirect_queue_head, queue **redirect_queue_tail,
        int *tcp_occupied, int *empty_redirect_queue);
//queue *get_data_pop_reply(queue *pops_queue_head, queue **pops_queue_tail, char *ptr, int *empty_pops_queue, int query_id,
//        int *received_pops, int waiting_pop_reply, int *correct_info, int* insert_tail);
int readesao(struct addrinfo *res_rs, int fd_rs, char *streamID, char *rsaddr, char *rsport, char *ipaddr, char *uport,
             queue **redirect_queue_head, queue **redirect_queue_tail, int *fd_array, int *tcp_occupied, int tcp_sessions,
             int *empty_redirect_queue, int *is_root, char *pop_addr, char *pop_tport, int *fd_pop, char *streamIP,
             char *streamPORT, char *tport, int fd_tcp_server, int bestpops, queue *redirect_aux, int tsecs,
             char **aux_buffer_sons, char **aux_ptr_sons, int *nread_sons, int *is_flowing, int send_broken, int query_id);
int receive_data_root(char *data, int fd_ss, int tcp_sessions, queue **redirect_queue_head, queue **redirect_queue_tail,
                      int *empty_redirect_queue, int *is_flowing, int *fd_array, int *tcp_occupied, int* missing_tqs, int*missing);
queue *send_data_root(char *data, int data_len, int tcp_sessions, int *fd_array, int *tcp_occupied, queue *redirect_queue_head,
                      queue **redirect_queue_tail, int *empty_redirect_queue, int* missing_tqs, int*missing);
queue *send_is_flowing(int is_flowing, int *fd_array, int index, int *tcp_occupied, queue *redirect_queue_head,
                       queue *element, queue **redirect_queue_tail, queue *previous, int *empty_redirect_queue,
                       int remove_by_index, int* missing_tqs, int*missing);
queue *send_broken_stream_to_all(int *fd_array, int *tcp_occupied, queue *redirect_queue_head, queue **redirect_queue_tail,
                                 int *empty_redirect_queue);
queue *send_stream_flowing_to_all(int *fd_array, int *tcp_occupied, queue *redirect_queue_head, queue **redirect_queue_tail,
                                  int *empty_redirect_queue);
queue* lost_son(queue *aux, int *fd_array, int i, int *tcp_occupied, queue *redirect_queue_head, queue **redirect_queue_tail,
                int *empty_redirect_queue, queue *previous, int remove_by_index);
int buffer_interm_sons(char ***aux_ptr_sons, char ***aux_buffer_sons, int **nread_sons, int tcp_sessions);




#endif //RCI_INTERFACE_H
