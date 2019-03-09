#include "interface.h"
#include "utils.h"

extern int flag_d;
extern int flag_b;
extern int ascii;

void interface_root(int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char * ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied, int fd_udp, int fd_tcp_server, int *fd_array)
{
    int maxfd, counter;
    fd_set fd_read_set;
    int exit_flag = 0;
    int i;
    printf("\n\nINTERFACE DE UTILIZADOR\n\n");

    //Neste ciclo, 
    while(1)
    {

        FD_ZERO(&fd_read_set);

        //Prepara os file descriptors do servidor de acesso e do stdin para leitura 
        FD_SET(fd_udp, &fd_read_set);
        FD_SET(0, &fd_read_set);
        maxfd = fd_udp;
        //Prepara os file descriptors do servidor tcp que aceita ligações de outros pares que servem para leitura
        FD_SET(fd_tcp_server, &fd_read_set);
        maxfd = max(maxfd, fd_tcp_server);
        //Prepara os file descriptors do array de file descriptors para comunicação TCP a jusante que podem servir para escrita e para leitura
        fd_array_set(fd_array, &fd_read_set, &maxfd, tcp_sessions);


        counter = select(maxfd + 1, &fd_read_set, (fd_set *)NULL, (fd_set *)NULL,  (struct timeval *)NULL);
        if(counter <= 0)
        {
            if(flag_d) fprintf(stderr, "Error: select: %s\n", strerror(errno));
            return;
        }


        //Escreve para os pares TCP a jusante
        for(i = 0; i<tcp_sessions; i++)
        {
            if(fd_array[i] != -1)
            {
                if(FD_ISSET(fd_array[i], &fd_read_set))
                {
                    //Enviar e receber mensagens
                }
            }
        }

        //Se chegar uma nova ligação a jusante -> Aceita ou redireciona novas ligações consoante a ocupação
        if(FD_ISSET(fd_tcp_server, &fd_read_set))
        {
            if(flag_d) printf("Novo pedido de conexão...\n");
            if(tcp_sessions > tcp_occupied)
            {
                welcome(tcp_sessions, &tcp_occupied, fd_tcp_server, fd_array, streamID);
            }
            else if(tcp_sessions == tcp_occupied)
            {
                //Encontrar/Obter o ip e o porto para redirecionar
                redirect(fd_tcp_server, "127.0.0.1", "57000");
            }
        }

        //Servidor de acessos - fizeram um pedido POPREQ
        if(FD_ISSET(fd_udp, &fd_read_set))
        {
            popresp(fd_udp, streamID, "127.0.0.1", "58001");
        }

        //O utilizador introduziu um comando de utilizador
        if(FD_ISSET(0, &fd_read_set))
        {
            exit_flag = read_terminal(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied);
            if(exit_flag == 1) return;
        }
    }
}

void interface_not_root(int fd_rs, struct addrinfo *res_rs, char* streamID, int is_root, char* ipaddr, char *uport,
        char *tport, int tcp_sessions, int tcp_occupied, int fd_udp, int fd_tcp_server, int *fd_array)
{
    int maxfd, counter;
    fd_set fd_read_set;
    int exit_flag = 0;
    int i;

    printf("\n\nINTERFACE DE UTILIZADOR\n\n");

    while(1)
    {
        FD_ZERO(&fd_read_set);
        FD_SET(0, &fd_read_set);
        maxfd = 0;
        FD_SET(fd_tcp_server, &fd_read_set);
        maxfd = max(maxfd, fd_tcp_server);
        fd_array_set(fd_array, &fd_read_set, &maxfd, tcp_sessions);





        counter = select(maxfd + 1, &fd_read_set, (fd_set *)NULL, (fd_set *)NULL, (struct timeval*)NULL);
        if(counter <= 0)
        {
            if(flag_d) fprintf(stderr, "Error: select: %s\n", strerror(errno));
            return;
        }

        //Escreve para os pares TCP a jusante
        for(i = 0; i<tcp_sessions; i++)
        {
            if(fd_array[i] != -1)
            {
                if(FD_ISSET(fd_array[i], &fd_read_set))
                {
                    //Enviar e receber mensagens
                }
            }
        }

        //Aceita ou redireciona novas ligações
        if(FD_ISSET(fd_tcp_server, &fd_read_set))
        {
            if(flag_d) printf("Novo pedido de conexão...\n");
            if(tcp_sessions > tcp_occupied)
            {
                welcome(tcp_sessions, &tcp_occupied, fd_tcp_server, fd_array, streamID);
            }
            else if(tcp_sessions == tcp_occupied)
            {
                redirect(fd_tcp_server, "127.0.0.1", "57000");
            }
        }

        if(FD_ISSET(0, &fd_read_set))
        {
            exit_flag = read_terminal(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied);
            if(exit_flag == 1)return;
        }
    }
}

int read_terminal(int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char *ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied)
{
    char buffer[BUFFER_SIZE];
    int counter = 0; //conta tentativas de chamada de uma função

    fgets(buffer, BUFFER_SIZE, stdin);

    if(!strcasecmp(buffer, "streams\n"))
    {
        while(dump(fd_rs, res_rs) == -1)
        {
            counter++;
            if(counter == MAX_TRIES)
            {
                if(flag_d)
                {
                    printf("\n");
                    printf("Impossível comunicar com o servidor de raízes, após %d tentativas...\n", MAX_TRIES);
                    printf("A terminar o programa...\n");
                    return 1;
                }
            }
        }
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
        if(flag_d) printf("Display on\n");
        flag_b = 1;
    }
    else if(!strcasecmp(buffer, "display off\n"))
    {
        if(flag_d) printf("Display off\n");
        flag_b = 0;
    }
    else if (!strcasecmp(buffer, "format ascii\n"))
    {
        if(flag_d) printf("Alteração para o formato ascii efetuada\n");
        ascii = 1;
    }
    else if(!strcasecmp(buffer, "format hex\n"))
    {
        if(flag_d) printf("Alteração para o formato hexadecimal efetuada\n");
        ascii = 0;
    }
    else if(!strcasecmp(buffer, "debug on\n"))
    {
        if(flag_d) printf("Debug on\n");
        flag_d = 1;
    }
    else if(!strcasecmp(buffer, "debug off\n"))
    {
        if(flag_d) printf("Debug on\n");
        flag_d = 0;
    }
    else if(!strcasecmp(buffer, "tree\n"))
    {
        //Apresentar estrutura da transmissão
        if(flag_d) printf("Estrutura de transmissão em árvore\n");
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

void welcome(int tcp_sessions, int *tcp_occupied, int fd_tcp_server, int *fd_array, char *streamID)
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

}

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
