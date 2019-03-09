#ifndef RCI_QUEUE_H
#define RCI_QUEUE_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

typedef struct _queue queue;

queue *newQueue(char *ip, char *port);
void freeQueue(queue *head);
void freeElement(queue *element);
char *getIP(queue *element);
char *getPORT(queue *element);
queue *getNext(queue *element);

#endif //RCI_QUEUE_H
