#include "root_server.h"

extern int flag_d;
extern int flag_b;

//////////////////////////////////// Comunicação com o servidor de raízes //////////////////////////////////////////////
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
        printf("Mensagem enviada ao servidor de raízes: %s\n", msg);
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

    if(flag_d) printf("Mensagem recebida do servidor de raízes: %s\n", msg2);

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

    //Aloca timeout
    timeout = (struct timeval *)malloc(sizeof(struct timeval));
    if(timeout == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: who_is_root: malloc: %s\n", strerror(errno));
        return NULL;
    }

    timeout->tv_sec = TIMEOUT_SECS;
    timeout->tv_usec = TIMEOUT_USECS;

    addrlen = sizeof(addr);

    msg = (char*) malloc(sizeof(char)*WIR_LEN);
    if(msg == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: who_is_root: malloc: %s\n", strerror(errno));
        return NULL;
    }

    //Pergunta ao servidor de raizes quem é a raiz
    sprintf(msg, "WHOISROOT %s %s:%s\n", streamID, ipaddr, uport);
    msg_len = strlen(msg);

    if(flag_d) printf("A comunicar com o servidor de raízes...\n");
    udp_send(fd_rs, msg, msg_len, 0, res_rs);
    if(flag_d) printf("Mensagem enviada ao servidor de raízes: %s\n", msg);
    free(msg);

    msg2 = (char*) malloc(sizeof(char)*RIS_LEN);
    if(msg2 == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: who_is_root: malloc: %s\n", strerror(errno));
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
        if(flag_d) fprintf(stderr, "Erro ao receber a mensagem UDP do servidor de raízes!\n");
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
    char msg[POPREQ_LEN] = "";
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
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n", strerror(errno));
        return -1;
    }

    timeout->tv_sec = TIMEOUT_SECS;
    timeout->tv_usec = TIMEOUT_USECS;

    addrlen = sizeof(addr);

    sprintf(msg, "POPREQ\n");

    if(flag_d)
    {
        printf("A comunicar com o servidor de acessos...\n");
    }

    udp_send(fd_udp, msg, msg_len, 0, res_udp);

    if(flag_d)
    {
        printf("Mensagem enviada ao servidor de acessos: %s\n", msg);
    }


    FD_ZERO(&fdSet);
    FD_SET(fd_udp, &fdSet);
    maxfd = fd_udp;

    counter = select(maxfd + 1, &fdSet, (fd_set *)NULL, (fd_set *)NULL, timeout);

    if(counter <= 0)
    {
        if(flag_d) printf("Timed out: não foi recebida nenhuma resposta do servidor de acessos...\n");
        free(timeout);
        return -1;
    }

    //Recebe a resposta POPRESP
    msg_rcv_len = udp_receive(fd_udp, &msg_rcv_len, msg_rcv, 0, &addr, &addrlen);

    if(flag_d)
    {
        printf("Mensagem recebida do servidor de acessos: %s\n", msg_rcv);
    }

    //Extraccção de pop_addr e pop_tport da mensagem recebida msg_rcv
    token = strtok(msg_rcv, " ");
    token = strtok(NULL, " ");
    token = strtok(NULL, ":");
    if(token == NULL)
    {
        if(flag_d)
        {
            printf("Falha em obter o IP do ponto de acesso...\n");
        }
        free(timeout);
        return -1;
    }
    strcpy(pop_addr, token);

    token = strtok(NULL, "\n");
    if(token == NULL)
    {
        if(flag_d) printf("Falha em obter o porto TCP do ponto de acesso...\n");
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

/////////////////////////////////////// Adesão à árvore ////////////////////////////////////////////////////////////////
//Envia uma mensagem welcome e acrescenta um novo file descriptor ao array de file descriports, retornando o seu índice
int welcome(int tcp_sessions, int *tcp_occupied, int fd_tcp_server, int *fd_array, char *streamID)
{
    char buffer[BUFFER_SIZE];
    char *ptr;
    int i = -1;


    i = new_connection(fd_tcp_server, fd_array, tcp_sessions);
    if (i == -1)
    {
        if (flag_d) printf("Novo pedido de ligação recusado...\n");
    }
    else
    {
        if (flag_d)
        {
            printf("Pedido de ligação aceite...\n");
            printf("A enviar uma mensagem WELCOME ao peer...\n");
        }
        sprintf(buffer, "WE %s\n", streamID);
        ptr = buffer;
        tcp_send(strlen(ptr), ptr, fd_array[i]);
        if (flag_d)
        {
            printf("Mensagem enviada ao peer: %s\n", ptr);
        }
        (*tcp_occupied)++;
    }

    return i;
}

//Retorna NULL se não receber resposta
char *receive_confirmation(int fd_tcp, char *msg)
{
    struct timeval *timeout = NULL;
    int counter = 0;
    int maxfd;
    fd_set fdSet;
    int nread = 0;
    char *ptr = NULL;
    msg = NULL;

    msg = (char*)malloc(sizeof(char)*WELCOME_LEN + 1);
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


    FD_ZERO(&fdSet);
    FD_SET(fd_tcp, &fdSet);
    maxfd = fd_tcp;

    counter = select(maxfd + 1, &fdSet, (fd_set *)NULL, (fd_set *)NULL, timeout);

    if(counter <= 0)
    {
        if(flag_d) printf("Timed out: não foi recebida nenhuma mensagem do par...\n");
        free(timeout);
        free(msg);
        return NULL;
    }

    ptr = msg;
    nread = tcp_receive(WELCOME_LEN, ptr, fd_tcp);
    if(nread == -1)
    {
        if(flag_d) fprintf(stderr, "Erro ao receber a mensagem TCP vinda do par...\n");
        free(timeout);
        free(msg);
        return NULL;
    }

    msg[nread] = '\0';

    free(timeout);
    return msg;
}

//Redireciona um peer
void redirect(int fd_tcp_server, char *ip, char *port)
{
    int refuse_fd = -1;
    char buffer[BUFFER_SIZE];
    char* ptr;

    refuse_fd = tcp_accept(fd_tcp_server);
    if(refuse_fd == -1)
    {
        if(flag_d) printf("Falha ao aceitar o novo peer...\n");
    }
    else
    {
        if(flag_b)
        {
            printf("A aplicação já não tem capacidade para aceitar novas ligações a jusante...\n");
            printf("A redirecionar o peer...\n");
        }
        //Dar um endereço válido aqui
        sprintf(buffer, "RE %s:%s\n", ip, port);
        ptr = buffer;
        tcp_send(strlen(ptr), ptr, refuse_fd);
        if(flag_d)
        {
            printf("Mensagem enviada ao peer: %s\n", ptr);
        }
    }
}

//Recebe endereço de redirecionamento
int get_redirect(char *pop_addr, char *pop_tport, char *msg)
{
    char *token = NULL;


    token = strtok(msg, " ");

    token = strtok(NULL, ":");
    if(token == NULL)
    {
        if(flag_d) printf("Falha em obter o IP de redirecionamento...\n");
        return -1;
    }
    strcpy(pop_addr, token);

    token = strtok(NULL, "\n");
    if(token == NULL)
    {
        if(flag_d) printf("Falha em obter o porto TCP de redirecionamento...\n");
        return -1;
    }
    strcpy(pop_tport, token);

    return 0;
}

int newpop(int fd_pop, char *ipaddr, char *tport)
{
    int n;

    char buffer[NEWPOP_LEN];

    sprintf(buffer, "NP %s:%s\n", ipaddr, tport);
    n = tcp_send(strlen(buffer), buffer, fd_pop);
    if(n == -1)
    {
        if(flag_d) printf("Erro durante o envio da mensagem New Pop...\n");
        return -1;
    }

    if(flag_d) printf("Mensagem enviada a montante: %s\n", buffer);

    return 0;

}

////////////////////////////////////////// Comunicação entre pares /////////////////////////////////////////////////////


/////////////////////////////////////// Descoberta de pontos de acesso /////////////////////////////////////////////////
int pop_query(int query_id, int bestpops, int fd)
{
    int n;
    char *buffer = NULL;

    //comprimento do buffer é o comprimento de POP_QUERY sem indicar bestpops e
    //o nº de casas decimais de bestpops
    buffer =(char*)malloc(sizeof(char)*(POP_QUERY_MIN_LEN + bestpops/10 + 1 + 1));
    if(buffer == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: pop_query_malloc: %s\n", strerror(errno));
        return -1;
    }

    //Imprimir query_id em hexadecimal
    sprintf(buffer, "PQ %04X %d\n", query_id, bestpops);

    n = tcp_send(strlen(buffer), buffer, fd);
    if(n == -1)
    {
        if(flag_d) printf("Erro duranto o envio da mensagem POP_QUERY\n");
        free(buffer);
        return -1;
    }
    else if(n == 0)
    {
        if(flag_d) printf("Falha ao enviar a mensagem POP_QUERY: conexão terminada pelo peer\n");
        free(buffer);
        return 0;
    }

    free(buffer);
    return 1;
}

void receive_pop_query(char *ptr, int *requested_pops, int *queryID)
{
    char *token = NULL;

    token = strtok(ptr, " ");
    token = strtok(NULL, " ");

    *queryID = atoi(token);


    token = strtok(NULL, "\n");

    *requested_pops = atoi(token);
}

int receive_pop_reply(char *ptr, char *ip, char *port, int *available_sessions)
{
    int query_id;
    char *token = NULL;

    token = strtok(ptr, " "); //PR

    token = strtok(NULL, " ");//query ID
    query_id = atoi(token);

    token = strtok(NULL, ":");
    strcpy(ip, token);

    token = strtok(NULL, " ");
    strcpy(port, token);

    token = strtok(NULL, "\n");
    *available_sessions = atoi(token);

    return query_id;
}

int send_pop_reply(int query_id, int avails, char *ip, char *port, int fd)
{
    char *msg = NULL;
    int n;

    msg = (char *)malloc(sizeof(char)*(strlen(ip) + strlen(port) + 7 + 4 + (avails/10)+1 + 1));
    if(msg == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: send_pop_reply: malloc: %s\n", strerror(errno));
        exit(1);
    }

    sprintf(msg, "PR %04X %s:%s %d\n", query_id, ip, port, avails);


    n = tcp_send(strlen(msg), msg, fd);
    if(n == -1)
    {
        if(flag_d) printf("Erro duranto o envio da mensagem POP_REPLY\n");
        free(msg);
        return -1;
    }
    else if(n == 0)
    {
        if(flag_d) printf("Falha ao enviar a mensagem REPLY: conexão terminada pelo peer\n");
        free(msg);
        return 0;
    }

    if(flag_d) printf("Mensagem enviada ao par a montante: %s\n", msg);

    free(msg);
    return 1;
}

///////////////////////////////// Interrupção e estabelecimento da stream //////////////////////////////////////////////
int stream_flowing(int fd)
{
    char msg[4];
    char *ptr;
    int n;

    sprintf(msg, "SF\n");
    ptr = msg;

    n = tcp_send(3, ptr, fd);

    return n;
}

int broken_stream(int fd)
{
    char msg[4];
    char *ptr;
    int n;

    sprintf(msg, "BS\n");
    ptr = msg;

    n = tcp_send(3, ptr, fd);

    return n;
}


/////////////////////////////////// Monitorização da estrutura da árvore ///////////////////////////////////////////////
int send_tree_query(char *ip, char *tport, int fd)
{
    char *msg = NULL;
    int n;

    msg = (char *)malloc(sizeof(char)*TQ_LEN);
    if(msg == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: send_tree_query: malloc: %s\n", strerror(errno));
        exit(1);
    }

    sprintf(msg, "TQ %s:%s\n", ip, tport);

    n = tcp_send(strlen(msg), msg, fd);
    if(n == -1)
    {
        if(flag_d) printf("Erro duranto o envio da mensagem Tree Query\n");
        free(msg);
        return -1;
    }
    else if(n == 0)
    {
        if(flag_d) printf("Falha ao enviar a mensagem Tree Query: conexão terminada pelo peer\n");
        free(msg);
        return 0;
    }

    free(msg);
    return 1;
}

void receive_tree_query(char *ptr, char *ip, char *tport){

    char *token = NULL;

    token = strtok(ptr, " "); //TQ

    token = strtok(NULL, ":");//ipaddr
    strcpy(ip, token);

    token = strtok(NULL, "\n");
    strcpy(tport, token);

}



int send_tree_reply(char *ip, char *tport, int tcp_sessions, int tcp_occupied, queue *redirect_queue_head, queue *redirect_queue_tail, int fd)
{
    char *msg = NULL;
    struct _queue* redirect_aux = NULL;
    char ip_aux[IP_LEN + 1] = "\0";
    char port_aux[PORT_LEN + 1] = "\0";
    char ip_port[IP_LEN + PORT_LEN + 2] = "\0"; // 2 -> ':' e '\n'
    int n;

    msg = (char *)malloc(sizeof(char)*(TR_MIN_LEN + (tcp_sessions/10)+1 + tcp_occupied*TR_LEN_BY_OCCUPIED));
    if(msg == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: send_tree_query: malloc: %s\n", strerror(errno));
        exit(1);
    }

    sprintf(msg, "TR %s:%s %d\n", ip, tport, tcp_sessions);

    //Percorrer a lista de redirects tantas vezes quanto o número tcp_occupied e para cada uma delas acrescentar à TREE REPLY a seguinte string:
    //"<addr>:<port>\n"
    redirect_aux = redirect_queue_head;
    while(redirect_aux != NULL)
    {
        strcpy(ip_aux,getIP(redirect_aux));
        strcpy(port_aux, getPORT(redirect_aux));
        sprintf(ip_port, "%s:%s\n", ip_aux, port_aux);
        strcat(msg, ip_port);
        redirect_aux = getNext(redirect_aux);
    }

    //No final acrescentar um \n à string resultante
    strcat(msg, "\n");
    //Enviar a string
    n = tcp_send(strlen(msg), msg, fd);
    if(n == -1)
    {
        if(flag_d) printf("Erro duranto o envio da mensagem TREE_REPLY\n");
        free(msg);
        return -1;
    }
    else if(n == 0)
    {
        if(flag_d) printf("Falha ao enviar a mensagem TREE_REPLY: conexão terminada pelo peer\n");
        free(msg);
        return 0;
    }

    free(msg);
    return 1;
}


void receive_tree_reply(char *ptr, int fd)
{

}




