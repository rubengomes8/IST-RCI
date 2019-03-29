#include "list_to_print.h"

struct _printlist
{
    char *line;
    struct _printlist *next;
};


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