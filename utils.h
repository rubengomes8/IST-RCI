#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H

#define BUFFER_SIZE 64

// Definição de tamanho dos vetores que iram alojar o nome da stream, o ip e a porta correspondente
#define STREAM_ID_SIZE 64
#define IP_LEN 15
#define PORT_LEN 5
#define STREAM_NAME_LEN 42
//16+5+40+2 = 63

//Tempo mínimo aceitado para o período associado ao registo periódico da raiz no servidor de raízes
#define MIN_TSECS 1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>


void sinopse();
int count_specific_char(char *string, char ch);
int arguments_reading(int argc, char *argv[], char ipaddr[], char tport[], char uport[], char rsaddr[],
        char rsport[], int *tcp_sessions, int *bestpops, int *tsecs, int *flag_h, char *streamID, char *streamNAME,
        char *streamIP, char *streamPORT);
int validate_stream(int argc, char *argv, char* streamID, char* streamNAME, char *streamIP, char* streamPORT);
void stream_id_to_lowercase(char *streamID);
int get_root_access_server(char *rasaddr, char *rasport, char *msg);

#endif //UTILS_UTILS_H
