#include "iamroot.h"

//Se ligar 2 vezes sem enviar NP, à terceira o root não funciona

/***************************************************************************** Dúvidas **************************************************************************************
-> temos de criar buffers intermédios para cada um dos file descriptors a jusante e a montante pq quando se recebe pode se receber mais do q 1 mensagem. e assim vai se ao buffer,
le se uma mensagem completa se houver (se nao houver vai se outra vez para o select)

-> neste momento estamos a meter o TCP_receive a ler um valor gigante de 10000 bytes, não é de certeza a melhor implementação. ler 1 a 1 é boa ideia?

-> o que se fazer quando se perde a ligação ao servidor fonte?

-> o que acontece se não houver pops suficientes para dar. Ex: bestpops = 5. Tem apenas um filho que aceita tcp_sessions = 1
 logo nunca vai receber 5 pops

-> o que acontece se mandar o WHOISROOT periódico e a resposta não for URROOT?

-> variável de entrada para definir o timeout entre pop_queries??? acho que era bom

 ****************************************************************************************************************************************************************************/


//Vamos precisar de as usar várias vezes, por isso defini como variáveis globais
int flag_b = FLAG_B; //apresenta os dados da stream na interface quando está a 1
int flag_d = FLAG_D; //apresenta informação detalhada do funcionamento da aplicação quando está a 1
int flag_tree = 0;
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
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////// Śervidor TCP - aceita conexões a jusante /////////////////////////////////////////////
    int fd_tcp_server = -1;
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
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Conta tentativas de comunicação
    //int counter = 0;

    char buffer[BUFFER_SIZE];

    int is_root = 0;
    int welcome = 0;

	char *msg = NULL;
	int is_flowing = 1;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    char **aux_buffer_sons = NULL;
    char **aux_ptr_sons = NULL;
    int *nread_sons = NULL;

    struct timeval *timeout = NULL;

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

        //Verifica se a resposta a WHOISROOT foi ERROR, URROOT ou ROOTIS
        if(!strcmp(msg, "ERROR")) //Recebeu Error
        {
            if(flag_d)
            {
                printf("Verifique se o identificador da stream está correto!\n");
                printf("A aplicação irá terminar...\n");
            }
            if(msg != NULL) free(msg);
            if(fd_rs != -1) close(fd_rs);
            if(res_rs != NULL) freeaddrinfo(res_rs);
            exit(0);
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
                fd_ss = source_server_connect(fd_rs, res_rs, streamIP, streamPORT);


                ////////////////// 2. Instalar servidor TCP para o ponto de acesso a jusante ////////////////////////////
                fd_tcp_server = install_tcp_server(tport, fd_rs, res_rs, fd_ss, ipaddr, tcp_sessions);


                //Cria array com tamanho tcp_sessions para ligações a jusante
                fd_array = create_fd_array(tcp_sessions, fd_rs, fd_ss, res_rs);


                ////////////////////// 3. instalar o servidor UDP de acesso de raiz /////////////////////////////////////
                fd_udp = install_access_server(ipaddr, fd_rs, fd_ss, res_rs, &res_udp, uport, fd_array);

                //////////////////////////// 4. executar a interface de utilizador //////////////////////////////////////
                interface_root(fd_ss, fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied,
                        fd_udp, fd_tcp_server, fd_array, bestpops, NULL, NULL, NULL, 1, tsecs, rsaddr, rsport,
                        aux_buffer_sons, aux_ptr_sons, nread_sons, is_flowing, timeout);

            }
            else if (!strcmp(buffer, "ROOTIS"))
            {
                //Obtém o IP e o porto do servidor de acesso
                get_root_access_server(rasaddr, rasport, msg, res_rs, fd_rs);

                ///////////// 1. Solicita ao servidor de acesso da raíz o IP e porto TCP do ponto de acesso ////////////
                do
                {
                    fd_udp = get_access_point(rasaddr, rasport, &res_udp, fd_rs, res_rs, pop_addr, pop_tport, 0, ipaddr);
                }while(fd_udp == -2);

                while(welcome == 0) //Enquanto não tiver recebido um WELCOME com a stream esperada
                {
                    //////////////////////// 2. Estabelece sessão TCP com o ponto de acesso ///////////////////////////
                    fd_pop = connect_to_peer(pop_addr, pop_tport, fd_rs, fd_udp, res_rs, 0);

                    //////////////////////////// 3. Aguarda confirmação de adesão /////////////////////////////////////
                    welcome = wait_for_confirmation(pop_addr, pop_tport, fd_rs, res_rs, fd_udp, fd_pop, streamID);
                }
                close(fd_udp);
                fd_udp = -1;

                ////////////////// 4. Instala servidor TCP para o ponto de acesso a jusante ////////////////////////////
                fd_tcp_server = install_tcp_server(tport, fd_rs, res_rs, fd_ss, ipaddr, tcp_sessions);

                //Cria array com tamanho tcp_sessions para ligações a jusante
                fd_array = create_fd_array(tcp_sessions, fd_rs, fd_ss, res_rs);

                //////////////// 5. Envia a montante a informação do novo ponto de acesso //////////////////////////////
                //Enviar port TCP para o peer de cima a mensagem NEW_POP ---> NP<SP><ipaddr>:<tport><LF>
                //Em que ipaddr e tport representam o IP e o porto do novo ponto de adesão
                send_new_pop(fd_pop, ipaddr, tport, fd_rs, res_rs, fd_tcp_server, fd_array);
                ////////////////////////// 6. Executar a interface de utilizador ////////////////////////////////////////
                interface_not_root(fd_rs, res_rs, streamID, streamIP, streamPORT, &is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied,
                        fd_tcp_server, fd_array, bestpops, fd_pop, pop_addr, pop_tport, rsaddr, rsport, tsecs);
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
    free_and_close(is_root, fd_rs, fd_udp, fd_pop, fd_ss, res_rs, fd_array);

    
}

//Envia ao servidor de raízes a mensagem who_is_root e recebe a resposta. Retorna essa resposta em caso de sucesso
//ou NULL em caso de erro
char *find_whoisroot(struct addrinfo *res_rs, int fd_rs, char *streamID, char *rsaddr, char *rsport, char *ipaddr, char *uport){
    int counter = 0;
    char *msg = NULL;

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
            if (res_rs != NULL) freeaddrinfo(res_rs);
            if (fd_rs != -1) close(fd_rs);
            exit(0);
        }
        msg = who_is_root(fd_rs, res_rs, streamID, rsaddr, rsport, ipaddr, uport);
    }
    return msg;
}

//Faz a ligação ao servidor fonte da stream
int source_server_connect(int fd_rs, struct addrinfo *res_rs, char *streamIP, char *streamPORT)
{
    int fd_ss = -1;

    if(flag_d)
    {
        printf("A estabelecer ligação TCP com o servidor fonte, no endereço %s:%s...\n", streamIP, streamPORT);
    }

    fd_ss = tcp_socket_connect(streamIP, streamPORT);
    if(fd_ss == -1)
    {
        if(flag_d)
        {
            printf("Erro na comunicação TCP com o servidor fonte...\n");
            printf("A aplicação irá terminar...\n");
        }
        if(fd_rs != -1) close(fd_rs);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        exit(0);
    }

    if(flag_d)
    {
        printf("Ligação com o servidor fonte estabelecida com sucesso!\n");
    }

    return fd_ss;
}

//Instala um servidor TCP para comunicação a jusante
int install_tcp_server(char *tport, int fd_rs, struct addrinfo *res_rs, int fd_ss, char *ipaddr, int tcp_sessions)
{
    int fd_tcp_server = -1;


    //Cria ponto de comunicação no porto tport
    if(flag_d)
    {
        printf("A instalar servidor TCP para transmissão a jusante, no endereço %s:%s...\n", ipaddr, tport);
    }

    fd_tcp_server = tcp_bind(tport, tcp_sessions);
    if(fd_tcp_server == -1)
    {
        if(flag_d)
        {
            printf("Erro ao instalar o servidor TCP para transmissão a jusante...\n");
            printf("A aplicação irá terminar...\n");
        }
        if(fd_rs != -1) close(fd_rs);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(fd_ss != -1) close(fd_ss);
        exit(0);
    }

    if(flag_d)
    {
        printf("Servidor TCP para comunicação a jusante instalado com sucesso, no endereço %s:%s\n", ipaddr, tport);
    }

    return fd_tcp_server;
}

//Cria um array para armazenar os file descriptors dos pares a jusante
int *create_fd_array(int tcp_sessions, int fd_rs, int fd_ss, struct addrinfo *res_rs)
{
    int *fd_array;

    fd_array = fd_array_init(tcp_sessions);
    if(fd_array == NULL)
    {
        if(fd_rs != -1) close(fd_rs);
        if(fd_ss != -1) close(fd_ss);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        exit(0);
    }

    return fd_array;
}

//Instala servidor de acessos UDP
int install_access_server(char *ipaddr, int fd_rs, int fd_ss, struct addrinfo *res_rs, struct addrinfo **res_udp, char *uport, int *fd_array)
{
    int fd_udp = -1;

    if(flag_d) printf("A instalar servidor de acessos UDP no endereço %s:%s...\n", ipaddr, uport);
    fd_udp = udp_socket(ipaddr, uport, res_udp);
    if(fd_udp == -1)
    {
        if(flag_d)
        {
            printf("Erro ao criar o socket UDP para o servidor de acessos...\n");
            printf("A aplicação irá terminar...\n");
        }
        if(fd_rs != -1) close(fd_rs);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(fd_ss != -1) close(fd_ss);
        if(*res_udp != NULL) freeaddrinfo(*res_udp);
        if(fd_array != NULL) free(fd_array);
        exit(0);
    }

    if(udp_bind(fd_udp, *res_udp) == -1)
    {
        if(flag_d)
        {
            printf("Erro ao criar o socket UDP para o servidor de acessos...\n");
            printf("A aplicação irá terminar...\n");
        }
        if(fd_rs != -1) close(fd_rs);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(fd_ss != -1) close(fd_ss);
        if(fd_udp != -1) close(fd_udp);
        if(*res_udp != NULL) freeaddrinfo(*res_udp);
        if(fd_array != NULL) free(fd_array);
        exit(0);
    }

    freeaddrinfo(*res_udp);
    return fd_udp;
}

//Retira de msg o ip e porto do servidor de acessos UDP
void get_root_access_server(char *rasaddr, char *rasport, char *msg, struct addrinfo *res_rs, int fd_rs)
{
    char *token = NULL;
    token = strtok(msg, " ");

    token = strtok(NULL, " ");

    token = strtok(NULL, ":");
    if(token == NULL)
    {
        if(flag_d)
        {
            printf("Falha em obter o IP do servidor de acessos...\n");
            printf("A aplicação irá terminar...\n");
        }
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(fd_rs != -1) close(fd_rs);
        if(msg != NULL) free(msg);
        exit(0);
    }
    strcpy(rasaddr, token);

    token = strtok(NULL, "\n");
    if(token == NULL)
    {
        if(flag_d)
        {
            printf("Falha em obter o porto UDP do servidor de acessos...\n");
            printf("A aplicação irá terminar...\n");
        }
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(fd_rs != -1) close(fd_rs);
        if(msg != NULL) free(msg);
        exit(0);
    }
    strcpy(rasport, token);

    free(msg);
}

//Pede e recebe do servidor de acessos um ponto de ligação à stream
int get_access_point(char *rasaddr, char *rasport, struct addrinfo **res_udp, int fd_rs, struct addrinfo *res_rs,
                     char *pop_addr, char *pop_tport, int flag_readesao, char *ipaddr)
{
    int fd_udp = -1;
    int counter = 0;

    if(flag_d) printf("A ligar-se o servidor de acessos, no endereço %s:%s...\n", rasaddr, rasport);
    fd_udp = udp_socket(rasaddr, rasport, res_udp);
    if(fd_udp == -1)
    {
        if(flag_readesao)
        {
            if(flag_d)
            {
                printf("Falha ao ligar-se ao servidor de acesso...\n");
            }
            freeaddrinfo(*res_udp);
            return -1;
        }
        if(flag_d)
        {
            printf("Falha ao ligar-se ao servidor de acesso...\n");
            printf("A aplicação irá terminar...\n");
        }
        if(fd_rs != -1) close(fd_rs);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(*res_udp != NULL) freeaddrinfo(*res_udp);
        exit(0);
    }


    while(popreq(fd_udp, *res_udp, pop_addr, pop_tport) == -1)
    {
        counter++;
        if(counter == MAX_TRIES)
        {
            if(flag_readesao)
            {
                if(flag_d) printf("Impossível comunicar com o servidor de acessos, após %d tentativas...\n", MAX_TRIES);
                close(fd_udp);
                freeaddrinfo(*res_udp);
                return -1;
            }
            if(flag_d)
            {
                printf("\n");
                printf("Impossível comunicar com o servidor de acessos, após %d tentativas...\n", MAX_TRIES);

                printf("A terminar o programa...\n");
            }
            if(fd_rs != -1) close(fd_rs);
            if(res_rs != NULL) freeaddrinfo(res_rs);
            if(fd_udp != -1) close(fd_udp);
            if(*res_udp != NULL) freeaddrinfo(*res_udp);
            exit(0);
        }
    }

    if(!strcmp(pop_addr, ipaddr))
    {
        close(fd_udp);
        freeaddrinfo(*res_udp);
        return -2;
    }

    freeaddrinfo(*res_udp);
    return fd_udp;
}

//Liga-se ao par a montante
int connect_to_peer(char *pop_addr, char *pop_tport, int fd_rs, int fd_udp, struct addrinfo *res_rs, int flag)
{
    int fd_pop = -1;

    if(flag_d) printf("A ligar-se ao par a montante, no endereço %s:%s\n", pop_addr, pop_tport);


    fd_pop = tcp_socket_connect(pop_addr, pop_tport);
    if(fd_pop == -1)
    {

        if(flag_d) printf("Falha ao ligar-se ao par a montante...\n");
        if(flag == 1)
        {
            if(flag_d) printf("A tentar ligar-se de novo...\n");
            return -1;
        }
        if(flag == 0) printf("A aplicação irá terminar...\n");

        if(fd_rs != -1) close(fd_rs);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(fd_udp != -1) close(fd_udp);
        exit(0);
    }



    if(flag_d)
    {
        printf("Ligação com o par a montante estabelecida com sucesso!\n");
    }

    return fd_pop;
}

//Espera pela confirmação de adesão à stream ou por ser redirecionado
int wait_for_confirmation(char *pop_addr, char *pop_tport, int fd_rs, struct addrinfo *res_rs, int fd_udp, int fd_pop, char *streamID)
{
    char *msg = NULL;
    char buffer[BUFFER_SIZE];
    int counter = 0;
    int welcome = 0;

    if(flag_d)
    {
        printf("A tentar comunicar com o par a montante...\n");
    }

    //Recebe port TCP do peer de cima a mensagem WELCOME ---> WE<SP><streamID><LF>
    //Ou então REDIRECT RE<SP><ipaddr>:<tport><LF>
    //Recebe NULL quando há erro. Nesse caso temos de tentar de novo
    msg = receive_confirmation(fd_pop, msg);
    while(msg == NULL)
    {
        counter++;
        if(counter == MAX_TRIES)
        {
            if(flag_d)
            {
                printf("\n");
                printf("Impossível comunicar com o par, após %d tentativas...\n", MAX_TRIES);
                printf("A aplicação irá terminar...\n");
            }
            if(fd_rs != -1) close(fd_rs);
            if(res_rs != NULL) freeaddrinfo(res_rs);
            if(fd_udp != -1) close(fd_udp);
            if(fd_pop != -1) close(fd_pop);
            exit(0);
        }
        if(flag_d)
        {
            printf("A tentar comunicar com o par...\n");
        }
        msg = receive_confirmation(fd_pop, msg);
    }

    counter = 0; //Reset do contador, caso tenha sido possível comunicar
    if(flag_d)
    {
        printf("Mensagem recebida do par a montante: %s\n", msg);
    }

    strncpy(buffer, msg, 2);
    buffer[2] = '\0';
    if(!strcmp(buffer, "WE")) //Recebeu uma mensagem  WELCOME
    {
        sprintf(buffer, "WE %s\n", streamID);
        if(!strcasecmp(msg, buffer)) //Confirma se o streamID está correto
        {
            welcome = 1;
        }
        else
        {
            if(flag_d)
            {
                printf("\n");
                printf("Recebido um Welcome com o streamID errado...\n");
                printf("A aplicação irá terminar...\n");
            }
            if(fd_rs != -1) close(fd_rs);
            if(res_rs != NULL) freeaddrinfo(res_rs);
            if(fd_udp != -1) close(fd_udp);
            if(fd_pop != -1) close(fd_pop);
            free(msg);
            exit(0);
        }
        free(msg);
        msg = NULL;
    }
    else if(!strcmp(buffer, "RE")) //Recebeu uma mensagem REDIRECT
    {
        if(flag_d) printf("Ponto de acesso indisponível...\nA obter novo ponto de acesso...\n");
        if(get_redirect(pop_addr, pop_tport, msg) == -1)
        {
            if(flag_d)
            {
                printf("Falha ao obter o novo ponto de acesso...\n");
                printf("A aplicação irá terminar...\n");
            }
            if(msg != NULL) free(msg);
            if(fd_rs != -1) close(fd_rs);
            if(res_rs != NULL) freeaddrinfo(res_rs);
            if(fd_udp != -1) close(fd_udp);
            if(fd_pop != -1) close(fd_pop);
            exit(0);
        }
        else
        {
            //Vai repetir o ciclo, tentando agora ligar-se ao endereço que veio no REDIRECT
            if(flag_d)
            {
                printf("A ser redirecionado para %s:%s\n", pop_addr, pop_tport);
            }
            free(msg);
            msg = NULL;
            close(fd_pop);
        }
    }

    free(msg);

    return welcome;
}

//Envia uma mensagem a montante a informar do endereço do novo ponto de acesso
void send_new_pop(int fd_pop, char *ipaddr, char *tport, int fd_rs, struct addrinfo *res_rs, int fd_tcp_server, int *fd_array)
{
    if(flag_d) printf("A enviar a montante o endereço e o porto do ponto de acesso disponibilizado...\n");
    if(newpop(fd_pop, ipaddr, tport)  == -1) //retorna -1 em caso de insucesso e 0 em caso de sucesso
    {
        if(flag_d)
        {
            printf("Falha ao enviar a montante o novo ponto de acesso...\n");
            printf("A aplicação irá terminar...\n");
        }
        if(fd_rs != -1) close(fd_rs);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(fd_array != NULL) free(fd_array);
        if(fd_tcp_server != -1) close(fd_tcp_server);
        exit(0);
    }
}

//Faz free e close de tudo o que for necessário
void free_and_close( int is_root, int fd_rs, int fd_udp, int fd_pop, int fd_ss, struct addrinfo *res_rs, int *fd_array)
{
    if(fd_rs != -1) close(fd_rs);
    if(res_rs != NULL) freeaddrinfo(res_rs);
    if(fd_udp != -1) close(fd_udp);
    if(is_root)
    {
        if(fd_ss != -1) close(fd_ss);
    }
    else
    {
        if(fd_pop != -1) close(fd_pop);
    }

    if(fd_array != NULL) free(fd_array);
}


