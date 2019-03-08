#include "root_server.h"

extern int flag_d;
extern int flag_b;

//Retorna -1 quando sai por timeout
int dump(int fd_rs, struct addrinfo *res_rs)
{
    char msg[DUMP_LEN];
    int msg_len;
    struct sockaddr_in addr;
    unsigned int addrlen;
    int n;
    char msg2[STREAMS_LEN];
    struct timeval *timeout = NULL;
    int counter = 0, maxfd;
    fd_set fdSet;

    timeout = (struct timeval *)malloc(sizeof(struct timeval));
    if(timeout == NULL)
    {
        if(flag_d) fprintf(stderr, "Error: malloc: %s\n", strerror(errno));
        exit(1);
    }

    timeout->tv_sec = TIMEOUT_SECS;
    timeout->tv_usec = TIMEOUT_USECS;


    addrlen = sizeof(addr);

    sprintf(msg, "DUMP\n");
    msg_len = strlen(msg);

    if(flag_d)
    {
        printf("A comunicar com o servidor de raízes...\n");
        printf("Mensagem enviada: %s\n", msg);
    }

    udp_send(fd_rs, msg, msg_len, 0, res_rs);


    //Espera pela resposta durante a duração de timeout
    FD_ZERO(&fdSet);
    FD_SET(fd_rs, &fdSet);
    maxfd = fd_rs;

    counter = select(maxfd + 1, &fdSet, (fd_set *)NULL, (fd_set *)NULL, timeout);
    if(counter <= 0)
    {
        if(flag_d) printf("Timed out: não foi recebida nenhuma resposta do servidor de raízes\n");
        free(timeout);
        return -1;
    }



    n = STREAMS_LEN; //Máximo comprimento da mensagem que pode receber
    n = udp_receive(fd_rs, &n, msg2, 0, &addr, &addrlen);

    printf("Received by Root Server: %s\n", msg2);

    free(timeout);
    return 0;
}

//Retorna NULL se não receber resposta ou retorna ROOTIS(URROOT) caso a resposta tenha sido recebida
char *who_is_root(int fd_rs, struct addrinfo *res_rs, char *streamID, char *rsaddr, char *rsport, char* ipaddr, char* uport)
{
    int msg_len;
    struct sockaddr_in addr;
    unsigned int addrlen;
    int n;
    char *msg;
    char *msg2;
    struct timeval *timeout = NULL;
    int counter = 0;
    int maxfd;
    fd_set fdSet;

    timeout = (struct timeval *)malloc(sizeof(struct timeval));
    if(timeout == NULL)
    {
        if(flag_d) fprintf(stderr, "Error: who_is_root: malloc: %s\n", strerror(errno));
        return NULL;
    }

    timeout->tv_sec = TIMEOUT_SECS;
    timeout->tv_usec = TIMEOUT_USECS;

    addrlen = sizeof(addr);

    //solicitar ao root_server o endereço IP e porto UDP do root_access_server associado ao streamID
    msg = (char*) malloc(sizeof(char)*WIR_LEN);
    if(msg == NULL)
    {
        if(flag_d) fprintf(stderr, "Error: who_is_root: malloc: %s\n", strerror(errno));
        return NULL;
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
    if(msg2 == NULL)
    {
        if(flag_d) fprintf(stderr, "Error: who_is_root: malloc: %s\n", strerror(errno));
        return NULL;
    }

    FD_ZERO(&fdSet);
    FD_SET(fd_rs, &fdSet);
    maxfd = fd_rs;

    counter = select(maxfd + 1, &fdSet, (fd_set *)NULL, (fd_set *)NULL, timeout);

    if(counter <= 0)
    {
        if(flag_d) printf("Timed out: não foi recebida nenhuma resposta do servidor de raízes\n");
        free(msg2);
        free(timeout);
        return NULL;
    }



    n = RIS_LEN; //Máximo comprimento da mensagem que pode receber

    //Recebe como resposta ROOTIS - 100 ou URROOT - 82
    n = udp_receive(fd_rs, &n, msg2, 0, &addr, &addrlen);
    if(n == -1)
    {
        if(flag_d) fprintf(stderr, "Erro ao receber a mensagem UDP...\n");
        free(timeout);
        return NULL;
    }


    if(flag_d)
    {
        printf("Mensagem recebida do servidor de raízes: %s\n", msg2);
    }

    free(timeout);
    return msg2;
}

void remove_stream(int fd_rs, struct addrinfo *res_rs, char *streamID)
{
    char msg[REMOVE_LEN] = "\0";
    int msg_len = REMOVE_LEN;

    sprintf(msg, "REMOVE %s\n", streamID);

    udp_send(fd_rs, msg, msg_len, 0, res_rs);
}


//////////////////////////////////////// Servidor de Acesso ////////////////////////////////////////////////////////////

int popreq(int fd_udp, struct addrinfo *res_udp, char *pop_addr, char *pop_tport)
{
    char msg[POPREQ_LEN];
    int msg_len = POPREQ_LEN;
    char msg_rcv[POPRESP_LEN];
    int msg_rcv_len = POPRESP_LEN;
    struct sockaddr_in addr;
    unsigned int addrlen;
    char *token = NULL;
    struct timeval *timeout;
    fd_set fdSet;
    int maxfd;
    int counter = 0;


    timeout = (struct timeval *)malloc(sizeof(struct timeval));
    if(timeout == NULL)
    {
        if(flag_d) fprintf(stderr, "Error: malloc: %s\n", strerror(errno));
        return -1;
    }

    timeout->tv_sec = TIMEOUT_SECS;
    timeout->tv_usec = TIMEOUT_USECS;

    addrlen = sizeof(addr);

    sprintf(msg, "POPREQ\n");

    if(flag_d)
    {
        printf("A comunicar com o servidor de acesso...\n");
    }

    udp_send(fd_udp, msg, msg_len, 0, res_udp);

    if(flag_d)
    {
        printf("Mensagem enviada: %s\n", msg);
    }


    FD_ZERO(&fdSet);
    FD_SET(fd_udp, &fdSet);
    maxfd = fd_udp;

    counter = select(maxfd + 1, &fdSet, (fd_set *)NULL, (fd_set *)NULL, timeout);

    if(counter <= 0)
    {
        if(flag_d) printf("Timed out: não foi recebida nenhuma resposta do servidor de acesso\n");
        free(timeout);
        return -1;
    }

    //Recebe a resposta POPRESP
    msg_rcv_len = udp_receive(fd_udp, &msg_rcv_len, msg_rcv, 0, &addr, &addrlen);

    if(flag_d)
    {
        printf("Mensagem recebida do servidor de acesso: %s\n", msg_rcv);
    }

    //Extraccção de pop_addr e pop_tport da mensagem recebida msg_rcv
    token = strtok(msg_rcv, " ");
    token = strtok(NULL, " ");
    token = strtok(NULL, ":");
    if(token == NULL)
    {
        if(flag_d) printf("ipaddr inválido!\n");
        free(timeout);
        return -1;
    }
    strcpy(pop_addr, token);

    token = strtok(NULL, "\n");
    if(token == NULL)
    {
        if(flag_d) printf("tport inválido!\n");
        free(timeout);
        return -1;
    }

    strcpy(pop_tport, token);
    free(timeout);
    return 0;
}

void popresp(int fd_udp, char *streamID, char *ipaddr, char *tport)
{
    char msg[POPREQ_LEN];
    int msg_len = POPREQ_LEN;
    char msg2[POPRESP_LEN];
    int msg_len2 = POPRESP_LEN;
    struct sockaddr_in addr;
    unsigned int addrlen;

    addrlen = sizeof(addr);

    //Recebe um POPREQ
    msg_len = udp_receive(fd_udp, &msg_len, msg, 0, &addr, &addrlen);

    if(flag_d)
    {
        printf("Mensagem recebida de um cliente iamroot: %s\n", msg);
    }

    printf("msg: %s\n", msg);
    if(strcmp(msg, "POPREQ\n") != 0)
    {
        if(flag_d) printf("Mensagem inválida!\n");
        return;
    }

    //Chamar função para procurar pontos de ligação

    sprintf(msg2, "POPRESP %s %s:%s\n", streamID, ipaddr, tport);

    if(flag_d)
    {
        printf("A comunicar com o novo cliente iamroot...\n");
    }

    msg_len2 = strlen(msg2);
    udp_answer(fd_udp, msg2, msg_len2, 0, (struct sockaddr *)&addr, addrlen);

    if(flag_d)
    {
        printf("Mensagem enviada ao novo cliente iamroot: %s\n", msg2);
    }
}

//////////////////////////////////// Comunicação entre pares ///////////////////////////////////////////////////////////
//Retorna NULL se não receber resposta
char *receive_confirmation(int fd_tcp, char *msg)
{
    int msg_len;
    int n;
    struct timeval *timeout = NULL;
    int counter = 0;
    int maxfd;
    fd_set fdSet;
    int nread = 0;
    char *ptr = NULL;
    msg = NULL;

    msg = (char*)malloc(sizeof(char)*WELCOME_LEN);
    if(msg == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: receive_confirmation: malloc: %s\n", strerror(errno));
        return NULL;
    }

    timeout = (struct timeval *)malloc(sizeof(struct timeval));
    if(timeout == NULL)
    {
        if(flag_d) fprintf(stderr, "Error: receive_confirmation: malloc: %s\n", strerror(errno));
        free(msg);
        return NULL;
    }

    timeout->tv_sec = TIMEOUT_SECS;
    timeout->tv_usec = TIMEOUT_USECS;


   /* FD_ZERO(&fdSet);
    FD_SET(fd_tcp, &fdSet);
    maxfd = fd_tcp;

    counter = select(maxfd + 1, &fdSet, (fd_set *)NULL, (fd_set *)NULL, timeout);

    if(counter <= 0)
    {
        if(flag_d) printf("Timed out: não foi recebida nenhuma mensagem do peer...\n");
        free(timeout);
        free(msg);
        return NULL;
    }*/

    ptr = msg;
    nread = tcp_receive(WELCOME_LEN, ptr, fd_tcp);
    if(nread == -1)
    {
        if(flag_d) fprintf(stderr, "Erro ao receber a mensagem TCP...\n");
        free(timeout);
        free(msg);
        return NULL;
    }
    //Dúvida do RG - Ele aqui pode receber mais do que o espaço que alocou?
    else if(nread > WELCOME_LEN)
    {
        if(flag_d) printf("Recebidos mais caracteres do que o possível do par...\n");
        free(timeout);
        free(msg);
        return NULL;
    }

    msg[nread] = '\0';
    if(flag_d)
    {
        printf("Mensagem recebida do peer: %s\n", msg);
    }

    free(timeout);
    return msg;
}

int newpop(int fd_pop, char *ipaddr, char *tport)
{
    int n;

    char buffer[NEWPOP_LEN];

    sprintf(buffer, "NP %s:%s\n", ipaddr, tport);
    n = tcp_send(NEWPOP_LEN, buffer, fd_pop);
    if(n == -1)
    {
        if(flag_d) printf("Erro durante o envio da mensagem NEWPOP.\n");
        return -1;
    }

    return 0; //mudar isto

}
