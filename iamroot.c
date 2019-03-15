#include "udp.h"
#include "tcp.h"
#include "utils.h"
#include "root_server.h"
#include "interface.h"
#include <signal.h>

#define RS_IP "193.136.138.142"
#define RS_PORT "59000"


//Defines relativos aos valores default
#define IPADDR "127.0.0.1"
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


/***************************************************************************** Dúvidas **************************************************************************************
-> temos de criar buffers intermédios para cada um dos file descriptors a jusante e a montante pq quando se recebe pode se receber mais do q 1 mensagem. e assim vai se ao buffer,
le se uma mensagem completa se houver (se nao houver vai se outra vez para o select)

-> neste momento estamos a meter o TCP_receive a ler um valor gigante de 10000 bytes, não é de certeza a melhor implementação. ler 1 a 1 é boa ideia?
****************************************************************************************************************************************************************************/


//Vamos precisar de as usar várias vezes, por isso defini como variáveis globais
int flag_b = FLAG_B; //apresenta os dados da stream na interface quando está a 1
int flag_d = FLAG_D; //apresenta informação detalhada do funcionamento da aplicação quando está a 1
int ascii = ASCII; //apresenta os dados da stream em ascii quando está a 1 e em hexadecimal quando está a 0

int main(int argc, char *argv[])
{
    ///////////////////////////////////// Argumentos de entrada ////////////////////////////////////////////////////////
    char streamID[STREAM_ID_SIZE]; //mystream:193.136.138.142:59000 (exemplo)
    char ipaddr[IP_LEN + 1] = IPADDR; //endereço ip do computador
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
    int fd_pop = -1;
    struct addrinfo *res_pop = NULL;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Conta tentativas de comunicação
    //int counter = 0;

    char buffer[BUFFER_SIZE];

    int is_root = 0;
    int welcome = 0;

	char *msg = NULL;

	//Proteger contra sigpipe

	/*struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;

	if(sigaction(SIGPIPE, &act, NULL) == -1) exit(1);*/

    //Lê e verifica os restantes argumentos
    has_stream = arguments_reading(argc, argv, ipaddr, tport, uport, rsaddr, rsport, &tcp_sessions, &bestpops,
            &tsecs, &flag_h, streamID, streamNAME, streamIP, streamPORT);

    //Se a flag -h estiver ativa, dá uma sinopse dos comandos possíveis
    if(flag_h == 1)
    {
        sinopse();
        exit(0);
    }

    // Liga-se ao servidor de raízes por UDP
    if(flag_d) fprintf(stdout, "A ligar-se ao servidor de raízes...\n");
    fd_rs = udp_socket(rsaddr, rsport, &res_rs);
    if(fd_rs == -1)
    {
        if(flag_d) printf("Falha ao ligar-se ao servidor de raízes...\nA aplicação irá terminar...\n");
        if (res_rs != NULL) freeaddrinfo(res_rs);
        exit(0);
    }

    if(flag_d) fprintf(stdout, "Ligação ao servidor de raízes efetuada com sucesso!\n");


    if(has_stream)
    {
        ///////////////////////////////// Descobre se é raíz ou não /////////////////////////////////////////////////////
        msg = find_whoisroot(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport);
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////



        //Verifica se a resposta a WHOISROOT foi ERROR, URROOT ou ROOTIS
        if(!strcmp(msg, "ERROR")) //Recebeu Error
        {
            printf("Verifique se o identificador da stream está correto!\n");
            if(msg != NULL) free(msg);
            if(fd_rs != -1) close(fd_rs);
            if(res_rs != NULL) freeaddrinfo(res_rs);
            exit(-1);
        }
        else
        {
            strncpy(buffer, msg, 6);
            buffer[6] = '\0';

            if(!strcmp(buffer, "URROOT"))
            {
                if(msg != NULL) free(msg);
                //A aplicação fica registada como sendo a raiz da nova árvore e escoamento
                is_root = 1;

                //////////////////// 1. Estabelecer sessão TCP com o servidor fonte /////////////////////////////////////
                fd_ss = source_server_connect(fd_rs, res_rs, res_ss, streamIP, streamPORT);


                ////////////////// 2. Instalar servidor TCP para o ponto de acesso a jusante ////////////////////////////
                fd_tcp_server = install_tcp_server(tport, fd_rs, res_rs, fd_ss, res_ss, ipaddr, tcp_sessions);


                //Cria array com tamanho tcp_sessions para ligações a jusante
                fd_array = create_fd_array(tcp_sessions, fd_rs, fd_ss, res_rs, res_ss);


                ////////////////////// 3. instalar o servidor UDP de acesso de raiz /////////////////////////////////////
                fd_udp = install_access_server(fd_rs, fd_ss, res_rs, res_ss, &res_udp, uport, fd_array);


                //////////////////////////// 4. executar a interface de utilizador //////////////////////////////////////
                interface_root(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied, fd_udp,
                        fd_tcp_server, fd_array, bestpops);

            }
            else if (!strcmp(buffer, "ROOTIS"))
            {
                //Obtém o IP e o porto do servidor de acesso
                get_root_access_server(rasaddr, rasport, msg, res_rs, fd_rs);

                ///////////// 1. Solicita ao servidor de acesso da raíz o IP e porto TCP do ponto de acesso ////////////
                fd_udp = get_access_point(rasaddr, rasport, &res_udp, fd_rs, res_rs, pop_addr, pop_tport);

                while(welcome == 0) //Enquanto não tiver recebido um WELCOME com a stream esperada
                {
                    if(flag_d)
                    {
                        printf("A estabelecer ligação TCP com um peer...\n");
                    }

                    //////////////////////// 2. Estabelece sessão TCP com o ponto de acesso ///////////////////////////
                    fd_pop = connect_to_peer(pop_addr, pop_tport, fd_rs, fd_udp, res_rs, res_udp, res_pop);


                    //////////////////////////// 3. Aguarda confirmação de adesão /////////////////////////////////////
                    welcome = wait_for_confirmation(pop_addr, pop_tport, fd_rs, res_rs, fd_udp, res_udp, fd_pop, res_pop,
                            streamID);
                }



                ////////////////// 4. Instala servidor TCP para o ponto de acesso a jusante ////////////////////////////
                fd_tcp_server = install_tcp_server_not_root(tport, fd_rs, res_rs, fd_ss, res_ss, ipaddr, tcp_sessions, fd_udp, res_udp);

                //Cria array com tamanho tcp_sessions para ligações a jusante
                fd_array = create_fd_array(tcp_sessions, fd_rs, fd_ss, res_rs, res_ss);


                //////////////// 5. Envia a montante a informação do novo ponto de acesso //////////////////////////////
                //Enviar port TCP para o peer de cima a mensagem NEW_POP ---> NP<SP><ipaddr>:<tport><LF>
                //Em que ipaddr e tport representam o IP e o porto do novo ponto de adesão
                send_new_pop(fd_pop, ipaddr, tport, fd_rs, res_rs, fd_tcp_server, res_tcp, fd_udp, res_udp, fd_array);

                ////////////////////////// 6. Executar a interface de utilizador ////////////////////////////////////////
                interface_not_root(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied, fd_udp,
                        fd_tcp_server, fd_array, bestpops, fd_pop);
                /////////////////////////////////////////////////////////////////////////////////////////////////////////
            }
        }
    }
    else
    {
        //Imprime a lista de streams disponíveis
        dump(fd_rs, res_rs);
    }

    //Fecha a stream
    if(is_root)
    {
        remove_stream(fd_rs, res_rs, streamID);
    }

    //Liberta os ponteiros para addrinfo e fecha os file descripotrs
    free_and_close(is_root, fd_rs, fd_udp, fd_pop, fd_ss, res_rs, res_udp, res_pop, res_ss, fd_array);

    
}

// ./iamroot jsrg:127.0.0.1:59000 -i 127.0.0.1 -d 

// ./iamroot jsrg:127.0.0.1:59000 -i 127.0.0.2 -d
