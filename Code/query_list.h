#ifndef RCI_QUERYLIST_H
#define RCI_QUERYLIST_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

typedef struct _querylist query_list;

query_list * newElementQuery(int id, int bp);
void insertTailQuery(int id, int bp, query_list **tail);
void freeQueryList(query_list *head);
void freeElementQuery(query_list *element);
int getIdQuery(query_list *element);
int getBestPopsQuery(query_list *element);
query_list *getNextQuery(query_list *element);
void decreaseBestPops(query_list *element, int received_pops);
query_list *getElementById(query_list *head, query_list **previous, int id);
void removeElementQuery(query_list *previous, query_list *toRemove, query_list **head, query_list **tail);

#endif //RCI_QUERYLIST_H