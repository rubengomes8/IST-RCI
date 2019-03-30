#include "intermediate_list.h"

struct _intermlist
{
    char *ip;
    char *port;
    int tcp_sessions;
    struct _intermlist *next;
};

extern int flag_d;
extern int flag_b;

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
void insertTailInterm(char *ip, char *port, int tcp_sessions, intermlist **tail)
{
    intermlist *new_element = NULL;

    if(*tail == NULL) return;

    new_element = newElementInterm(ip, port, tcp_sessions);
    if(new_element == NULL) return;

    (*tail)->next = new_element;
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


///////////////////////////////////////////////////////////////////
intermlist* construct_interm_list_header(struct _intermlist *interm_list, char *ptr)
{
	char *token = NULL;
	char ip[IP_LEN+1];
	char port[PORT_LEN+1];
	char tcp_sessions_str[4];
	int tcp_sessions;

	token = strtok(ptr, " ");

    token = strtok(NULL, ":");
    if(token == NULL)
    {
        if(flag_d) printf("Falha em obter o IP...\n");
        return NULL;
    }
    strcpy(ip, token);
    token = NULL;

    token = strtok(NULL, " ");
    if(token == NULL)
    {
        if(flag_d) printf("Falha em obter o porto...\n");
        return NULL;
    }
    strcpy(port, token);
    token = NULL;

    token = strtok(NULL, "\n");
    if(token == NULL)
    {
        if(flag_d) printf("Falha em obter o número de ligações tcp...\n");
        return NULL;
    }
    strcpy(tcp_sessions_str, token);
    tcp_sessions = atoi(tcp_sessions_str);

    interm_list = newElementInterm(ip, port, tcp_sessions);

    return interm_list;
}

intermlist* construct_interm_list_nodes(struct _intermlist *interm_list, char *ptr, int *fd_array, int tcp_sessions, int *tcp_occupied, queue **redirect_queue_head,
        queue **redirect_queue_tail, int *empty_redirect_queue, int *missing, intermlist **interm_tail, int index, queue *aux)
{
	char *token = NULL;
	char ip[IP_LEN+1];
	char port[PORT_LEN+1];
	char tree_query_str[TQ_LEN];

	token = strtok(ptr, ":");
    if(token == NULL)
    {
        if(flag_d) printf("Falha em obter o IP...\n");
        return NULL;
    }
    strcpy(ip, token);
    token = NULL;

    token = strtok(NULL, "\n");
    if(token == NULL)
    {
        if(flag_d) printf("Falha em obter o porto...\n");
        return NULL;
    }
    strcpy(port, token);
    token = NULL;



    *redirect_queue_head = send_tq_to_1_son(ip, port, fd_array, *redirect_queue_head, redirect_queue_tail, empty_redirect_queue,
            index, aux, tcp_occupied);
    //*redirect_queue_head = send_tree_query(ip, port, fd_array, tcp_sessions, tcp_occupied, *redirect_queue_head,
       //     redirect_queue_tail, empty_redirect_queue);
    (*missing)++;



    insertTailInterm(ip, port, -1, interm_tail);

    return interm_list;
}
