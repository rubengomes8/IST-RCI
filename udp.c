#include "udp.h"

extern int flag_d;
extern int flag_b;

int udp_socket(char *host, char *service, struct addrinfo **res)
{
  struct addrinfo hints, *aux;
  int errcode, fd;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; //IPv4
  hints.ai_socktype = SOCK_DGRAM; //UDP socket
  hints.ai_flags = AI_NUMERICSERV;

  errcode = getaddrinfo(host, service, &hints, res);
  if(errcode != 0)
  {
    if(flag_d) fprintf(stderr, "Erro: udp_socket: getaddrinfo: %s\n", gai_strerror(errcode));
    return -1;
  }

  aux = *res;

  fd = socket(aux->ai_family, aux->ai_socktype, aux->ai_protocol);
  if(fd == -1)
  {
    if(flag_d) fprintf(stderr, "Erro: udp_socket: socket: %s\n", strerror(errno));
    return -1;
  }

  return fd;
}

void udp_send(int fd, char* msg, int msg_len, int flags, struct addrinfo *res)
{
  int n;

  n = sendto(fd, msg, msg_len, flags, res->ai_addr, res->ai_addrlen);
  if(n == -1)
  {
    if(flag_d) fprintf(stderr, "Erro: udp_send: sendto: %s\n", strerror((errno)));
    exit(1);
  }
}


int udp_receive(int fd, int *msg_len, char* buffer, int flags, struct sockaddr_in *addr, unsigned int *addrlen) {
  int n;
  struct sockaddr_in addr_aux;

  addr_aux = *addr;

  //Se não for inicializado pode nem sempre funcionar
  *addrlen = sizeof(addr_aux);

  // n é o número de caracteres recebidos
  n = recvfrom(fd, buffer, *msg_len, flags, (struct sockaddr *)&addr_aux, addrlen);
  if (n == -1)
  {
    if(flag_d) fprintf(stderr, "Erro: udp_receive: recvfrom failed: %s\n", strerror(errno));
    return -1;
  }

  //Certificação de que a stream acaba em \0
  if(*msg_len > n)
  {
    buffer[n] = '\0';
  }
  else
  {
    buffer[*msg_len - 1] = '\0';
  }

  *msg_len = n;
  *addr = addr_aux;

  return n;
}

///////////////////////////////////////// Funções UDP Server ///////////////////////////////////////////////////////////

int udp_bind(int fd, struct addrinfo *res)
{
  int n;

  //Serve para registar o nome do servidor e a porta, associando-os ao socket apontado por fd
  n = bind(fd, res->ai_addr, res->ai_addrlen);
  if(n == -1)
  {
    if(flag_d) fprintf(stderr, "Erro: udp_bind: bind: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}

void udp_answer(int fd, char* msg, int msg_len, int flags, struct sockaddr *addr, int addrlen)
{
  int n;

  n = sendto(fd, msg, msg_len, flags, addr, addrlen);
  if(n == -1)
  {
    if(flag_d) fprintf(stderr, "Erro: udp_answer: sendto: %s\n", strerror((errno)));
    exit(1);
  }
}
