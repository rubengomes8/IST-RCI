#include "udp.h"
#include "tcp.h"
#include "utils.h"
#include "root_server.h"



#define BUFFER_SIZE 64


#define RS_IP "193.136.138.142"
#define RS_PORT "59000"



//Vamos precisar de as usar várias vezes, por isso defini como variáveis globais
int flag_b = 1; //apresenta os dados da stream na interface quando está a 1
int flag_d = 0; //apresenta informação detalhada do funcionamento da aplicação quando está a 1

int main(int argc, char *argv[])
{
    ///////////////////////////////////// Argumentos de entrada ////////////////////////////////////////////////////////
    char streamID[STREAM_ID_SIZE]; //mystream:193.136.138.142:59000 (exemplo)
    char ipaddr[20];
    char tport[6] = "58000"; //tport - porto TCP onde a app aceita sessões de outros pares a jusante
    char uport[6] = "58000"; //uport - porto UDP do servidor de acesso
    char rsaddr[20] = "193.136.138.142"; //endereço IP do servidor de raízes
    char rsport[6] = "59000"; //porto UDP do servidor de raízes
    int tcp_sessions = 1; //número de sessões TCP que a aplicação aceita a jusante
    int bestpops = 1; //número de pontos de acesso, não inferior a um, a recolher por esta instância da aplicação, quando raiz, durante a descoberta de pontos de acesso
    int tsecs = 5; //período, em segundos, associado ao registo periódico que a raiz deve fazer no servidor de raízes
    int flag_h = 0; //apresenta a sinopse da linha de comandos associada à invocação da aplicação e termina de seguida, quando está a 1
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    int has_stream = 0;

    //////////////////////////// Variáveis que irão alojar o nome, ip e port da stream /////////////////////////////////
    char streamNAME[STREAM_NAME_LEN];
    char streamIP[IP_LEN];
    char streamPORT[PORT_LEN];
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////// Comunicação com o servidor de raízes /////////////////////////////////////////////////
    int fd_rs;
    struct addrinfo *res_rs;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////// Comunicação com o servidor fonte ////////////////////////////////////////////////
    int fd_source_server;


    char buffer[BUFFER_SIZE];

	char rasaddr[20]; //endereço IP do servidor de acesso da raiz
	char rasport[6]; //porto UDP do servidor de acesso da raiz
	char *msg = NULL;


	//Verifica se a stream foi especificada e se é válida
    has_stream = validate_stream(argc, argv[1], streamID, streamNAME, streamIP, streamPORT);

    //Lê e verifica os restantes argumentos
    arguments_reading(argc, argv, has_stream, ipaddr, tport, uport, rsaddr, rsport, &tcp_sessions, &bestpops, &tsecs, &flag_h);

    if(flag_h == 1)
    {
        //Apresenta a sinopse da linha de comandos e sai do programa
        exit(0);
    }

    // Conectar-se ao servidor de raízes com protocolo UDP
    fd_rs = udp_socket(rsaddr, rsport, &res_rs);

    if(has_stream)
    {
        msg = who_is_root(fd_rs, res_rs, streamID, rsaddr, rsport);

        if(!strcmp(msg, "ERROR")) //Recebeu Error
        {
            printf("Make sure the stream ID, the IPaddress and the port UDP are well formated\n");
            exit(-1);
        }
        else
        {
            strncpy(buffer, msg, 6);
            buffer[6] = '\0';
            if(!strcmp(buffer, "URROOT"))
            {
                //caso não haja nenhuma raiz associada ao streamID
                //aplicação fica registada como sendo a raiz da nova árvore e escoamento

                //1. Estabelecer sessão TCP com o servidor fonte
                fd_source_server = tcp_socket_connect(); //Não sei que IP pôr aqui

                //2. instalar servidor TCP para o ponto de acesso a jusante

                //3. instalar o servidor UDP de acesso de raiz

                //4. executar a interface de utilizador
            }
            else if (!strcmp(buffer, "ROOTIS"))
            {
                //caso já exista uma raiz associada à stream, a aplicação deverá

                //1. solicitar ao servidor de acesso da raiz o IP e porto TCP do ponto de acesso

                //2. estabelecer sessão TCP com o ponto de acesso

                //3. aguardar confirmação de adesão
            }
        }
        free(msg);
    }
    else
    {
        dump(fd_rs, res_rs);
    }
    close(fd_rs);

}


