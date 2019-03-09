#include "queue.h"

struct _queue
{
    char *ip;
    char *port;
    int available_sessions;
    int index; //corresponde ao indíce do fd correspondente em fd_array
    struct _queue *next;
};

//Retorna a head de uma nova fila em caso de sucesso, ou NULL em caso de insucesso
queue *newElement(char *ip, char *port, int available_sessions, int index)
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

    head->ip = (char *)malloc(sizeof(char)*(strlen(ip)+1));
    if(head->ip == NULL)
    {
        fprintf(stderr, "Erro: newQueue: malloc: %s\n", strerror(errno));
        free(head);
        return NULL;
    }

    strcpy(head->ip, ip);

    head->port = (char *)malloc(sizeof(char)*(strlen(port)+1));
    if(head->port == NULL)
    {
        fprintf(stderr, "Erro: newQueue: malloc: %s\n", strerror(errno));
        free(head->ip);
        free(head);
        return NULL;
    }

    strcpy(head->port, port);
    head->available_sessions = available_sessions;
    head->index = index;

    return head;
}

//Retorna a nova tail
queue *insertTail(char *ip, char *port, int available_sessions, int index, queue *tail)
{
    queue *new_element = NULL;

    if(tail == NULL) return NULL;

    new_element = newElement(ip, port, available_sessions, index);
    if(new_element == NULL) return NULL;

    tail->next = new_element;

    return new_element;
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

int getAvailableSessions(queue *element)
{
    if(element == NULL) return -1;
    return element->available_sessions;
}

void decreaseAvailableSessions(queue *element)
{
    if(element == NULL) return;
    element->available_sessions--;
}

//Remove um elemento, retorna a nova head e altera a tail, se necessário. Se retornar NULL deixou de existir fila
queue *removeElement(queue *head, queue **tail, char *ip, char *port)
{
    queue *aux = head;
    queue *aux2;

    if(aux == NULL) return NULL;

    while(aux != NULL)
    {
        if(aux->ip == NULL || aux->port == NULL) continue;

        //Elemento a remover
        if(strcmp(ip, aux->ip) == 0 && strcmp(port, aux->port) == 0)
        {
            //Se a aux for igual à head e à tail, a fila só tem um elemento.
            //Esse elemento é libertado e retona-se NULL
            if(aux == head && aux == *tail)
            {
                free(aux);
                return NULL;
            }

            //Se aux for igual à head, o elemento a remover é o primeiro da fila
            if(aux == head)
            {
                head = aux->next;
                freeElement(aux);
                return head;
            }


            //Se aux for igual à tail, o elemento a remover é o último da fila
            if(aux == *tail)
            {
                *tail = aux2;
                free(aux);
                return head;
            }

        }

        //aux2 é sempre o elemento anterior a aux, exceto quando aux é a head
        aux2 = aux;
        aux = aux->next;
    }

    //Senão encontrou nenhum igual retorna a head
    return head;
}

queue *removeElementByIndex(queue *head, queue **tail, int index)
{
    queue *aux = head;
    queue *aux2;

    if(aux == NULL) return NULL;

    while(aux != NULL)
    {
        //Elemento a remover
        if(aux->index == index)
        {
            //Se a aux for igual à head e à tail, a fila só tem um elemento.
            //Esse elemento é libertado e retona-se NULL
            if(aux == head && aux == *tail)
            {
                free(aux);
                return NULL;
            }

            //Se aux for igual à head, o elemento a remover é o primeiro da fila
            if(aux == head)
            {
                head = aux->next;
                freeElement(aux);
                return head;
            }


            //Se aux for igual à tail, o elemento a remover é o último da fila
            if(aux == *tail)
            {
                *tail = aux2;
                free(aux);
                return head;
            }

        }

        //aux2 é sempre o elemento anterior a aux, exceto quando aux é a head
        aux2 = aux;
        aux = aux->next;
    }

    //Senão encontrou nenhum igual retorna a head
    return head;
}
