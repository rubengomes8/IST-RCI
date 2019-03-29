#ifndef RCI_PRINTLIST_H
#define RCI_PRINTLIST_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

typedef struct _printlist printlist;

printlist *newElementPrint(char *line);
printlist *insertTailPrint(char *line, printlist *tail);
void freePrintList(printlist *head);
void freeElementPrint(printlist *element);
char *getLinePrint(printlist *element);
printlist *getNextPrint(printlist *element);


#endif //RCI_PRINTLIST_H