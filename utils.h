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



int count_specific_char(char *string, char ch);
void arguments_reading(int argc, char *argv[], int has_stream, char ipaddr[], char tport[], char uport[], char rsaddr[], char rsport[], int *tcp_sessions, int *bestpops, int *tsecs, int *flag_h);
int validate_stream(int argc, char *argv, char* streamID, char* streamNAME, char *streamIP, char* streamPORT);

#endif //UTILS_UTILS_H
