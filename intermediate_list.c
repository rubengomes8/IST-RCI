#include "intermediate_list.h"

struct _intermlist
{
    char *ip;
    char *port;
    int tcp_sessions;
    struct _intermlist *next;
};

//Retorna a head de uma nova fila em caso de sucesso, ou NULL em caso de insucesso
intermlist *newElementInterm(char *ip, char *port, int tcp_sessions)
{
    intermlist *head = NULL;

    head = (intermlist *)malloc(sizeof(intermlist));
    if(head == NULL)
    {
        fprintf(stderr, "Erro: newIntermList: malloc: %s\n", strerror(errno));
        return NULL;
    }

    head->ip = NULL;
    head->port = NULL;
    head->next = NULL;

    head->ip = (char *)malloc(sizeof(char)*(strlen(ip)+1));
    if(head->ip == NULL)
    {
        fprintf(stderr, "Erro: newIntermList: malloc: %s\n", strerror(errno));
        free(head);
        return NULL;
    }

    strcpy(head->ip, ip);

    head->port = (char *)malloc(sizeof(char)*(strlen(port)+1));
    if(head->port == NULL)
    {
        fprintf(stderr, "Erro: newIntermList: malloc: %s\n", strerror(errno));
        free(head->ip);
        free(head);
        return NULL;
    }

    strcpy(head->port, port);

    head->tcp_sessions = tcp_sessions;

    return head;
}

//Retorna a nova tail
intermlist *insertTailInterm(char *ip, char *port, int tcp_sessions, intermlist *tail)
{
    intermlist *new_element = NULL;

    if(tail == NULL) return NULL;

    new_element = newElementInterm(ip, port, tcp_sessions);
    if(new_element == NULL) return NULL;

    tail->next = new_element;

    return new_element;
}


void freeIntermList(intermlist *head)
{
    intermlist *aux;

    aux = head;

    while(aux != NULL)
    {
        head = aux->next;
        freeElementInterm(aux);
        aux = head;
    }
}

void freeElementInterm(intermlist *element)
{
    if(element == NULL) return;
    if(element->ip != NULL) free(element->ip);
    if(element->port != NULL) free(element->port);
    free(element);
}



char *getIPInterm(intermlist *element)
{
    if(element == NULL) return NULL;

    return element->ip;
}

char *getPORTInterm(intermlist *element)
{
    if(element == NULL) return NULL;

    return element->port;
}

int getTcpSessionsInterm(intermlist *element)
{
    if(element == NULL) return -1;
    return element->tcp_sessions;
}

intermlist *getNextInterm(intermlist *element)
{
    if(element == NULL) return NULL;
    return element->next;
}