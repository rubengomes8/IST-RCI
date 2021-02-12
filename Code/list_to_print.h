#ifndef RCI_PRINTLIST_H
#define RCI_PRINTLIST_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "intermediate_list.h"
#include "queue.h"

typedef struct _printlist printlist;

printlist *newElementPrint(char *line);
printlist *insertTailPrint(char *line, printlist *tail);
void freePrintList(printlist *head);
void freeElementPrint(printlist *element);
char *getLinePrint(printlist *element);
printlist *getNextPrint(printlist *element);

char *construct_line(intermlist *intermlist);
void print_tree(printlist *head, char *streamID, queue *redirect_queue_head, char *ipaddr, char *tport, int tcp_sessions);


#endif //RCI_PRINTLIST_H