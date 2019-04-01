#include "query_list.h"

struct _querylist
{
    int id;
    int bestpops;
    struct _querylist *next;
};



extern int flag_d;

query_list * newElementQuery(int id, int bp)
{
	query_list *head = NULL;
	head = (query_list *)malloc(sizeof(query_list));
	if(head == NULL)
    {
        fprintf(stderr, "Erro: newQueryList: malloc: %s\n", strerror(errno));
        return NULL;
    }
    head->id = id;
    head->bestpops = bp;
    head->next = NULL;
    return head;
}

void insertTailQuery(int id, int bp, query_list **tail)
{
    query_list *new_element = NULL;

    if(*tail == NULL) return;

    new_element = newElementQuery(id, bp);
    if(new_element == NULL) return;

    (*tail)->next = new_element;

    *tail = new_element;
}

void freeQueryList(query_list *head)
{
    query_list *aux;

    aux = head;

    while(aux != NULL)
    {
        head = aux->next;
        freeElementQuery(aux);
        aux = head;
    }
}

void freeElementQuery(query_list *element)
{
    if(element == NULL) return;
    free(element);
}

int getIdQuery(query_list *element)
{
    if(element == NULL) return -1;
    return element->id;
}

int getBestPopsQuery(query_list *element)
{
    if(element == NULL) return -1;
    return element->bestpops;
}

query_list *getNextQuery(query_list *element)
{
    if(element == NULL) return NULL;
    return element->next;
}

void decrementBestPops(query_list *element)
{
    if(element == NULL) return;
    (element->bestpops)--;
}
 
query_list *getElementById(query_list *head, query_list **previous, int id)
{
	query_list *aux = head;
	query_list *prev = NULL;
	while(aux != NULL)
	{
		if(getIdQuery(aux) == id) 
		{
			*previous = prev;
			return aux;
		}
		prev = aux;
		aux = getNextQuery(aux);
	}

	return NULL;
}

void removeElementQuery(query_list *previous, query_list *toRemove, query_list **head, query_list **tail)
{
	
	if(toRemove == NULL)
	{
		if(flag_d) printf("removeElementQuery: Elemento a remover é igual a NULL\n\n");
		return;
	}
	
	if(toRemove == *head && toRemove == *tail) 
	{
		freeElementQuery(toRemove);
		*head = NULL;
		*tail = NULL;
	}
	else if(toRemove == *head)
	{
		*head = getNextQuery(toRemove);
		freeElementQuery(toRemove);
		
	}
	else if(toRemove == *tail)
	{
		freeElementQuery(toRemove);
		previous->next = NULL;
		*tail = previous;
	}
	else
	{
		previous->next = toRemove->next;
		freeElementQuery(toRemove);
	}
}





