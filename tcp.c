#include "tcp.h"

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
    fprintf(stderr, "Error: getaddrinfo: %s\n", gai_strerror(n));
    exit(1);
  }

  fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(fd == -1)
  {
    fprintf(stderr, "Error: socket: %s\n", strerror(errno));
    exit(1);
  }

  n = connect(fd, res->ai_addr, res->ai_addrlen);
  if(n == -1)
  {
    fprintf(stderr, "Error: connect: %s\n", strerror(errno));
    exit(1);
  }

  freeaddrinfo(res);

  return fd;
}

void tcp_send(int nbytes, char *ptr, int fd)
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
      fprintf(stderr, "Error: write: %s\n", strerror(errno));
      exit(1);
    }
    nleft -= nwritten;
    ptr += nwritten; //incremento do ponteiro até ao primeiro caracter não enviado
  }
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
      fprintf(stderr, "Error: read: %s\n", strerror(errno));
      exit(1);
    }
    else if(nread == 0)
    {
      break; //conexão terminada pelo peer
    }
    nleft -= nread;
    ptr += nread;
  }
  nread = nbytes - nleft;
  return nread;
}
