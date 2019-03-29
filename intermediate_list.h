#ifndef RCI_INTERMEDIATELIST_H
#define RCI_INTERMEDIATELIST_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

typedef struct _intermlist intermlist;

intermlist *newElementInterm(char *ip, char *port, int tcp_sessions);
intermlist *insertTailInterm(char *ip, char *port, int tcp_sessions, intermlist *tail);
void freeIntermList(intermlist *head);
void freeElementInterm(intermlist *element);
char *getIPInterm(intermlist *element);
char *getPORTInterm(intermlist *element);
int getTcpSessionsInterm(intermlist *element);
intermlist *getNextInterm(intermlist *element);


#endif //RCI_INTERMEDIATELIST_H


 //////////////////////////////////////////
    /*struct _printlist *head = NULL;
    struct _printlist *aux = NULL;
    struct _printlist *tail = NULL;

    struct _intermlist **interm_list = NULL;

    interm_list = (intermlist **)malloc(sizeof(intermlist*)*tcp_sessions);
    if(interm_list == NULL)
    {
        if(flag_d) fprintf(stdout, "Erro: malloc: %s\n\n", strerror(errno));
        free(interm_list);
        return;
    }

    for(i = 0; i<tcp_sessions; i++)
    {
        interm_list[i] = NULL;
        interm_list[i] = (interm_list *)malloc(sizeof(char)*sizeof(intermlist));
        if(interm_list[i] == NULL)
        {
            for(j = 0; j<i; j++)
            {
                free(interm_list[i]);
            }
            free(interm_list);
            if(flag_d) fprintf(stdout, "Erro: malloc: %s\n\n", strerror(errno));
            return;
        }
        //aux_buffer_sons[i] = '\0';    
    }
    */