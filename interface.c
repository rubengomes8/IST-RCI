#include "interface.h"
#include "utils.h"

extern int flag_d;
extern int flag_b;
extern int ascii;

#define MAX_BYTES 10000

//Fazer verificação se a queue não está vazia antes de tentar aceder
//Ver no programa os sítios que fazem exit() e mudar/certificar que a saída do programa é suave, ou seja,
//com libertação de memória e tudo o resto


void interface_root(int fd_ss, int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char * ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied, int fd_udp, int fd_tcp_server, int *fd_array, int bestpops, queue *redirect_queue_head,
        queue *redirect_queue_tail, queue *redirect_aux, int empty_redirect_queue)
{
    int maxfd, counter;
    fd_set fd_read_set;
    int exit_flag = 0;
    int i;
    int n;
    int query_id = 0;

    //Variáveis para lista de apps iamroot ligadas imadiatamente a jusante
    //Vão formar uma lista utilizada para redirects
    /*struct _queue* redirect_queue_head = NULL;
    struct _queue* redirect_queue_tail = NULL;
    struct _queue* redirect_aux = NULL;
    int empty_redirect_queue = 1; //indica que a queue está vazia quando é igual a 1*/


    //Queue de pops. A raiz nunca faz parte da lista
    struct _queue *pops_queue_head = NULL;
    struct _queue *pops_queue_tail = NULL;
    int empty_pops_queue = 1;


    char msg[MAX_BYTES]; msg[0] = '\0';
    char *ptr = NULL;
    char buffer[MAX_BYTES]; buffer[0] = '\0';

    int waiting_pop_reply = 0;
    int received_pops = 0;

    int is_flowing = 1;

    //Dados da stream
    char *data = NULL;
    char buffer_data[BUFFER_SIZE];

    printf("\n\nINTERFACE DE UTILIZADOR\n\n");

    while(1)
    {
        //Preparação dos file descriptors
        FD_ZERO(&fd_read_set);
        FD_SET(fd_udp, &fd_read_set);
        FD_SET(0, &fd_read_set);
        maxfd = fd_udp;
        FD_SET(fd_tcp_server, &fd_read_set);
        maxfd = max(maxfd, fd_tcp_server);
        FD_SET(fd_ss, &fd_read_set);
        maxfd = max(maxfd, fd_ss);
        fd_array_set(fd_array, &fd_read_set, &maxfd, tcp_sessions);

        //Verifica se flag_tree está ativa e se estiver envia TREE_QUERY a todos os filhos
        /*
        if(tree_query)
        {
            //invoke
        }
        */

        //Espera que 1 ou mais file descriptors estejam prontos
        counter = select(maxfd + 1, &fd_read_set, (fd_set *)NULL, (fd_set *)NULL,  (struct timeval *)NULL);
        if(counter <= 0)
        {
            if(flag_d) fprintf(stderr, "Error: select: %s\n", strerror(errno));
            return;
        }

        if(FD_ISSET(fd_ss, &fd_read_set)) //Servidor fonte
        {


            data = buffer_data;
            n = read(fd_ss, data, BUFFER_SIZE - 1);
            //n = tcp_receive(BUFFER_SIZE -1, data, fd_ss);
            if(n == 0)
            {
                //Mandar broken stream em caso de perda de ligação
                if(flag_d) printf("Perdida a ligação ao servidor fonte\n");
                is_flowing = 0;
                for(i=0; i<tcp_sessions; i++)
                {
                    if(fd_array[i] != -1)
                    {
                        n = broken_stream(fd_array[i]);
                        if(n == 0)
                        {
                            //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                            if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                            close(fd_array[i]);
                            fd_array[i] = -1;
                            tcp_occupied--;
                            redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                            if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                        }
                        else if(n == -1)
                        {
                            if (flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                        }
                    }
                }
            }
            else if(n == -1)
            {
                if(flag_d) printf("Erro a receber informação do servidor fonte!\n");
            }
            else
            {
                data[n] = '\0';


                // data[BUFFER_SIZE - 1] = '\0';
                if(flag_b) printf("%s", data);
                fflush(stdout);
            }
        }

        //Lê algo dos pares TCP a jusante
        for(i = 0; i<tcp_sessions; i++)
        {
            if(fd_array[i] != -1)
            {
                if(FD_ISSET(fd_array[i], &fd_read_set))
                {
                    ptr = msg;
                    n = tcp_receive(MAX_BYTES, ptr, fd_array[i]);

                    if(n == 0)
                    {
                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                        if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                        close(fd_array[i]);
                        fd_array[i] = -1;
                        tcp_occupied--;
                        redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                        if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                    }
                    else if(n == -1)
                    {
                        if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                    }
                    else
                    {
                        //Coloca um \0 no fim da mensagem recebida
                        //Se n fosse igual a MAX_BYTES estariamos a apagar o ultimo carater recebido
                        if (n < MAX_BYTES) {
                            *(ptr + n - 1) = '\0';
                        }

                        //Copia os 2 primeiros carateres de msg para buffer
                        strncpy(buffer, msg, 2);
                        buffer[2] = '\0';

                        if(flag_d) printf("Mensagem recebida do para a jusante com índice %d: %s\n", i, ptr);

                        //NEW_POP
                        if(!strcmp("NP", buffer))
                        {
                            redirect_queue_head = receive_newpop(redirect_queue_head, &redirect_queue_tail, i, fd_array,
                                    &empty_redirect_queue, ptr);

                            //Depois de receber o NEW_POP manda um SF
                            if(is_flowing)
                            {
                                n = stream_flowing(fd_array[i]);
                                if(n == 0)
                                {
                                    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                    if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                    close(fd_array[i]);
                                    fd_array[i] = -1;
                                    tcp_occupied--;
                                    redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                    if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                }
                                else if(n == -1)
                                {
                                    if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                }
                                if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: SF\n", i);
                            }
                        }
                        //recebeu um POP_REPLY
                        if(!strcmp("PR", buffer))
                        {

                            pops_queue_head = get_data_pop_reply(pops_queue_head, &pops_queue_tail, ptr, &empty_pops_queue,
                                    query_id, &received_pops, waiting_pop_reply);
                            if(received_pops >= bestpops)
                            {
                                waiting_pop_reply = 0;
                                received_pops = 0;
                            }
                        }
                        //recebeu um TREE_REPLY
                        else if(!strcmp("TR", buffer))
                        {
                            if(flag_d) printf("Mensagem recebida do para a jusante com índice %d: %s\n", i, ptr);
                            //Receber o tree reply

                        }
                    }
                    ptr = NULL;
                    msg[0] = '\0';
                    buffer[0] = '\0';
                }
            }
        }

        //Se chegar uma nova ligação a jusante -> Aceita ou redireciona novas ligações consoante a ocupação
        if(FD_ISSET(fd_tcp_server, &fd_read_set))
        {
            if(flag_d) printf("Novo pedido de conexão...\n");
            if(tcp_sessions > tcp_occupied)
            {
                i = welcome(tcp_sessions, &tcp_occupied, fd_tcp_server, fd_array, streamID);
            }
            else if(tcp_sessions == tcp_occupied)
            {
                //Vai à lista de iamroot imediatamente a jusante para procurar um endereço para onde fazer redirect
                if(redirect_aux == NULL)
                {
                    redirect_aux = redirect_queue_head;
                }
                redirect(fd_tcp_server, getIP(redirect_aux), getPORT(redirect_aux));
                //Para não redirecionar sempre para o mesmo
                //Vai "circulando" e quando chega a NULL e volta a este troço, faz-se redirect_aux = redirect_head_queue
                redirect_aux = getNext(redirect_aux);
            }
        }

        //Servidor de acessos - fizeram um pedido POPREQ
        if(FD_ISSET(fd_udp, &fd_read_set))
        {
            if(tcp_sessions > tcp_occupied)
            {
                //Se tiver sessões disponíveis diz para se ligarem a ele próprio
                popresp(fd_udp, streamID, ipaddr, tport);
                //Não se atualiza aqui o nº de sessões ocupadas. Apenas quando se envia welcome

                if(tcp_occupied > 0 && tcp_sessions == tcp_occupied + 1 && empty_pops_queue && waiting_pop_reply == 0)
                {
                    //faz pop_query
                    query_id++;
                    redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops, redirect_queue_head,
                                                          &redirect_queue_tail);
                    //está à espera da resposta do pop_query
                    waiting_pop_reply = 1;

                    if(redirect_queue_head == NULL)
                    {
                        //Se a a cabeça da lista de elementos diretamente a jusante for NULL, a lista ficou vazia
                        empty_redirect_queue = 1;
                        tcp_occupied = 0;
                        for(i = 0; i<tcp_sessions; i++)
                        {
                            close(fd_array[i]);
                            fd_array[i] = -1;
                        }
                    }
                }
            }
            else
            {
                //Se não estiver à espera de um pop_reply
                if(waiting_pop_reply == 0)
                {
                    //se não tiver nenhum elemento na lista de pops
                    if(empty_pops_queue)
                    {
                        //faz pop_query
                        query_id++; //prinft("query_id: %d", query_id);
                        redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops, redirect_queue_head,
                                                              &redirect_queue_tail);
                        //está à espera da resposta do pop_query
                        waiting_pop_reply = 1;
                        received_pops = 0;

                        if(redirect_queue_head == NULL)
                        {
                            //Se a a cabeça da lista de elementos diretamente a jusante for NULL, a lista ficou vazia
                            empty_redirect_queue = 1;
                            tcp_occupied = 0;
                            for(i = 0; i<tcp_sessions; i++)
                            {
                                close(fd_array[i]);
                                fd_array[i] = -1;
                            }
                        }
                    }
                    else
                    {
                        //Caso ainda haja elementos na lista de pops, indicar o endereço do primeiro
                        popresp(fd_udp, streamID, getIP(pops_queue_head), getPORT(pops_queue_head));
                        decreaseAvailableSessions(pops_queue_head);
                        if(getAvailableSessions(pops_queue_head) == 0)
                        {
                            //Se o pop que foi indicado deixar de ter ligações, removê-lo da lista
                            pops_queue_head = removeElementByAddress(pops_queue_head, &pops_queue_tail, getIP(pops_queue_head),
                                                                     getPORT(pops_queue_head));

                            if(pops_queue_head == NULL)
                            {
                                //Se a lista de pops ficar vazia, fazer pop_query
                                empty_pops_queue = 1;
                                query_id++;
                                redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops,
                                        redirect_queue_head, &redirect_queue_tail);
                                waiting_pop_reply = 1;
                                received_pops = 0;

                                if(redirect_queue_head == NULL)
                                {
                                    //Se a a cabeça da lista de elementos diretamente a jusante for NULL, a lista ficou vazia
                                    empty_redirect_queue = 1;
                                    tcp_occupied = 0;
                                    for(i = 0; i<tcp_sessions; i++)
                                    {
                                        close(fd_array[i]);
                                        fd_array[i] = -1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        //O utilizador introduziu um comando de utilizador
        if(FD_ISSET(0, &fd_read_set))
        {
            exit_flag = read_terminal(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied,
                    redirect_queue_head, NULL, NULL, is_flowing);
            if(exit_flag == 1)
            {
                freeQueue(redirect_queue_head);
                freeQueue(pops_queue_head);
                return;
            }
        }
    }
}

void interface_not_root(int fd_rs, struct addrinfo *res_rs, char* streamID, char *streamIP, char *streamPORT, int *is_root, char* ipaddr, char *uport,
        char *tport, int tcp_sessions, int tcp_occupied, int fd_tcp_server, int *fd_array, int bestpops,
        int fd_pop, char *pop_addr, char *pop_tport, char *rsaddr, char *rsport)
{
    int maxfd, counter;
    fd_set fd_read_set;
    int exit_flag = 0;
    int i;
    int n;

    int query_id = -1;
    int query_id_rcvd;
    int requested_pops;
    int sent_pops = 0;

    struct _queue* redirect_queue_head = NULL;
    struct _queue* redirect_queue_tail = NULL;
    struct _queue* redirect_aux = NULL;
    int empty_redirect_queue = 1; //indica que a queue está vazia quando é igual a 1
   
    char buffer[MAX_BYTES]; buffer[0] = '\0';
    char *ptr = NULL;
    char msg[MAX_BYTES]; msg[0] = '\0';

    //Variáveis que vieram do pop reply
    char popreply_ip[IP_LEN +1];
    char popreply_port[PORT_LEN +1];
    int popreply_avails;
    int query_id_rcvd_aux = 0;

    int is_flowing = 0;

    //Variáveis que vieram do tree query
    char treequery_ip[IP_LEN +1];
    char treequery_port[PORT_LEN +1];
    int validate_treequery = -1;

    //Variáveis para readesão
    struct addrinfo *res_udp = NULL;
    int fd_udp = -1;
    int welcome_flag = 0;
    char rasaddr[IP_LEN + 1];
    char rasport[PORT_LEN + 1];
    int fd_ss = -1;
    char *msg_readesao = NULL;
    char buffer_readesao[BUFFER_SIZE];


    printf("\n\nINTERFACE DE UTILIZADOR\n\n");

    while(1)
    {
        FD_ZERO(&fd_read_set);
        FD_SET(0, &fd_read_set);
        maxfd = 0;
        FD_SET(fd_tcp_server, &fd_read_set);
        maxfd = max(maxfd, fd_tcp_server);
        FD_SET(fd_pop, &fd_read_set);
        maxfd = max(maxfd, fd_pop);
        fd_array_set(fd_array, &fd_read_set, &maxfd, tcp_sessions);

        //Espera por uma mensagem
        counter = select(maxfd + 1, &fd_read_set, (fd_set *)NULL, (fd_set *)NULL, (struct timeval*)NULL);
        if(counter <= 0)
        {
            if(flag_d) fprintf(stderr, "Error: select: %s\n", strerror(errno));
            return;
        }

        //Aceita ou redireciona novas ligações
        if(FD_ISSET(fd_tcp_server, &fd_read_set))
        {
            if(flag_d) printf("Novo pedido de conexão...\n");
            if(tcp_sessions > tcp_occupied)
            {
                i = welcome(tcp_sessions, &tcp_occupied, fd_tcp_server, fd_array, streamID);
            }
            else if(tcp_sessions == tcp_occupied)
            {
                if(redirect_aux == NULL)
                {
                    redirect_aux = redirect_queue_head;
                }
                redirect(fd_tcp_server, getIP(redirect_aux), getPORT(redirect_aux));
                redirect_aux = getNext(redirect_aux);
            }
        }


        //Pares TCP a jusante
        for(i = 0; i<tcp_sessions; i++)
        {
            if(fd_array[i] != -1)
            {
                if(FD_ISSET(fd_array[i], &fd_read_set))
                {
                    ptr = msg;
                    n = tcp_receive(MAX_BYTES, ptr, fd_array[i]);

                    if(n == 0)
                    {
                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                        if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                        close(fd_array[i]);
                        fd_array[i] = -1;
                        tcp_occupied--;
                        redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                        if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                    }
                    else if(n == -1)
                    {
                        if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                    }
                    else
                    {
                        //Coloca um \0 no fim da mensagem recebida
                        //Se n fosse igual a MAX_BYTES estariamos a apagar o ultimo carater recebido
                        if (n < MAX_BYTES) {
                            *(ptr + n - 1) = '\0';
                        }

                        //Copia os 2 primeiros carateres de msg para buffer
                        strncpy(buffer, msg, 2);
                        buffer[2] = '\0';

                        if(flag_d) printf("Mensagem recebida do para a jusante com índice %d: %s\n", i, ptr);

                        //NEW_POP
                        if(!strcmp("NP", buffer))
                        {
                            redirect_queue_head = receive_newpop(redirect_queue_head, &redirect_queue_tail, i, fd_array,
                                                                 &empty_redirect_queue, ptr);

                            //Depois de receber o NEW_POP manda um SF
                            if(is_flowing)
                            {
                                n = stream_flowing(fd_array[i]);
                                if(n == 0)
                                {
                                    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                    if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                    close(fd_array[i]);
                                    fd_array[i] = -1;
                                    tcp_occupied--;
                                    redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                    if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                }
                                else if(n == -1)
                                {
                                    if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                }
                                if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: SF\n", i);
                            }
                        }
                        //POP_REPLY
                        else if(!strcmp("PR", buffer))
                        {
                            query_id_rcvd_aux = receive_pop_reply(ptr, popreply_ip, popreply_port, &popreply_avails);
                            if(requested_pops - sent_pops > 0) //Se tiver pops por enviar
                            {
                                query_id_rcvd = query_id_rcvd_aux;
                                //Receber os dados do pop reply vindos de jusante

                                //Verifica o queryID recebido na reply é o mesmo que o da última query
                                if(query_id == query_id_rcvd)
                                {
                                    n = send_pop_reply(query_id, popreply_avails, popreply_ip, popreply_port, fd_pop);

                                    if(n == 0)
                                    {
                                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                        if(flag_d) printf("Perdida a ligação ao par a montante...\n");
                                        close(fd_pop);
                                        //Fazer nova adesão
                                    }
                                    else if(n == -1)
                                    {
                                        if(flag_d) printf("Falha ao comunicar com o peer a montante...\n");
                                    }
                                    else
                                    {
                                        //se correu bem atualiza o nº de pops enviados
                                        sent_pops += popreply_avails;
                                    }
                                }
                            }
                            else
                            {
                                if(flag_d) printf("Pop reply descartada, pois já tinham sido recebidos todos os pops pedidos!\n");
                            }
                        }
                            //TREE_REPLY
                        else if(!strcmp("TR", buffer))
                        {
                            if(flag_d) printf("Mensagem recebida do para a jusante com índice %d: %s\n", i, ptr);
                            //Receber TR
                            n = receive_tree_reply_and_propagate(ptr, fd_pop, fd_array[i]);
                            if(n == 10)
                            {
                                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                close(fd_array[i]);
                                fd_array[i] = -1;
                                tcp_occupied--;
                                redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                            }
                        }
                    }
                    ptr = NULL;
                    msg[0] = '\0';
                    buffer[0] = '\0';
                }
            }
        }


        //Par TCP a montante
        if(FD_ISSET(fd_pop, &fd_read_set))
        {
            ptr = msg;
            n = tcp_receive(MAX_BYTES, ptr, fd_pop);

            if(n == 0)
            {
                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                if(flag_d) printf("Perdida a ligação ao par a montante...\n");
                close(fd_pop);

                // Informar da interrupção da stream
                is_flowing = 0;
                for(i = 0; i<tcp_sessions; i++)
                {
                    if(fd_array[i] != -1)
                    {
                        n = broken_stream(fd_array[i]);

                        if(n == 0)
                        {
                            //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                            if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                            close(fd_array[i]);
                            fd_array[i] = -1;
                            tcp_occupied--;
                            redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                            if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                        }
                        else if(n == -1)
                        {
                            if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                        }
                        if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: BS\n", i);
                    }
                }



                /////////////////////////////// Aderir novamente à stream //////////////////////////////////
                msg_readesao = find_whoisroot(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport);

                if(!strcmp(msg_readesao, "ERROR"))
                {
                    freeQueue(redirect_queue_head);
                    return;
                }
                else
                {
                    strncpy(buffer_readesao, msg_readesao, 6);
                    buffer_readesao[6] = '\0';

                    if(!strcmp(buffer_readesao, "URROOT"))
                    {
                        if(msg_readesao != NULL) free(msg_readesao);
                        //A aplicação fica registada como sendo a raiz da nova árvore de escoamento
                        *is_root = 1;

                        //////////////////// 1. Estabelecer sessão TCP com o servidor fonte /////////////////////////////////////
                        fd_ss = source_server_connect(fd_rs, res_rs, streamIP, streamPORT);

                        ////////////////////// 3. instalar o servidor UDP de acesso de raiz /////////////////////////////////////
                        fd_udp = install_access_server(ipaddr, fd_rs, fd_ss, res_rs, &res_udp, uport, fd_array);


                        //Informar do restabelecimento da stream
                        for(i = 0; i<tcp_sessions; i++)
                        {
                            if(fd_array[i] != -1)
                            {
                                n = stream_flowing(fd_array[i]);

                                if(n == 0)
                                {
                                    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                    if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                    close(fd_array[i]);
                                    fd_array[i] = -1;
                                    tcp_occupied--;
                                    redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                    if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                }
                                else if(n == -1)
                                {
                                    if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                }
                                if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: SF\n", i);
                            }
                        }


                        //////////////////////////// 4. executar a interface de utilizador //////////////////////////////////////
                        interface_root(fd_ss, fd_rs, res_rs, streamID, *is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied,
                                       fd_udp, fd_tcp_server, fd_array, bestpops, redirect_queue_head, redirect_queue_tail,
                                       redirect_aux, empty_redirect_queue);
                        return; //Se ele sair da inferface_root é porque o programa foi terminado

                    }
                    else if(!strcmp(buffer_readesao, "ROOTIS"))
                    {
                        //Obtém o IP e o porto do servidor de acesso
                        get_root_access_server(rasaddr, rasport, msg_readesao, res_rs, fd_rs);

                        ///////////// 1. Solicita ao servidor de acesso da raíz o IP e porto TCP do ponto de acesso ////
                        fd_udp = get_access_point(rasaddr, rasport, &res_udp, fd_rs, res_rs, pop_addr, pop_tport);

                        while(welcome_flag == 0) //Enquanto não tiver recebido um WELCOME com a stream esperada
                        {
                            //////////////////////// 2. Estabelece sessão TCP com o ponto de acesso ///////////////////////////
                            fd_pop = connect_to_peer(pop_addr, pop_tport, fd_rs, fd_udp, res_rs);

                            //////////////////////////// 3. Aguarda confirmação de adesão /////////////////////////////////////
                            welcome_flag = wait_for_confirmation(pop_addr, pop_tport, fd_rs, res_rs, fd_udp, fd_pop, streamID);
                        }
                        welcome_flag = 0;
                        close(fd_udp);
                        fd_udp = -1;


                        //Informar do restabelecimento da stream
                        for(i = 0; i<tcp_sessions; i++)
                        {
                            if(fd_array[i] != -1)
                            {
                                n = stream_flowing(fd_array[i]);

                                if(n == 0)
                                {
                                    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                    if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                    close(fd_array[i]);
                                    fd_array[i] = -1;
                                    tcp_occupied--;
                                    redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                    if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                }
                                else if(n == -1)
                                {
                                    if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                }
                                if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: SF\n", i);
                            }
                        }
                    }
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////////////

            }
            else if(n == -1)
            {
                if(flag_d) printf("Falha ao comunicar com o peer a montante...\n");
            }
            else
            {
                //Coloca um \0 no fim da mensagem recebida
                //Se n fosse igual a MAX_BYTES estariamos a apagar o ultimo carater recebido
                if(n < MAX_BYTES)
                {
                    *(ptr + n - 1) = '\0';
                }

                //Copia os 2 primeiros caracteres de msg para buffer
                strncpy(buffer, msg, 2);
                buffer[2] = '\0';

                if(!strcmp("BS", buffer))
                {
                    if(flag_d) printf("Mensagem recebida do par a montante: BS\n");
                    //Informar da interrupção da stream
                    is_flowing = 0;
                    for(i = 0; i<tcp_sessions; i++)
                    {
                        if(fd_array[i] != -1)
                        {
                            n = broken_stream(fd_array[i]);

                            if(n == 0)
                            {
                                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                close(fd_array[i]);
                                fd_array[i] = -1;
                                tcp_occupied--;
                                redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                            }
                            else if(n == -1)
                            {
                                if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                            }
                            if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: BS\n", i);
                        }
                    }
                }
                else if(!strcmp("SF", buffer))
                {
                    if(flag_d) printf("Mensagem recebida do par a montante: SF\n");
                    is_flowing = 1;
                    //Informar do restabelecimento da stream
                    for(i = 0; i<tcp_sessions; i++)
                    {
                        if(fd_array[i] != -1)
                        {
                            n = stream_flowing(fd_array[i]);

                            if(n == 0)
                            {
                                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                close(fd_array[i]);
                                fd_array[i] = -1;
                                tcp_occupied--;
                                redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                            }
                            else if(n == -1)
                            {
                                if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                            }
                            if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: SF\n", i);
                        }
                    }
                }
                else if(!strcmp("PQ", buffer))
                {
                    if(flag_d) printf("Mensagem recebida do par a montante: %s\n", ptr);
                    //Retornar os pontos ip:port e o número de pontos de acessos disponíveis aqui
                    //Se não tiver suficientes fazer pop_query ele próprio
                    n = receive_pop_query(ptr, &requested_pops, &query_id); //Verifica quantos pops lhe foram pedidos e qual o queryID associado

                    if(n != -1)
                    {
                        sent_pops = 0;
                        if(tcp_sessions - tcp_occupied >= requested_pops) //Tem espaço para todas
                        {
                            n = send_pop_reply(query_id, tcp_sessions - tcp_occupied, ipaddr, tport, fd_pop);
                            if(n == 0)
                            {
                                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                if(flag_d) printf("Perdida a ligação ao par a montante...\n");
                                close(fd_pop);
                                //Fazer nova adesão

                                is_flowing = 0;
                                for(i = 0; i<tcp_sessions; i++)
                                {
                                    if(fd_array[i] != -1)
                                    {
                                        n = broken_stream(fd_array[i]);

                                        if(n == 0)
                                        {
                                            //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                            if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                            close(fd_array[i]);
                                            fd_array[i] = -1;
                                            tcp_occupied--;
                                            redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                            if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                        }
                                        else if(n == -1)
                                        {
                                            if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                        }
                                        if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: BS\n", i);
                                    }
                                }


                                /////////////////////////////// Aderir novamente à stream //////////////////////////////////
                                msg_readesao = find_whoisroot(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport);

                                if(!strcmp(msg_readesao, "ERROR"))
                                {
                                    freeQueue(redirect_queue_head);
                                    return;
                                }
                                else
                                {
                                    strncpy(buffer_readesao, msg_readesao, 6);
                                    buffer_readesao[6] = '\0';

                                    if(!strcmp(buffer_readesao, "URROOT"))
                                    {
                                        if(msg_readesao != NULL) free(msg_readesao);
                                        //A aplicação fica registada como sendo a raiz da nova árvore de escoamento
                                        *is_root = 1;

                                        //////////////////// 1. Estabelecer sessão TCP com o servidor fonte /////////////////////////////////////
                                        fd_ss = source_server_connect(fd_rs, res_rs, streamIP, streamPORT);

                                        ////////////////////// 3. instalar o servidor UDP de acesso de raiz /////////////////////////////////////
                                        fd_udp = install_access_server(ipaddr, fd_rs, fd_ss, res_rs, &res_udp, uport, fd_array);


                                        //Informar do restabelecimento da stream
                                        is_flowing = 1;
                                        for(i = 0; i<tcp_sessions; i++)
                                        {
                                            if(fd_array[i] != -1)
                                            {
                                                n = stream_flowing(fd_array[i]);

                                                if(n == 0)
                                                {
                                                    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                                    if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                                    close(fd_array[i]);
                                                    fd_array[i] = -1;
                                                    tcp_occupied--;
                                                    redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                                    if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                                }
                                                else if(n == -1)
                                                {
                                                    if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                                }
                                                if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: SF\n", i);
                                            }
                                        }


                                        //////////////////////////// 4. executar a interface de utilizador //////////////////////////////////////
                                        interface_root(fd_ss, fd_rs, res_rs, streamID, *is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied,
                                                       fd_udp, fd_tcp_server, fd_array, bestpops, redirect_queue_head, redirect_queue_tail,
                                                       redirect_aux, empty_redirect_queue);
                                        return; //Se ele sair da inferface_root é porque o programa foi terminado

                                    }
                                    else if(!strcmp(buffer_readesao, "ROOTIS"))
                                    {
                                        //Obtém o IP e o porto do servidor de acesso
                                        get_root_access_server(rasaddr, rasport, msg_readesao, res_rs, fd_rs);

                                        ///////////// 1. Solicita ao servidor de acesso da raíz o IP e porto TCP do ponto de acesso ////
                                        fd_udp = get_access_point(rasaddr, rasport, &res_udp, fd_rs, res_rs, pop_addr, pop_tport);

                                        while(welcome_flag == 0) //Enquanto não tiver recebido um WELCOME com a stream esperada
                                        {
                                            //////////////////////// 2. Estabelece sessão TCP com o ponto de acesso ///////////////////////////
                                            fd_pop = connect_to_peer(pop_addr, pop_tport, fd_rs, fd_udp, res_rs);

                                            //////////////////////////// 3. Aguarda confirmação de adesão /////////////////////////////////////
                                            welcome_flag = wait_for_confirmation(pop_addr, pop_tport, fd_rs, res_rs, fd_udp, fd_pop, streamID);
                                        }
                                        welcome_flag = 0;
                                        close(fd_udp);
                                        fd_udp = -1;

                                        //Informar do restabelecimento da stream
                                        is_flowing = 1;
                                        for(i = 0; i<tcp_sessions; i++)
                                        {
                                            if(fd_array[i] != -1)
                                            {
                                                n = stream_flowing(fd_array[i]);

                                                if(n == 0)
                                                {
                                                    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                                    if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                                    close(fd_array[i]);
                                                    fd_array[i] = -1;
                                                    tcp_occupied--;
                                                    redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                                    if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                                }
                                                else if(n == -1)
                                                {
                                                    if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                                }
                                                if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: SF\n", i);
                                            }
                                        }
                                    }
                                }
                                ////////////////////////////////////////////////////////////////////////////////////////////////////////



                            }
                            else if(n == -1)
                            {
                                if(flag_d) printf("Falha ao comunicar com o peer a montante...\n");
                            }
                            else
                            {
                                sent_pops += tcp_sessions - tcp_occupied; //Atualiza se a comunicação correu bem
                            }
                        }
                        else if(tcp_sessions - tcp_occupied > 0)
                        {
                            n = send_pop_reply(query_id, tcp_sessions - tcp_occupied, ipaddr, tport, fd_pop);

                            if(n == 0)
                            {
                                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                if(flag_d) printf("Perdida a ligação ao par a montante...\n");
                                close(fd_pop);

                                is_flowing = 0;
                                for(i = 0; i<tcp_sessions; i++)
                                {
                                    if(fd_array[i] != -1)
                                    {
                                        n = broken_stream(fd_array[i]);

                                        if(n == 0)
                                        {
                                            //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                            if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                            close(fd_array[i]);
                                            fd_array[i] = -1;
                                            tcp_occupied--;
                                            redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                            if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                        }
                                        else if(n == -1)
                                        {
                                            if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                        }
                                        if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: BS\n", i);
                                    }
                                }


                                /////////////////////////////// Aderir novamente à stream //////////////////////////////////
                                msg_readesao = find_whoisroot(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport);

                                if(!strcmp(msg_readesao, "ERROR"))
                                {
                                    freeQueue(redirect_queue_head);
                                    return;
                                }
                                else
                                {
                                    strncpy(buffer_readesao, msg_readesao, 6);
                                    buffer_readesao[6] = '\0';

                                    if(!strcmp(buffer_readesao, "URROOT"))
                                    {
                                        if(msg_readesao != NULL) free(msg_readesao);
                                        //A aplicação fica registada como sendo a raiz da nova árvore de escoamento
                                        *is_root = 1;

                                        //////////////////// 1. Estabelecer sessão TCP com o servidor fonte /////////////////////////////////////
                                        fd_ss = source_server_connect(fd_rs, res_rs, streamIP, streamPORT);

                                        ////////////////////// 3. instalar o servidor UDP de acesso de raiz /////////////////////////////////////
                                        fd_udp = install_access_server(ipaddr, fd_rs, fd_ss, res_rs, &res_udp, uport, fd_array);


                                        //Informar do restabelecimento da stream
                                        is_flowing = 1;
                                        for(i = 0; i<tcp_sessions; i++)
                                        {
                                            if(fd_array[i] != -1)
                                            {
                                                n = stream_flowing(fd_array[i]);

                                                if(n == 0)
                                                {
                                                    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                                    if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                                    close(fd_array[i]);
                                                    fd_array[i] = -1;
                                                    tcp_occupied--;
                                                    redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                                    if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                                }
                                                else if(n == -1)
                                                {
                                                    if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                                }
                                                if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: SF\n", i);
                                            }
                                        }



                                        //////////////////////////// 4. executar a interface de utilizador //////////////////////////////////////
                                        interface_root(fd_ss, fd_rs, res_rs, streamID, *is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied,
                                                       fd_udp, fd_tcp_server, fd_array, bestpops, redirect_queue_head, redirect_queue_tail,
                                                       redirect_aux, empty_redirect_queue);
                                        return; //Se ele sair da inferface_root é porque o programa foi terminado

                                    }
                                    else if(!strcmp(buffer_readesao, "ROOTIS"))
                                    {
                                        //Obtém o IP e o porto do servidor de acesso
                                        get_root_access_server(rasaddr, rasport, msg_readesao, res_rs, fd_rs);

                                        ///////////// 1. Solicita ao servidor de acesso da raíz o IP e porto TCP do ponto de acesso ////
                                        fd_udp = get_access_point(rasaddr, rasport, &res_udp, fd_rs, res_rs, pop_addr, pop_tport);

                                        while(welcome_flag == 0) //Enquanto não tiver recebido um WELCOME com a stream esperada
                                        {
                                            //////////////////////// 2. Estabelece sessão TCP com o ponto de acesso ///////////////////////////
                                            fd_pop = connect_to_peer(pop_addr, pop_tport, fd_rs, fd_udp, res_rs);

                                            //////////////////////////// 3. Aguarda confirmação de adesão /////////////////////////////////////
                                            welcome_flag = wait_for_confirmation(pop_addr, pop_tport, fd_rs, res_rs, fd_udp, fd_pop, streamID);
                                        }
                                        welcome_flag = 0;
                                        close(fd_udp);
                                        fd_udp = -1;

                                        //Informar do restabelecimento da stream
                                        is_flowing = 1;
                                        for(i = 0; i<tcp_sessions; i++)
                                        {
                                            if(fd_array[i] != -1)
                                            {
                                                n = stream_flowing(fd_array[i]);

                                                if(n == 0)
                                                {
                                                    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                                    if(flag_d) printf("Perdida a ligação ao par a jusante com índice %d...\n", i);
                                                    close(fd_array[i]);
                                                    fd_array[i] = -1;
                                                    tcp_occupied--;
                                                    redirect_queue_head = removeElementByIndex(redirect_queue_head, &redirect_queue_tail, i);
                                                    if(redirect_queue_head == NULL) empty_redirect_queue = 1;
                                                }
                                                else if(n == -1)
                                                {
                                                    if(flag_d) printf("Falha ao comunicar com o peer a jusante com índice %d...\n", i);
                                                }
                                                if(flag_d) printf("Mensagem enviada ao para a jusante com índice %d: SF\n", i);
                                            }
                                        }
                                    }
                                }
                                ////////////////////////////////////////////////////////////////////////////////////////////////////////



                            }
                            else if(n == -1)
                            {
                                if(flag_d) printf("Falha ao comunicar com o peer a montante...\n");
                            }
                            else
                            {
                                sent_pops += tcp_sessions - tcp_occupied; //Atualiza se a comunicação correu bem

                                //faz pop query a pedir o nº de pops que falta
                                redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, requested_pops - sent_pops,
                                                                      redirect_queue_head, &redirect_queue_tail);
                            }
                        }
                        else
                        {
                            redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, requested_pops,
                                                                  redirect_queue_head, &redirect_queue_tail);

                            if(redirect_queue_head == NULL)
                            {
                                //Se a a cabeça da lista de elementos diretamente a jusante for NULL, a lista ficou vazia
                                empty_redirect_queue = 1;
                                tcp_occupied = 0;
                                for(i = 0; i<tcp_sessions; i++)
                                {
                                    close(fd_array[i]);
                                    fd_array[i] = -1;
                                }
                            }
                        }
                    }

                }
                else if(!strcmp("TQ", buffer))               
                {                   
                    if(flag_d) printf("Mensagem recebida do par a montante: %s\n", ptr);
                    receive_tree_query(ptr, treequery_ip, treequery_port);
                    validate_treequery = compare_ip_and_port(treequery_ip, treequery_port, ipaddr, tport);

                    if(validate_treequery == 0) //O ip e o porto de destino forem o desta iamroot
                    {
                        //Responder com um TREE_REPLY com capacidade e ocupação do ponto de acesso
                        send_tree_reply(treequery_ip, treequery_port, tcp_sessions, tcp_occupied, redirect_queue_head,redirect_queue_tail, fd_pop);

                    }
                    else //Caso contrário deverá replicar a mensagem a jusante a menos que não tenha pares a jusante
                    {
                        //Percorrer a lista redirect e enviar para cada um deles a mesma mensagem -> send_tree_query
                        redirect_aux = redirect_queue_head;
                        while(redirect_aux != NULL)
                        {
                            send_tree_query(getIP(redirect_aux), getPORT(redirect_aux), fd_tcp_server);
                            
                            redirect_aux = getNext(redirect_aux);
                        }
                    }
                }
            }
            ptr = NULL;
            msg[0] = '\0';
            buffer[0] = '\0';
        }

        if(FD_ISSET(0, &fd_read_set))
        {
            exit_flag = read_terminal(fd_rs, res_rs, streamID, *is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied,
                    redirect_queue_head, pop_addr, pop_tport, is_flowing);
            if(exit_flag == 1)
            {
                freeQueue(redirect_queue_head);
                return;
            }
        }
    }
}

int read_terminal(int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char *ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied, queue *redirect_queue_head, char *pop_addr, char *pop_tport, int is_flowing)
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
        if(is_flowing)
        {
            printf("A stream está a correr\n");
        }
        else
        {
            printf("Stream interrompida\n");
        }

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
            printf("Ligação a montante ao par com o endereço: %s:%s\n", pop_addr, pop_tport);
        }

        //Endereço IP e porto TCP do ponto de acesso disponibilizado
        printf("Ponto de ligação disponibilizado: %s:%s\n", ipaddr, tport);


        //Número de sessões TCP suportadas a jusante e indicação de quantas se encontram ocupadas
        printf("%d sessões TCP suportadas a jusante, das quais %d se encontram ocupadas!\n", tcp_sessions, tcp_occupied);

        //Endereço IP e porto TCP dos pontos de acesso dos pares imediatamente a jusante
        if(redirect_queue_head != NULL)
            printf("Endereços IP e portos TCP dos pontos de acesso dos pares imediatamente a jusante:\n");

        while(redirect_queue_head != NULL)
        {
            printf("%s:%s\n", getIP(redirect_queue_head), getPORT(redirect_queue_head));
            redirect_queue_head = getNext(redirect_queue_head);
        }
    }
    else if(!strcasecmp(buffer, "display on\n"))
    {
        printf("Display on\n");
        flag_b = 1;
    }
    else if(!strcasecmp(buffer, "display off\n"))
    {
        printf("Display off\n");
        flag_b = 0;
    }
    else if (!strcasecmp(buffer, "format ascii\n"))
    {
        printf("Alteração para o formato ascii efetuada\n");
        ascii = 1;
    }
    else if(!strcasecmp(buffer, "format hex\n"))
    {
        printf("Alteração para o formato hexadecimal efetuada\n");
        ascii = 0;
    }
    else if(!strcasecmp(buffer, "debug on\n"))
    {
        flag_d = 1;
        printf("Debug on\n");
    }
    else if(!strcasecmp(buffer, "debug off\n"))
    {
        printf("Debug off\n");
        flag_d = 0;
    }
    else if(!strcasecmp(buffer, "tree\n") && is_root)
    {
        //Apresentar estrutura da transmissão
        printf("Estrutura de transmissão em árvore\n");
        //meter flag global a 1 e na interface_root se tiver flag ativa chamar função q envia tree query aos filhos
    }
    else if(!strcasecmp(buffer, "exit\n"))
    {
        printf("A aplicação irá ser terminada...\n");
        return 1;
    }
    else
    {
        printf("Comando inválido!\n");
    }

    return 0;
}

queue *receive_newpop(queue *redirect_queue_head, queue **redirect_queue_tail, int i, int *fd_array, int *empty_queue, char *msg)
{
    char *token = NULL;
    char ip_aux[IP_LEN + 1]; ip_aux[0] = '\0';
    char port_aux[PORT_LEN + 1]; port_aux[0] = '\0';

    token = strtok(msg, " ");
    token = strtok(NULL, ":");

    if (token == NULL)
    {
        if (flag_d) printf("Endereço IP inválido\n");
        return redirect_queue_head;
    }
    strcpy(ip_aux, token);

    token = strtok(NULL, "\n");
    if (token == NULL)
    {
        if (flag_d) printf("Porto inválido\n");
        return redirect_queue_head;
    }
    strcpy(port_aux, token);

    if (*empty_queue == 0)
    {
        //Se a fila não estiver vazia insere na cauda
        *redirect_queue_tail = insertTail(ip_aux, port_aux, 1, i, *redirect_queue_tail);
    }
    else
    {
        //Se a lista estiver vazia cria uma nova
        redirect_queue_head = newElement(ip_aux, port_aux, 1, i);
        *redirect_queue_tail = redirect_queue_head;
        *empty_queue = 0;
    }

    return redirect_queue_head;
}

//Faz pop query a todos os elementos imediatamente a jusante. Retorna a head da lista desses elementos
queue *pop_query_peers(int tcp_sessions, int *fd_array, int query_id, int bestpops, queue *redirect_queue_head,
        queue **redirect_queue_tail)
{
    int i;
    int n;

    for(i = 0; i<tcp_sessions; i++)
    {
        if(fd_array[i] != -1)
        {
            n = pop_query(query_id, bestpops, fd_array[i]);

            //Ligação perdida
            //Fecha o fd, coloca -1 no vetor e fd a jusante e remove a entrada da fila de pares a jusante
            if(n == -1)
            {
                if(flag_d) printf("Falha na comunicação com um dos peers...\n");
            }
            else if(n == 0)
            {
                if(flag_d) printf("Ligação a um dos peers perdida...\n");
                close(fd_array[i]);
                fd_array[i] = -1;
                redirect_queue_head = removeElementByIndex(redirect_queue_head, redirect_queue_tail, i);
            }
            else if(n == 1)
            {
                if(flag_d) printf("Mensagem enviada ao par com índice %d: PQ %X %d\n", i, query_id, bestpops);
            }
        }
        else if(fd_array[i] == -1)
        {
            //Em princípio nunca vai chegar aqui
            if(flag_d) printf("Erro, pares todos ocupados mas um deles não tem fd\n");
        }
    }

    return redirect_queue_head;
}

queue *get_data_pop_reply(queue *pops_queue_head, queue **pops_queue_tail, char *ptr, int *empty_pops_queue, int query_id,
        int *received_pops, int waiting_pop_reply)
{
    char ip[IP_LEN + 1];
    char port[PORT_LEN + 1];
    int available_sessions;
    int query_id_rcvd;

    query_id_rcvd = receive_pop_reply(ptr, ip, port, &available_sessions);

    if(waiting_pop_reply)
    {
        *received_pops += available_sessions;

        //Se o query id recebido na mensagem do POP REPLY fôr igual ao queryid atual
        if(query_id_rcvd == query_id)
        {

            if(*empty_pops_queue)
            {
                pops_queue_head = newElement(ip, port, available_sessions, -1);
                *pops_queue_tail = pops_queue_head;
                *empty_pops_queue = 0;
            }
            else
            {
                *pops_queue_tail = insertTail(ip, port, available_sessions, -1, *pops_queue_tail);
            }
        }
    }
    else
    {
        if(flag_d) printf("Pop reply descartada, pois já foram recebidas todas as pedidas!\n");
    }


    return pops_queue_head;   
}





