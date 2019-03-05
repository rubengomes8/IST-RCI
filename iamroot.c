#include "udp.h"
#include "tcp.h"
#include "utils.h"
#include "root_server.h"
#include "interface.h"






#define RS_IP "193.136.138.142"
#define RS_PORT "59000"



//Vamos precisar de as usar várias vezes, por isso defini como variáveis globais
int flag_b = 1; //apresenta os dados da stream na interface quando está a 1
int flag_d = 0; //apresenta informação detalhada do funcionamento da aplicação quando está a 1
int ascii = 1; //apresenta os dados da stream em ascii quando está a 1 e em hexadecimal quando está a 0

int main(int argc, char *argv[])
{
    ///////////////////////////////////// Argumentos de entrada ////////////////////////////////////////////////////////
    char streamID[STREAM_ID_SIZE]; //mystream:193.136.138.142:59000 (exemplo)
    char ipaddr[IP_LEN + 1];
    char tport[PORT_LEN + 1] = "58000"; //tport - porto TCP onde a app aceita sessões de outros pares a jusante
    char uport[PORT_LEN + 1] = "58000"; //uport - porto UDP do servidor de acesso
    char rsaddr[IP_LEN + 1] = "193.136.138.142"; //endereço IP do servidor de raízes
    char rsport[PORT_LEN + 1] = "59000"; //porto UDP do servidor de raízes
    int tcp_sessions = 1; //número de sessões TCP que a aplicação aceita a jusante
    int bestpops = 1; //número de pontos de acesso, não inferior a um, a recolher por esta instância da aplicação, quando raiz, durante a descoberta de pontos de acesso
    int tsecs = 5; //período, em segundos, associado ao registo periódico que a raiz deve fazer no servidor de raízes
    int flag_h = 0; //apresenta a sinopse da linha de comandos associada à invocação da aplicação e termina de seguida, quando está a 1
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
    int fd_tcp_server;
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
    char pop_tport[PORT_LEN];
    char pop_addr[IP_LEN];
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    char buffer[BUFFER_SIZE];

    int is_root = 0;

	char *msg = NULL;
	char *token = NULL;


	//Verifica se a stream foi especificada e se é válida
    has_stream = validate_stream(argc, argv[1], streamID, streamNAME, streamIP, streamPORT);

    //Lê e verifica os restantes argumentos
    arguments_reading(argc, argv, has_stream, ipaddr, tport, uport, rsaddr, rsport, &tcp_sessions, &bestpops, &tsecs, &flag_h);

    if(flag_h == 1)
    {
        sinopse();
        exit(0);
    }

    // Conectar-se ao servidor de raízes com protocolo UDP
    fd_rs = udp_socket(rsaddr, rsport, &res_rs);

    if(has_stream)
    {
        msg = who_is_root(fd_rs, res_rs, streamID, rsaddr, rsport, ipaddr, uport);

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

                if(flag_d)
                {
                    printf("Ligação com o servidor fonte estabelecida com sucesso!\n");
                }

                //2. instalar servidor TCP para o ponto de acesso a jusante
                //Cria ponto de comunicação no porto tport
                fd_tcp_server = tcp_bind(tport);

                //Cria array com tamanho tcp_sessions para ligações a jusante
                fd_array = fd_array_init(tcp_sessions);

                //3. instalar o servidor UDP de acesso de raiz
                fd_udp = udp_socket(NULL, uport, &res_udp);
                udp_bind(fd_udp, res_udp);

                //4. executar a interface de utilizador
                interface(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied, fd_udp);

            }
            else if (!strcmp(buffer, "ROOTIS"))
            {
                //Obtém o IP e o porto do servidor de acesso
                token = strtok(msg, " ");
                token = strtok(NULL, " ");

                token = strtok(NULL, ":");
                if(token == NULL && flag_d)
                {
                    printf("ipaddr inválido!\n");
                    exit(1);
                }
                strcpy(rasaddr, token);

                token = strtok(NULL, "\n");
                if(token == NULL && flag_d)
                {
                    printf("uport inválido!\n");
                    exit(1);
                }
                strcpy(rasport, token);

                //caso já exista uma raiz associada à stream, a aplicação deverá
                //1. solicitar ao servidor de acesso da raiz o IP e porto TCP do ponto de acesso
                fd_udp = udp_socket(rasaddr, rasport, &res_udp);

                popreq(fd_udp, res_udp, pop_addr, pop_tport);

                printf("pop_addr %s\n", pop_addr);
                printf("pop_tport %s\n", pop_tport);

                //2. estabelecer sessão TCP com o ponto de acesso

                //3. aguardar confirmação de adesão

            }
            free(msg);
        }
    }
    else
    {
        dump(fd_rs, res_rs);
    }
    if(res_rs != NULL)
    {
        freeaddrinfo(res_rs);
    }

    if(res_udp != NULL)
    {
        freeaddrinfo(res_udp);
    }

    if(is_root)
    {
        remove_stream(fd_rs, res_rs, streamID);
    }

    if(fd_udp != -1) close(fd_udp);
    if(fd_rs != -1) close(fd_rs);
    if(fd_tcp_server != -1) close(fd_tcp_server);
    if(fd_ss != -1) close(fd_ss);
    if(fd_array != NULL) free(fd_array);
}


