#include "queue.h"

struct _queue
{
    char *ip;
    char *port;
    struct _queue *next;
};

//Retorna a head de uma nova fila em caso de sucesso, ou NULL em caso de insucesso
queue *newQueue(char *ip, char *port)
{
    queue *head = NULL;

    head = (queue *)malloc(sizeof(queue));
    if(head == NULL)
    {
        fprintf(stderr, "Erro: newQueue: malloc: %s\n", strerror(errno));
        return NULL;
    }

    head->ip = NULL;
    head->port = NULL;
    head->next = NULL;

    head->ip = (char *)malloc(sizeof(char)*strlen(ip));
    if(head->ip == NULL)
    {
        fprintf(stderr, "Erro: newQueue: malloc: %s\n", strerror(errno));
        free(head);
        return NULL;
    }

    strcpy(head->ip, ip);

    head->port = (char *)malloc(sizeof(char)*strlen(port));
    if(head->port == NULL)
    {
        fprintf(stderr, "Erro: newQueue: malloc: %s\n", strerror(errno));
        free(head->ip);
        free(head);
        return NULL;
    }

    strcpy(head->port, port);

    return head;
}

void freeQueue(queue *head)
{
    queue *aux;

    aux = head;

    while(aux != NULL)
    {
        head = aux->next;
        freeElement(aux);
        aux = head;
    }
}

void freeElement(queue *element)
{
    if(element == NULL) return;
    if(element->ip != NULL) free(element->ip);
    if(element->port != NULL) free(element->port);
    free(element);
}

char *getIP(queue *element)
{
    if(element == NULL) return NULL;

    return element->ip;
}

char *getPORT(queue *element)
{
    if(element == NULL) return NULL;

    return element->port;
}

queue *getNext(queue *element)
{
    if(element == NULL) return NULL;
    return element->next;
}

