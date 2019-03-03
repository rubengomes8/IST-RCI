#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int count_specific_char(char *string, char ch);
void arguments_reading(int argc, char *argv[], int has_stream, char ipaddr[], char tport[], char uport[], char rsaddr[], char rsport[], int *tcp_sessions, int *bestpops, int *tsecs, int *flag_b, int *flag_d, int *flag_h);


#endif //UTILS_UTILS_H
