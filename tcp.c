#include "tcp.h"

extern int flag_d;
extern int flag_b;

//Recebe host:service e retorna file descriptor do socket
int tcp_socket_connect(char *host, char *service)
{
  struct addrinfo hints, *res = NULL;
  int fd, n;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; //IPv4
  hints.ai_socktype = SOCK_STREAM; //TCP socket
  hints.ai_flags = AI_NUMERICSERV;

  n = getaddrinfo(host, service, &hints, &res);
  if(n != 0)
  {
    if(flag_d) fprintf(stderr, "Erro: tcp_socket_connect: getaddrinfo: %s\n", gai_strerror(n));
    if(res != NULL) freeaddrinfo(res);
    return -1;
  }

  fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(fd == -1)
  {
    if(flag_d) fprintf(stderr, "Erro: tcp_socket_connect: socket: %s\n", strerror(errno));
    freeaddrinfo(res);
    return -1;
  }

  n = connect(fd, res->ai_addr, res->ai_addrlen);
  if(n == -1)
  {
    if(flag_d) fprintf(stderr, "Erro: tcp_socket_connect: connect: %s\n", strerror(errno));
    close(fd);
    freeaddrinfo(res);
    return -1;
  }

  freeaddrinfo(res);

  return fd;
}

//Envia mensagem tcp com máximo de nbytes, apontada pelo ponteiro ptr, para o file descriptor fd
int tcp_send(int nbytes, char *ptr, int fd)
{
  int nleft, nwritten;
  //Proteger contra sigpipe

  struct sigaction act;

  memset(&act, 0, sizeof(act));
  act.sa_handler = SIG_IGN;

  if(sigaction(SIGPIPE, &act, NULL) == -1)
  {
      fprintf(stderr, "Perdida a ligação ao par durante a escrita...\n");
      return 0;
  }

  nleft = nbytes;

  //Envio
  while (nleft > 0)
  {
    //Envia o número de bytes que faltam. Recebe o número de bytes enviado com sucesso
    nwritten = write(fd, ptr, nleft);
    if(nwritten == -1)
    {
        if(flag_d) fprintf(stderr, "Erro: tcp_send: write: %s\n", strerror(errno));
        return 0; //conexão terminada pelo peer
    }

    nleft -= nwritten;
    ptr += nwritten; //incremento do ponteiro até ao primeiro caracter não enviado
  }

  return nwritten; //success
}

//Recebe mensagem de fd, com máximo de nbytes e escreve-a em ptr
//ANTIGA
int tcp_receive(int nbytes, char *ptr, int fd)
{
    int nleft, nread;

    nleft = nbytes;
    int flag = 0;

    //Lê até encontrar um \n
    while (flag == 0 || *(ptr-1) != '\n')
    {
        flag = 1;
        // nread = read(fd, ptr, nleft);
        nread = read(fd, ptr, 1); //lê um char de cada vez
        if(nread == -1)
        {
            if(flag_d) fprintf(stderr, "Erro: tcp_receive: read: %s\n", strerror(errno));
            return 0;
        }
        else if(nread == 0)
        {
            return 0; //conexão terminada pelo peer
        }
        nleft -= nread;
        ptr += nread;
    }
    nread = nbytes - nleft;
    //Se calhar pôr \0 sempre aqui no fim da mensagem
    return nread;
}

//NOVA
int tcp_receive2(int nbytes, char *ptr, int fd)
{
    int nleft, nread;

    nleft = nbytes;
    int flag = 0;

    while(1)
    {
        nread = read(fd, ptr, 1);
        if(nread == -1)
        {
            if(flag_d) fprintf(stderr, "Erro: tcp_receive: read: %s\n", strerror(errno));
            return 0;
        }
        else if(nread == 0)
        {
            return 0; //conexão terminada pelo peer
        }
        nleft -= nread;
        ptr += nread;


        if(*(ptr-1) == '\n') break;
    }

    nread = nbytes-nleft;
    return nread;
}

 ///////////////////////////// Funções do servidor TCP /////////////////////////////////////////////////


int tcp_bind(char *service, int tcp_sessions)
{
    struct addrinfo hints, *res;
    int n;
    int fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP socket
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    n = getaddrinfo(NULL, service, &hints, &res);
    if(n!=0)
    {
        if(flag_d) fprintf(stderr, "Erro: tcp_bind: getaddrinfo: %s\n", gai_strerror(n));
        freeaddrinfo(res);
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(fd == -1)
    {
        if(flag_d) fprintf(stderr, "Erro: tcp_bind: socket: %s\n", strerror(errno));
        freeaddrinfo(res);
        return -1;
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1)
    {
        if(flag_d) fprintf(stderr, "Erro: tcp_bind: bind: %s\n", strerror(errno));
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    n = listen(fd, tcp_sessions);
    if(n == -1)
    {
        if(flag_d) fprintf(stderr, "Erro: tcp_bind: listen: %s\n", strerror(errno));
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    freeaddrinfo(res);

    return fd;
}

int *fd_array_init(int tcp_sessions)
{
    int *fd_array;
    int i;

    fd_array = (int *)malloc(sizeof(int)*tcp_sessions);
    if(fd_array == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: fd_array_init: malloc: %s\n", strerror(errno));
        return NULL;
    }

    for(i = 0; i<tcp_sessions; i++)
    {
        fd_array[i] = -1;
    }

    return fd_array;
}

void fd_array_set(int *fd_array, fd_set *fdSet, int *maxfd, int tcp_sessions)
{
    int i;

    //Faz set de todos os file descriptors diferentes de -1
    for(i=0; i<tcp_sessions; i++)
    {
        if(fd_array[i] != -1)
        {
            FD_SET(fd_array[i], fdSet);
            *maxfd = max(*maxfd, fd_array[i]);
        }
    }
}

int tcp_accept(int fd_server)
{
    struct sockaddr_in addr;
    unsigned int addrlen;
    int fd;

    addrlen= sizeof(addr);

    if((fd = accept(fd_server, (struct sockaddr*)&addr, &addrlen)) == -1)
    {
        if(flag_d) fprintf(stderr, "Erro: new_connection: accept: %s\n", strerror(errno));
        return -1;
    }

    return fd;
}

int new_connection(int fd, int *fd_array, int tcp_sessions)
{
    struct sockaddr_in addr;
    unsigned int addrlen;
    enum {accepted, refused} state;
    int newfd, i;

    //À falta de indicação em contrário, a ligação é recusada
    state = refused;

    addrlen = sizeof(addr);
    //Recebe um novo pedido de ligação
    if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1)
    {
        if(flag_d) fprintf(stderr, "Erro: new_connection: accept: %s\n", strerror(errno));
        return -1;
    }

    for(i=0; i<tcp_sessions; i++)
    {
        if(fd_array[i] == -1)
        {
            fd_array[i] = newfd;
            state = accepted;
            break;
        }
    }

    //Se a ligação for recusada, envia uma mensagem ao cliente a informar
    if(state == refused)
    {
        close(newfd);
        return -1;
    }

    return i;
}


