#include "udp.h"
#include "tcp.h"
#include "utils.h"
#include "root_server.h"
#include "interface.h"

#define RS_IP "193.136.138.142"
#define RS_PORT "59000"


//Defines relativos aos valores default
#define TPORT "58000"
#define UPORT "58000"
#define RSADDR "193.136.138.142"
#define RSPORT "59000"
#define TCP_SESSIONS 1
#define BESTPOPS 1
#define TSECS 5
#define FLAG_H 0
#define FLAG_B 1
#define FLAG_D 0
#define ASCII 1


//Vamos precisar de as usar várias vezes, por isso defini como variáveis globais
int flag_b = FLAG_B; //apresenta os dados da stream na interface quando está a 1
int flag_d = FLAG_D; //apresenta informação detalhada do funcionamento da aplicação quando está a 1
int ascii = ASCII; //apresenta os dados da stream em ascii quando está a 1 e em hexadecimal quando está a 0

int main(int argc, char *argv[])
{
    ///////////////////////////////////// Argumentos de entrada ////////////////////////////////////////////////////////
    char streamID[STREAM_ID_SIZE]; //mystream:193.136.138.142:59000 (exemplo)
    char ipaddr[IP_LEN + 1];
    char tport[PORT_LEN + 1] = TPORT; //tport - porto TCP onde a app aceita sessões de outros pares a jusante
    char uport[PORT_LEN + 1] = UPORT; //uport - porto UDP do servidor de acesso
    char rsaddr[IP_LEN + 1] = RSADDR; //endereço IP do servidor de raízes
    char rsport[PORT_LEN + 1] = RSPORT; //porto UDP do servidor de raízes
    int tcp_sessions = TCP_SESSIONS; //número de sessões TCP que a aplicação aceita a jusante
    int bestpops = BESTPOPS; //número de pontos de acesso, não inferior a um, a recolher por esta instância da aplicação, quando raiz, durante a descoberta de pontos de acesso
    int tsecs = TSECS; //período, em segundos, associado ao registo periódico que a raiz deve fazer no servidor de raízes
    int flag_h = FLAG_H; //apresenta a sinopse da linha de comandos associada à invocação da aplicação e termina de seguida, quando está a 1
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    int has_stream = 0;
    int tcp_occupied = 0;

    //////////////////////////// Variáveis que irão alojar o nome, ip e port da stream /////////////////////////////////
    char streamNAME[STREAM_NAME_LEN];
    char streamIP[IP_LEN + 1];
    char streamPORT[PORT_LEN + 1];
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////// Comunicação com o servidor de raízes /////////////////////////////////////////////////
    int fd_rs = -1;
    struct addrinfo *res_rs = NULL;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    ///////////////////////////// Comunicação com o servidor de raízes /////////////////////////////////////////////////
    int fd_ss = -1;
    struct addrinfo *res_ss = NULL;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////// Śervidor TCP - aceita conexões a jusante /////////////////////////////////////////////
    int fd_tcp_server = -1;
    struct addrinfo *res_tcp = NULL;
    int *fd_array = NULL;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////// Servidor UDP de acesso ////////////////////////////////////////////////////////
    int fd_udp = -1;
    struct addrinfo *res_udp = NULL;
    char rasaddr[IP_LEN + 1]; //endereço IP do servidor de acesso da raiz
    char rasport[PORT_LEN + 1]; //porto UDP do servidor de acesso da raiz
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////// Comunicação com par a montante ////////////////////////////////////////////////
    char pop_tport[PORT_LEN + 1];
    char pop_addr[IP_LEN + 1];
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //Conta tentativas de comunicação
    int counter = 0;

    char buffer[BUFFER_SIZE];

    int is_root = 0;

	char *msg = NULL;
	char *token = NULL;

    //Lê e verifica os restantes argumentos
    has_stream = arguments_reading(argc, argv, ipaddr, tport, uport, rsaddr, rsport, &tcp_sessions, &bestpops,
            &tsecs, &flag_h, streamID, streamNAME, streamIP, streamPORT);


    if(flag_h == 1)
    {
        sinopse();
        exit(0);
    }

    // Conectar-se ao servidor de raízes com protocolo UDP
    fd_rs = udp_socket(rsaddr, rsport, &res_rs);
    if(fd_rs == -1)
    {
        if(flag_d) printf("A aplicação irá terminar...\n");
        freeaddrinfo(res_rs);
        exit(1);
    }

    if(has_stream)
    {
        msg = who_is_root(fd_rs, res_rs, streamID, rsaddr, rsport, ipaddr, uport);
        //Enquanto receber NULL, significa que não houve resposta do servidor de raízes
        while(msg == NULL)
        {
            counter++;
            if(counter == MAX_TRIES)
            {
                if(flag_d)
                {
                    printf("\n");
                    printf("Impossível comunicar com o servidor de raízes, após %d tentativas...\n", MAX_TRIES);
                    printf("A terminar o programa...\n");
                }
                freeaddrinfo(res_rs);
                close(fd_rs);
                free(msg);
                exit(0);
            }
            who_is_root(fd_rs, res_rs, streamID, rsaddr, rsport, ipaddr, uport);
        }
        counter = 0; //Reset do contador, caso tenha sido possível comunicar

        if(!strcmp(msg, "ERROR")) //Recebeu Error
        {
            printf("Verifique que o identificador da stream está corretamente formatado\n");
            free(msg);
            close(fd_rs);
            freeaddrinfo(res_rs);
            exit(-1);
        }
        else
        {
            strncpy(buffer, msg, 6);
            buffer[6] = '\0';

            if(!strcmp(buffer, "URROOT")) //caso não haja nenhuma raiz associada ao streamID
            {
                
                //aplicação fica registada como sendo a raiz da nova árvore e escoamento
                is_root = 1;

                //1. Estabelecer sessão TCP com o servidor fonte
                if(flag_d)
                {
                    printf("A estabelecer ligação TCP com o servidor fonte...\n");
                }

            	fd_ss = tcp_socket_connect(streamIP, streamPORT);
                if(fd_ss == -1)
                {
                    if(flag_d) printf("A aplicação irá terminar...\n");
                    close(fd_rs);
                    freeaddrinfo(res_rs);
                    free(msg);
                    exit(0);
                }

                if(flag_d)
                {
                    printf("Ligação com o servidor fonte estabelecida com sucesso!\n");
                }

                //2. instalar servidor TCP para o ponto de acesso a jusante
                //Cria ponto de comunicação no porto tport
                if(flag_d)
                {
                    printf("A instalar servidor TCP para transmissão a jusante...\n");
                }

                fd_tcp_server = tcp_bind(tport);
                if(fd_tcp_server == -1)
                {
                    if(flag_d) printf("A aplicação irá terminar...\n");
                    close(fd_rs);
                    freeaddrinfo(res_rs);
                    free(msg);
                    exit(0);
                }

                if(flag_d)
                {
                    printf("Servidor TCP para comunicação a jusante instalado com sucesso, no endereço %s:%s\n", ipaddr, tport);
                }

                //Cria array com tamanho tcp_sessions para ligações a jusante
                fd_array = fd_array_init(tcp_sessions);

                //3. instalar o servidor UDP de acesso de raiz
                fd_udp = udp_socket(NULL, uport, &res_udp);
                if(fd_udp == -1)
                {
                    if(flag_d) printf("A aplicação irá terminar...\n");
                    close(fd_rs);
                    freeaddrinfo(res_rs);
                    freeaddrinfo(res_udp);
                    free(msg);
                    free(fd_array);
                    exit(0);
                }

                if(udp_bind(fd_udp, res_udp) == -1)
                {
                    if(flag_d) printf("A aplicação irá terminar...\n");
                    close(fd_rs);
                    freeaddrinfo(res_rs);
                    freeaddrinfo(res_udp);
                    free(msg);
                    free(fd_array);
                    exit(0);
                }

                //4. executar a interface de utilizador
                interface_root(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied, fd_udp);

            }
            else if (!strcmp(buffer, "ROOTIS"))
            {
                //Obtém o IP e o porto do servidor de acesso
                if(get_root_access_server(rasaddr, rasport, msg) == -1)
                {
                    if(flag_d)
                    {
                        printf("Falha em obter o endereço do servidor de acesso...\n");
                        printf("A aplicação irá terminar...\n");
                    }
                    freeaddrinfo(res_rs);
                    close(fd_rs);
                    free(msg);
                }

                //caso já exista uma raiz associada à stream, a aplicação deverá
                //1. solicitar ao servidor de acesso da raiz o IP e porto TCP do ponto de acesso
                fd_udp = udp_socket(rasaddr, rasport, &res_udp);
                if(fd_udp == -1)
                {
                    if(flag_d) printf("A aplicação irá terminar...\n");
                    close(fd_rs);
                    freeaddrinfo(res_rs);
                    freeaddrinfo(res_udp);
                    free(msg);
                    exit(0);
                }

                while(popreq(fd_udp, res_udp, pop_addr, pop_tport) == -1)
                {
                    counter++;
                    if(counter == MAX_TRIES)
                    {
                        if(flag_d)
                        {
                            printf("\n");
                            printf("Impossível comunicar com o servidor de acesso, após %d tentativas...\n", MAX_TRIES);
                            printf("A terminar o programa...\n");
                        }
                        close(fd_rs);
                        freeaddrinfo(res_rs);
                        freeaddrinfo(res_udp);
                        free(msg);
                        exit(0);
                    }
                }

                printf("pop_addr %s\n", pop_addr);
                printf("pop_tport %s\n", pop_tport);

                //2. estabelecer sessão TCP com o ponto de acesso
                if(flag_d)
                {
                    printf("A estabelecer ligação TCP com o um peer...\n");
                }

                fd_ss = tcp_socket_connect("127.0.0.1", "58000");
                if(fd_ss == -1)
                {
                    if(flag_d) printf("A aplicação irá terminar...\n");
                    close(fd_rs);
                    freeaddrinfo(res_rs);
                    free(msg);
                    exit(0);
                }

                if(flag_d)
                {
                    printf("Ligação com o servidor fonte estabelecida com sucesso!\n");
                }


                //3. aguardar confirmação de adesão




                //4. Instalar servidor TCP para o ponto de acesso a jusante
                //Cria ponto de comunicação no porto tport
                if(flag_d)
                {
                    printf("A instalar servidor TCP para transmissão a jusante...\n");
                }

                fd_tcp_server = tcp_bind(tport);
                if(fd_tcp_server == -1)
                {
                    if(flag_d) printf("A aplicação irá terminar...\n");
                    close(fd_rs);
                    freeaddrinfo(res_rs);
                    free(msg);
                    exit(0);
                }

                if(flag_d)
                {
                    printf("Servidor TCP para comunicação a jusante instalado com sucesso, no endereço %s:%s\n", ipaddr, tport);
                }

                //Cria array com tamanho tcp_sessions para ligações a jusante
                fd_array = fd_array_init(tcp_sessions);




                //5. Enviar a montante a informação do novo ponto de acesso






                //6. Executar a interface de utilizador
                interface_not_root(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied, fd_udp);

            }
            free(msg);
        }
    }
    else
    {
        dump(fd_rs, res_rs);
    }

    //Fecha a stream
    if(is_root)
    {
        remove_stream(fd_rs, res_rs, streamID);
    }

    //Libertação de memória
    if(res_rs != NULL)
    {
        freeaddrinfo(res_rs);
    }

    if(res_udp != NULL)
    {
        freeaddrinfo(res_udp);
    }

    if(fd_udp != -1) close(fd_udp);
    if(fd_rs != -1) close(fd_rs);
    if(fd_tcp_server != -1) close(fd_tcp_server);
    if(fd_ss != -1) close(fd_ss);
    if(fd_array != NULL) free(fd_array);
}


