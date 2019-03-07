#include "tcp.h"

extern int flag_d;
extern int flag_b;

//Recebe host:service e retorna file descriptor do socket
int tcp_socket_connect(char *host, char *service)
{
  struct addrinfo hints, *res;
  int fd, n;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; //IPv4
  hints.ai_socktype = SOCK_STREAM; //TCP socket
  hints.ai_flags = AI_NUMERICSERV;

  n = getaddrinfo(host, service, &hints, &res);
  if(n != 0)
  {
    if(flag_d) fprintf(stderr, "Error: tcp_socket_connect: getaddrinfo: %s\n", gai_strerror(n));
    freeaddrinfo(res);
    return -1;
  }

  fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(fd == -1)
  {
    if(flag_d) fprintf(stderr, "Error: tcp_socket_connect: socket: %s\n", strerror(errno));
    freeaddrinfo(res);
    return -1;
  }

  n = connect(fd, res->ai_addr, res->ai_addrlen);
  if(n == -1)
  {
    if(flag_d) fprintf(stderr, "Error: tcp_socket_connect: connect: %s\n", strerror(errno));
    close(fd);
    freeaddrinfo(res);
    return -1;
  }

  freeaddrinfo(res);

  return fd;
}

int tcp_send(int nbytes, char *ptr, int fd)
{
  int nleft, nwritten;

  //Certificação de que a string acaba com \0
/*  if(nbytes < BUFFER_SIZE)
  {
      ptr[nbytes] = '\0';
  }
  else
  {
      ptr[BUFFER_SIZE] = '\0';
  }

  //Passa a ter de enviar mais um byte, o \0
  nbytes++;*/
  //nº de bytes que falta enviar é igual ao nº total de bytes
  nleft = nbytes;

  //Envio
  while (nleft > 0)
  {
    //Envia o número de bytes que faltam. Recebe o número de bytes enviado com sucesso
    nwritten = write(fd, ptr, nleft);
    if(nwritten <= 0)
    {
        if(flag_d) fprintf(stderr, "Error: tcp_send: write: %s\n", strerror(errno));
        return -1;
    }
    nleft -= nwritten;
    ptr += nwritten; //incremento do ponteiro até ao primeiro caracter não enviado
  }

  return 0; //success
}

int tcp_receive(int nbytes, char *ptr, int fd)
{
  int nleft, nread;

  nleft = nbytes;

  while (nleft > 0)
  {
    nread = read(fd, ptr, nleft);
    if(nread == -1)
    {
        if(flag_d) fprintf(stderr, "Error: tcp_receive: read: %s\n", strerror(errno));
        return -1;
    }
    else if(nread == 0)
    {
        return 0; //conexão terminada pelo peer
    }
    nleft -= nread;
    ptr += nread;
  }
  nread = nbytes - nleft;
  return nread;
}

 ///////////////////////////// Funções do servidor TCP /////////////////////////////////////////////////


int tcp_bind(char *service)
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
        if(flag_d) fprintf(stderr, "Error: tcp_bind: getaddrinfo: %s\n", gai_strerror(n));
        freeaddrinfo(res);
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(fd == -1)
    {
        if(flag_d) fprintf(stderr, "Error: tcp_bind: socket: %s\n", strerror(errno));
        freeaddrinfo(res);
        return -1;
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1)
    {
        if(flag_d) fprintf(stderr, "Error: tcp_bind: bind: %s\n", strerror(errno));
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    n = listen(fd, MAX_CONNECTIONS_PENDING);
    if(n == -1)
    {
        if(flag_d) fprintf(stderr, "Error: tcp_bind: listen: %s\n", strerror(errno));
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
        if(flag_d) fprintf(stderr, "Error: fd_array_init: malloc: %s\n", strerror(errno));
        exit(1);
    }

    for(i = 0; i<tcp_sessions; i++)
    {
        fd_array[i] = -1;
    }

    return fd_array;
}

void fd_array_set(int *fd_array, fd_set *fdSet, int *maxfd)
{
    int i;

    //Faz set de todos os file descriptors diferentes de -1
    for(i=0; i<MAX_CONNECTIONS; i++)
    {
        if(fd_array[i] != -1)
        {
            FD_SET(fd_array[i], fdSet);
            *maxfd = max(*maxfd, fd_array[i]);
        }
    }
}

void new_connection(int fd, int *fd_array)
{
    struct sockaddr_in addr;
    unsigned int addrlen;
    enum {accepted, refused} state;
    int newfd, i;

    //À falta de indicação em contrário, a ligação é recusada
    state = refused;

    addrlen = sizeof(addr);
    //Recebe um novo pedido de ligação
    if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) {
        if(flag_d) fprintf(stderr, "Error: new_connection: accept: %s\n", strerror(errno));
        exit(1);
    }

    for(i=0; i<MAX_CONNECTIONS; i++)
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
        write(newfd, "Busy\n", 6);
        close(newfd);
    }
}

void tcp_echo_communication(int *fd_array, char *buffer, int i)
{
    int n;

    if((n = read(fd_array[i], buffer, BUFFER_SIZE)) != 0)
    {
        //Caso haja um erro de leitura imprime o erro no ecrã e fecha a ligação
        if(n == -1)
        {
            if(flag_d) fprintf(stderr, "Error: tcp_echo_communications: read: %s\n", strerror(errno));
            close(fd_array[i]);
            fd_array[i] = -1;
        }
        else
        {
            //Caso não haja nenhum erro, envia a mensagem
            write(fd_array[i], buffer, n);
        }
    }
    else
    {
        //Se o valor retornado por read for 0, a ligação foi fechada pelo peer
        close(fd_array[i]);
        fd_array[i] = -1;
    }
}
