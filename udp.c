#include "udp.h"

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
    fprintf(stderr, "Error: getaddrinfo: %s\n", gai_strerror(errcode));
    exit(1);
  }

  aux = *res;

  fd = socket(aux->ai_family, aux->ai_socktype, aux->ai_protocol);
  if(fd == -1)
  {
    fprintf(stderr, "Error: socket: %s\n", strerror(errno));
    exit(1);
  }

  return fd;
}

void udp_send(int fd, char* msg, int msg_len, int flags, struct addrinfo *res)
{
  int n;

  n = sendto(fd, msg, msg_len, flags, res->ai_addr, res->ai_addrlen);
  if(n == -1)
  {
    fprintf(stderr, "Error: sendto: %s\n", strerror((errno)));
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
  if (n == -1) {
    fprintf(stderr, "Error: recvfrom failed: %s\n", strerror(errno));
    exit(1);
  }



  // Certificação de que a string acaba em \0. É provável que seja necessário acrescentar mais verificações no futuro
  // Talvez seja também melhor passar a alocar strings dinamicamente
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


// Telvez seja bom/preciso pôr isto a retornar o host e o service em vez de os imprimir
void print_sender(struct sockaddr_in addr, unsigned int addrlen, int flags)
{
  char host[NI_MAXHOST], service[NI_MAXSERV]; //constantes vindas de netdb.h
  int errcode = 0;

  if((errcode=getnameinfo((struct sockaddr *)&addr, addrlen, host, sizeof(host), service, sizeof(service), flags)) != 0)
  {
    fprintf(stderr, "Error: getnameinfo: %s\n", gai_strerror(errcode));
    exit(1);
  }
  else
  {
    printf("Sent by [%s:%s]\n", host, service);
  }
}


//A partir daqui são funções do udp_server.c

void udp_bind(int fd, struct addrinfo *res)
{
  int n;

  //Serve para registar o nome do servidor e a porta, associando-os ao socket apontado por fd
  n = bind(fd, res->ai_addr, res->ai_addrlen);
  if(n == -1)
  {
    fprintf(stderr, "Error: bind: %s\n", strerror(errno));
    exit(1);
  }
}
