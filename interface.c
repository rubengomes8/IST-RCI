#include "interface.h"
#include "utils.h"

extern int flag_d;
extern int flag_b;
extern int ascii;

void interface_root(int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char * ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied, int fd_udp)
{
    char buffer[BUFFER_SIZE];

    int maxfd, counter;
    fd_set fd_read_set;
    int exit_flag = 0;


    while(1)
    {
        FD_ZERO(&fd_read_set);

        //Prepara os file descriptors do servidor de acesso e do stdin para leitura
        FD_SET(fd_udp, &fd_read_set);
        FD_SET(0, &fd_read_set);
        maxfd = fd_udp;

        counter = select(maxfd + 1, &fd_read_set, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        if(counter <= 0)
        {
            if(flag_d) fprintf(stderr, "Error: select: %s\n", strerror(errno));
            exit(1);
        }

        if(FD_ISSET(fd_udp, &fd_read_set))
        {
            popresp(fd_udp, streamID);
        }
        else if(FD_ISSET(0, &fd_read_set))
        {
            exit_flag = read_terminal(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied);
            if(exit_flag == 1) return;
        }
    }
}

void interface_not_root()
{

}

int read_terminal(int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char *ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied)
{
    char buffer[BUFFER_SIZE];


    fgets(buffer, BUFFER_SIZE, stdin);

    if(!strcasecmp(buffer, "streams\n"))
    {
        dump(fd_rs, res_rs);
    }
    else if(!strcasecmp(buffer, "status\n"))
    {
        //Apresentar:
        //Identificação da stream
        printf("StreamID: %s\n", streamID);

        //Indicação se está interrompido


        //Indicação se a aplicação é raiz
        if(is_root)
        {
            printf("I am Root!\n");

            //Endereço IP e porto UDP do servidor de acesso se for raiz
            printf("Servidor de acesso: %s:%s\n", ipaddr, uport);

        }
        else
        {
            printf("I am not Root!\n");
            //Endereço IP e porto TCP do ponto de acesso onde está ligado a montante, se não for raiz
        }

        //Endereço IP e porto TCP do ponto de acesso disponibilizado
        printf("Ponto de ligação disponibilizado: %s:%s\n", ipaddr, tport);


        //Número de sessões TCP suportadas a jusante e indicação de quantas se encontram ocupadas
        printf("%d sessões TCP suportadas a jusante, das quais %d se encontram ocupadas!\n\n\n", tcp_sessions, tcp_occupied);

        //Endereço IP e porto TCP dos pontos de acesso dos pares imediatamente a jusante
    }
    else if(!strcasecmp(buffer, "display on\n"))
    {
        flag_b = 1;
    }
    else if(!strcasecmp(buffer, "display off\n"))
    {
        flag_b = 0;
    }
    else if (!strcasecmp(buffer, "format ascii\n"))
    {
        ascii = 1;
    }
    else if(!strcasecmp(buffer, "format hex\n"))
    {
        ascii = 0;
    }
    else if(!strcasecmp(buffer, "debug on\n"))
    {
        flag_d = 1;
    }
    else if(!strcasecmp(buffer, "debug off\n"))
    {
        flag_d = 0;
    }
    else if(!strcasecmp(buffer, "tree\n"))
    {
        //Apresentar estrutura da transmissão
    }
    else if(!strcasecmp(buffer, "exit\n"))
    {
        if(flag_d)
        {
            printf("A aplicação irá ser terminada...\n");
        }
        return 1;
    }
    else
    {
        printf("Comando inválido!\n");
    }

    return 0;
}
