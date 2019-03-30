#include "list_to_print.h"


struct _printlist
{
    char *line;
    struct _printlist *next;
};

extern int flag_d;
extern int flag_b;

//Retorna a head de uma nova fila em caso de sucesso, ou NULL em caso de insucesso
printlist *newElementPrint(char *line)
{
    printlist *head = NULL;

    head = (printlist *)malloc(sizeof(printlist));
    if(head == NULL)
    {
        fprintf(stderr, "Erro: newPrintList: malloc: %s\n", strerror(errno));
        return NULL;
    }

    head->line = NULL;
    head->next = NULL;

    head->line = (char *)malloc(sizeof(char)*(strlen(line)+1));
    if(head->line == NULL)
    {
        fprintf(stderr, "Erro: newPrintList: malloc: %s\n", strerror(errno));
        free(head);
        return NULL;
    }

    strcpy(head->line, line);
    return head;
}

//Retorna a nova tail
printlist *insertTailPrint(char *line, printlist *tail)
{
    printlist *new_element = NULL;

    if(tail == NULL) return NULL;

    new_element = newElementPrint(line);
    if(new_element == NULL) return NULL;

    tail->next = new_element;

    return new_element;
}


void freePrintList(printlist *head)
{
    printlist *aux;

    aux = head;

    while(aux != NULL)
    {
        head = aux->next;
        freeElementPrint(aux);
        aux = head;
    }
}

void freeElementPrint(printlist *element)
{
    if(element == NULL) return;
    if(element->line != NULL) free(element->line);
    free(element);
}



char *getLinePrint(printlist *element)
{
    if(element == NULL) return NULL;
    return element->line;
}

printlist *getNextPrint(printlist *element)
{
    if(element == NULL) return NULL;
    return element->next;
}



////////////////////////////////////////

char *construct_line(intermlist *intermlist)
{
	int tcp_sessions;
	struct _intermlist *aux = intermlist;
	char *msg = NULL;
	tcp_sessions = getTcpSessionsInterm(aux);
	char *tcp_sessions_str = NULL;


	tcp_sessions_str = (char*)malloc(sizeof(char)*(tcp_sessions/10 + 2));
	if(tcp_sessions_str == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: construct_line: malloc: %s\n", strerror(errno));
        return NULL;
    }

	msg = (char*)malloc(sizeof(char)*(25 + tcp_sessions/10 + 1 + 22*tcp_sessions + 2)); //15+1+5+1+5+1+21*tcp_sessions = 28+21*tcp_sessions
    if(msg == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: construct_line: malloc: %s\n", strerror(errno));
        return NULL;
    }

    msg[0] = '\0';
    strcpy(msg, getIPInterm(aux));
    strcat(msg, ":");
    strcat(msg, getPORTInterm(aux));
    strcat(msg, " (");
    sprintf(tcp_sessions_str, "%d", tcp_sessions);
    strcat(msg, tcp_sessions_str);

    aux=getNextInterm(aux);
    if(aux != NULL)
    {
    	while(aux != NULL)
	    {
	    	strcat(msg, " ");
	    	strcat(msg, getIPInterm(aux));
		    strcat(msg, ":");
		    strcat(msg, getPORTInterm(aux));
	    	aux=getNextInterm(aux);
	    }

    }
    else
    {
    	strcat(msg, ")\n");
    	return msg;
    }
    strcat(msg, ")\n");

    free(tcp_sessions_str);
	return msg;
}

void print_tree(printlist *head, char *streamID)
{
	printlist *aux = head;

	while(aux != NULL)
	{
		printf("%s", getLinePrint(aux));
		aux = getNextPrint(aux);
	}
}