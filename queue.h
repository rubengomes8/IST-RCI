#ifndef RCI_QUEUE_H
#define RCI_QUEUE_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

typedef struct _queue queue;

queue *newElement(char *ip, char *port, int available_sessions, int index);
queue *insertTail(char *ip, char *port, int available_sessions, int index, queue *tail);
void freeQueue(queue *head);
void freeElement(queue *element);
char *getIP(queue *element);
char *getPORT(queue *element);
queue *getNext(queue *element);
int getIndex(queue *element);
int getAvailableSessions(queue *element);
void decreaseAvailableSessions(queue *element);
//retorna a nova head e atualiza a tail
queue *removeElementByAddress(queue *head, queue **tail, char *ip, char *port);
queue *removeElementByIndex(queue *head, queue **tail, int index);

#endif //RCI_QUEUE_H
