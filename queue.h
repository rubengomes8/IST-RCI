#ifndef RCI_QUEUE_H
#define RCI_QUEUE_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

typedef struct _queue queue;

queue *newElement(char *ip, char *port, int available_sessions);
queue *insertTail(char *ip, char *port, int available_sessions, queue *tail);
void freeQueue(queue *head);
void freeElement(queue *element);
char *getIP(queue *element);
char *getPORT(queue *element);
queue *getNext(queue *element);
int getAvailableSessions(queue *element);
void decreaseAvailableSessions(queue *element);
//retorna a nova head e atualiza a tail
queue *removeElement(queue *head, queue **tail, char *ip, char *port);

#endif //RCI_QUEUE_H
