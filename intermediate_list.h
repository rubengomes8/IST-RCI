#ifndef RCI_INTERMEDIATELIST_H
#define RCI_INTERMEDIATELIST_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "queue.h"

#define IP_LEN 15
#define PORT_LEN 5

typedef struct _intermlist intermlist;

intermlist *newElementInterm(char *ip, char *port, int tcp_sessions);
void insertTailInterm(char *ip, char *port, int tcp_sessions, intermlist **tail);
void freeIntermList(intermlist *head);
void freeElementInterm(intermlist *element);
char *getIPInterm(intermlist *element);
char *getPORTInterm(intermlist *element);
int getTcpSessionsInterm(intermlist *element);
intermlist *getNextInterm(intermlist *element);


intermlist* construct_interm_list_header(struct _intermlist *interm_list, char *ptr);
intermlist* construct_interm_list_nodes(struct _intermlist *interm_list, char *ptr, int *fd_array, int tcp_sessions, int *tcp_occupied, queue **redirect_queue_head,
                                        queue **redirect_queue_tail, int *empty_redirect_queue, int *missing, intermlist **interm_tail, int index,
                                        queue *aux, int *missing_tqs);


#endif //RCI_INTERMEDIATELIST_H



