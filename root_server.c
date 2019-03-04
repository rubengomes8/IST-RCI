#include "root_server.h"

extern int flag_d;
extern int flag_b;

void dump(int fd_rs, struct addrinfo *res_rs)
{
    char *msg;
    int msg_len;
    struct sockaddr_in addr;
    unsigned int addrlen;
    int n;

    addrlen = sizeof(addr);

    msg = (char*) malloc(sizeof(char)*DUMP_LEN);
    if(msg == NULL && flag_d == 1)
    {
        fprintf(stderr, "Error: malloc: %s\n", strerror(errno));
        exit(-1);
    }

    sprintf(msg, "DUMP\n");
    msg_len = strlen(msg);

    if(flag_d)
    {
        printf("A comunicar com o servidor de raízes...\n");
        printf("Mensagem enviada: %s\n", msg);
    }

    udp_send(fd_rs, msg, msg_len, 0, res_rs);
    free(msg);

    msg = (char*) malloc(sizeof(char)*STREAMS_LEN);
    if(msg == NULL && flag_d == 1)
    {
        fprintf(stderr, "Error: malloc: %s\n", strerror(errno));
        exit(-1);
    }


    n = STREAMS_LEN; //Máximo comprimento da mensagem que pode receber
    n = udp_receive(fd_rs, &n, msg, 0, &addr, &addrlen);

    if(flag_d)
    {
        printf("Received by Root Server: %s\n", msg);
    }

    free(msg);
}

char *who_is_root(int fd_rs, struct addrinfo *res_rs, char *streamID, char *rsaddr, char *rsport, char* ipaddr, char* uport)
{
    int msg_len;
    struct sockaddr_in addr;
    unsigned int addrlen;
    int n;
    char *msg;
    char *msg2;

    addrlen = sizeof(addr);

    //solicitar ao root_server o endereço IP e porto UDP do root_access_server associado ao streamID
    msg = (char*) malloc(sizeof(char)*WIR_LEN);
    if(msg == NULL && flag_d == 1)
    {
        fprintf(stderr, "Error: malloc: %s\n", strerror(errno));
        exit(-1);
    }

    //Composição da mensagem a ser enviada
    sprintf(msg, "WHOISROOT %s %s:%s\n", streamID, ipaddr, uport);
    msg_len = strlen(msg);

    if(flag_d)
    {
        printf("A comunicar com o servidor de raízes...\n");
        printf("Mensagem enviada: %s\n", msg);
    }

    udp_send(fd_rs, msg, msg_len, 0, res_rs);
    free(msg);

    msg2 = (char*) malloc(sizeof(char)*RIS_LEN);
    if(msg2 == NULL && flag_d == 1)
    {
        fprintf(stderr, "Error: malloc: %s\n", strerror(errno));
        exit(-1);
    }


    n = RIS_LEN; //Máximo comprimento da mensagem que pode receber

    //Recebe como resposta ROOTIS - 100 ou URROOT - 82
    n = udp_receive(fd_rs, &n, msg2, 0, &addr, &addrlen);

    if(flag_d)
    {
        printf("Mensagem recebida pelo servidor de raízes: %s\n", msg2);
    }

    return msg2;
}

