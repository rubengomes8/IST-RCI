#include "interface.h"
#include "utils.h"

extern int flag_d;
extern int flag_b;
extern int flag_tree;
extern int ascii;

#define MAX_BYTES 10000

//Fazer verificação se a queue não está vazia antes de tentar aceder
//Ver no programa os sítios que fazem exit() e mudar/certificar que a saída do programa é suave, ou seja,
//com libertação de memória e tudo o resto


void interface_root(int fd_ss, int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char * ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied, int fd_udp, int fd_tcp_server, int *fd_array, int bestpops, queue *redirect_queue_head,
        queue *redirect_queue_tail, queue *redirect_aux, int empty_redirect_queue, int tsecs, char *rsaddr, char *rsport)
{
    int maxfd, counter;
    fd_set fd_read_set;
    int exit_flag = 0;
    int i;
    int n;
    int query_id = 0;


    //Queue de pops. A raiz nunca faz parte da lista
    struct _queue *pops_queue_head = NULL;
    struct _queue *pops_queue_tail = NULL;
    int empty_pops_queue = 1;

    struct _queue *aux_pops_queue_head = NULL;
    struct _queue *aux_pops_queue_tail = NULL;
    struct _queue *aux_pops_queue_aux = NULL;
    int empty_aux_pops_queue = 1;
    //int counter_aux_pops = 0;

    ///////// buffers intermédios ////////

    char **aux_buffer_sons = NULL;
    char **aux_ptr_sons = NULL;
    int *nread_sons = NULL;
    int j;

    n = buffer_interm_sons(aux_ptr_sons, aux_buffer_sons, nread_sons, tcp_sessions);
    if(n == -1) return;

    //////////////////////////////////////

    char msg[MAX_BYTES]; msg[0] = '\0';
    char *ptr = NULL;
    char buffer[MAX_BYTES]; buffer[0] = '\0';

    int waiting_pop_reply = 0;
    int received_pops = 0;

    int is_flowing = 1;

    //Dados da stream
    char *data = NULL;
    char buffer_data[BUFFER_SIZE];
    int data_len;

    long reference_time_whoisroot = time(NULL);
    long reference_time_pop_query = time(NULL);
    long now = time(NULL);
    char *msg_whoisroot = NULL;
    char buffer_whoisroot[BUFFER_SIZE];

    //ler dos pares tcp a jusante
    queue *aux = NULL;

    printf("\n\nINTERFACE DE UTILIZADOR\n\n");

    while(1)
    {
        //Verifica se flag_tree está ativa e se estiver envia TREE_QUERY a todos os filhos
        if(flag_tree)
        {
            redirect_queue_head = root_send_tree_query(redirect_queue_head, &redirect_queue_tail, fd_array,
                    &empty_redirect_queue, &tcp_occupied);
        }


        now = time(NULL);
        if(now -reference_time_whoisroot >= tsecs)
        {
            msg_whoisroot = find_whoisroot(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport);
            if(!strcmp(msg_whoisroot, "ERROR"))
            {
                //O que fazer? Pode acontecer?
                free(msg_whoisroot);
                msg_whoisroot = NULL;
                do
                {
                    msg_whoisroot = find_whoisroot(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport);
                }while(!strcmp(msg_whoisroot, "ERROR"));

                strncpy(buffer_whoisroot, msg_whoisroot, 6);
                buffer_whoisroot[6] = '\0';

                if(!strcmp(buffer_whoisroot, "URROOT"))
                {
                    reference_time_whoisroot = time(NULL);
                }
                else if(!strcmp(buffer_whoisroot, "ROOTIS"))
                {
                    //O que fazer? Pode acontecer?
                    //Prof diz para não fazer nada e deixar criar árvore paralela
                }
            }
            else
            {
                strncpy(buffer_whoisroot, msg_whoisroot, 6);
                buffer_whoisroot[6] = '\0';

                if(!strcmp(buffer_whoisroot, "URROOT"))
                {
                    reference_time_whoisroot = time(NULL);
                }
                else if(!strcmp(buffer_whoisroot, "ROOTIS"))
                {
                    //O que fazer? Pode acontecer?
                    //Prof diz para não fazer nada e deixar criar árvore paralela
                }
            }
            free(msg_whoisroot);
            msg_whoisroot = NULL;
        }
        if(now - reference_time_pop_query >= POP_QUERY_TIMEOUT && tcp_occupied > 0)
        {
            query_id++;
            redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops, redirect_queue_head,
                                                  &redirect_queue_tail, &tcp_occupied, &empty_redirect_queue);
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
                    if(fd_array[i] != -1)
                    {
                        close(fd_array[i]);
                        fd_array[i] = -1;
                    }
                }
            }

            reference_time_pop_query = time(NULL);
        }

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

        //Espera que 1 ou mais file descriptors estejam prontos
        counter = select(maxfd + 1, &fd_read_set, (fd_set *)NULL, (fd_set *)NULL,  (struct timeval *)NULL);
        if(counter <= 0)
        {
            if(flag_d) fprintf(stderr, "Error: select: %s\n", strerror(errno));
            return;
        }

        //Servidor fonto
        if(FD_ISSET(fd_ss, &fd_read_set))
        {
            data = buffer_data;
            n = receive_data_root(data, fd_ss, tcp_sessions, &redirect_queue_head, &redirect_queue_tail,
                    &empty_redirect_queue, &is_flowing, fd_array, &tcp_occupied);
            if(n != 0)
            {
                //Imprime os dados recebidos
                data[n] = '\0';
                data_len = n;
                if(flag_b) printf("%s", data);
                fflush(stdout);

                //Envia os dados aos pares a jusante. Recebe como retorno a head de lista de redirects
                redirect_queue_head = send_data_root(data, data_len, tcp_sessions, fd_array, &tcp_occupied,
                        redirect_queue_head, &redirect_queue_tail, &empty_redirect_queue);
            }
        }

        //Pares a jusante
        for(i = 0; i<tcp_sessions; i++)
        {
            if(fd_array[i] != -1)
            {
                if(FD_ISSET(fd_array[i], &fd_read_set))
                {
                    ptr = msg;


                   /* if(aux_ptr_sons[i] == NULL)
                    {
                        aux_ptr_sons[i] = aux_buffer_sons[i];
                    }
                    //Se o ponteiro estiver a apontar para um '\n' significa que a mensagem anterior foi lida na totalidade
                    else if(*aux_ptr_sons[i] == '\n')
                    {
                        aux_ptr_sons[i] = aux_buffer_sons[i];
                    }*/


                    nread_sons[i] = tcp_receive2(BUFFER_SIZE -nread_sons[i] - 1, (aux_ptr_sons[i]), fd_array[i]);
                    aux_ptr_sons[i] += nread_sons[i];
                    //ptr = aux_ptr_sons[i];

                    //n = tcp_receive(MAX_BYTES, ptr, fd_array[i]);
                    aux = getElementByIndex(redirect_queue_head, i);

                    if(nread_sons[i] == 0)
                    {
                        redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                &empty_redirect_queue, NULL, 1);
                    }
                    else if(aux_buffer_sons[i][nread_sons[i] - 1] == '\n')
                    {
                        //Coloca um \0 no fim da mensagem recebida
                        //Se n fosse igual a MAX_BYTES estariamos a apagar o ultimo carater recebido
                        /*if (n < MAX_BYTES) {
                           // *(ptr + n - 1) = '\0';

                        }*/
                        aux_buffer_sons[i][nread_sons[i]] = '\0';

                        //Copia os 2 primeiros carateres de msg para buffer
                        //strncpy(buffer, msg, 2);
                        //buffer[2] = '\0';
                        strncpy(buffer, aux_buffer_sons[i], 2);
                        buffer[2] = '\0';

                        //NEW_POP
                        if(!strcmp("NP", buffer))
                        {
                            redirect_queue_head = receive_newpop(redirect_queue_head, &redirect_queue_tail, i, fd_array,
                                    &empty_redirect_queue, aux_buffer_sons[i]);
                            aux = getElementByIndex(redirect_queue_head, i);
                            if(flag_d) printf("Mensagem recebida do novo par a jusante: NP %s:%s\n", getIP(aux), getPORT(aux));

                            //Depois de receber o NP, envia um SF/BS
                            redirect_queue_head = send_is_flowing_broken(is_flowing, fd_array, i, &tcp_occupied, redirect_queue_head, aux,
                                                                  &redirect_queue_tail, NULL, &empty_redirect_queue, 1);


                            query_id++;
                            redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops, redirect_queue_head,
                                                                  &redirect_queue_tail, &tcp_occupied, &empty_redirect_queue);
                            //está à espera da resposta do pop_query
                            waiting_pop_reply = 1;

                            if(redirect_queue_head == NULL)
                            {
                                //Se a a cabeça da lista de elementos diretamente a jusante for NULL, a lista ficou vazia
                                empty_redirect_queue = 1;
                                tcp_occupied = 0;
                                for(j = 0; j<tcp_sessions; j++)
                                {
                                    if(fd_array[j] != -1)
                                    {
                                        close(fd_array[j]);
                                        fd_array[j] = -1;
                                    }
                                }
                            }


                        }
                        if(!strcmp("PR", buffer))
                        {
                            if(flag_d) printf("Mensagem recebida do par a jusante %s:%s: %s\n", getIP(aux), getPORT(aux), aux_buffer_sons[i]);
                            pops_queue_head = get_data_pop_reply(pops_queue_head, &pops_queue_tail, aux_buffer_sons[i], &empty_pops_queue,
                                    query_id, &received_pops, waiting_pop_reply);

                            //Liberta a lista de pops auxiliares, pois esta já não é precisa
                            if(received_pops >= bestpops)
                            {
                                waiting_pop_reply = 0;
                                received_pops = 0;
                                freeQueue(aux_pops_queue_head);
                                aux_pops_queue_aux = NULL;
                                aux_pops_queue_head = NULL;
                                aux_pops_queue_tail = NULL;
                                empty_aux_pops_queue = 1;
                            }
                        }
                        else if(!strcmp("TR", buffer))
                        {
                            if(flag_d) printf("Mensagem recebida do par a jusante %s:%s: %s\n", getIP(aux), getPORT(aux), aux_buffer_sons[i]);
                            //Receber o tree reply
                            //Vai receber de um filho e vai


                        }

                        aux_buffer_sons[i][0] = '\0';
                        aux_ptr_sons[i] = aux_buffer_sons[i];
                        buffer[0] = '\0';
                        nread_sons[i] = 0;
                    }

                  /*  if(aux_buffer_sons[nread-1] == '\n')
                    {
                        aux_buffer_sons[i] = '\0';
                        aux_ptr_sons[i] = aux_buffer_sons[i];
                        buffer[0] = '\0';
                    }*/



                   /* ptr = NULL;
                    msg[0] = '\0';
                    buffer[0] = '\0';*/
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

                //porquê tcp_sessions == tcp_occupied +1?
                if(tcp_occupied > 0 && empty_pops_queue && waiting_pop_reply == 0)
                {
                    //faz pop_query
                    query_id++;
                    redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops, redirect_queue_head,
                                                          &redirect_queue_tail, &tcp_occupied, &empty_redirect_queue);
                    //está à espera da resposta do pop_query
                    waiting_pop_reply = 1;

                    if(redirect_queue_head == NULL)
                    {
                        //Se a a cabeça da lista de elementos diretamente a jusante for NULL, a lista ficou vazia
                        empty_redirect_queue = 1;
                        tcp_occupied = 0;
                        for(i = 0; i<tcp_sessions; i++)
                        {
                            if(fd_array[i] != -1)
                            {
                                close(fd_array[i]);
                                fd_array[i] = -1;
                            }
                        }
                    }
                }
            }
            else
            {
                //Se não estiver à espera de um pop_reply
                //&&received_pops == 0
              //  if(waiting_pop_reply == 0)
                //{
                    //se não tiver nenhum elemento na lista de pops
                    if(empty_pops_queue && tcp_occupied > 0)
                    {
                        //faz pop_query
                        query_id++;
                        redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops, redirect_queue_head,
                                                              &redirect_queue_tail, &tcp_occupied, &empty_redirect_queue);
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
                                if(fd_array[i] != -1)
                                {
                                    close(fd_array[i]);
                                    fd_array[i] = -1;
                                }
                            }
                        }


                        //Para não causar atrasos, recorre à lista auxiliar de pops, caso esta não esteja vazia e
                        //dá um desses pops
                        if(empty_aux_pops_queue == 0)
                        {
                            //Lista "circular" como para o redirect
                            if(aux_pops_queue_aux == NULL)
                            {
                                aux_pops_queue_aux = aux_pops_queue_head;
                            }
                            popresp(fd_udp, streamID, getIP(aux_pops_queue_aux), getPORT(aux_pops_queue_aux));
                            aux_pops_queue_aux = getNext(aux_pops_queue_aux);
                        }
                    }
                    else
                    {
                        //Caso ainda haja elementos na lista de pops, indicar o endereço do primeiro
                        popresp(fd_udp, streamID, getIP(pops_queue_head), getPORT(pops_queue_head));
                        decreaseAvailableSessions(pops_queue_head);
                        if(getAvailableSessions(pops_queue_head) == 0)
                        {
                            //remover da lista de pops e passar para a lista de aux_pops
                            if(pops_queue_head == pops_queue_tail)
                            {
                                if(empty_aux_pops_queue)
                                {
                                    //Se a lista auxiliar de pops estiver vazia, cria a lista
                                    aux_pops_queue_head = pops_queue_head;
                                    aux_pops_queue_tail = aux_pops_queue_head;
                                    empty_aux_pops_queue = 0;
                                }
                                else
                                {
                                    //Se não estiver vazia, insere na tail
                                    setNext(aux_pops_queue_tail, pops_queue_head);
                                    aux_pops_queue_tail = pops_queue_head;
                                }

                                pops_queue_head = NULL;
                                pops_queue_tail = NULL;
                                empty_pops_queue = 1;


                                if(tcp_occupied > 0)
                                {
                                    query_id++;
                                    redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops,
                                                                          redirect_queue_head, &redirect_queue_tail, &tcp_occupied, &empty_redirect_queue);
                                    waiting_pop_reply = 1;
                                    received_pops = 0;

                                    if(redirect_queue_head == NULL)
                                    {
                                        //Se a a cabeça da lista de elementos diretamente a jusante for NULL, a lista ficou vazia
                                        empty_redirect_queue = 1;
                                        tcp_occupied = 0;
                                        for(i = 0; i<tcp_sessions; i++)
                                        {
                                            if(fd_array[i] != -1)
                                            {
                                                close(fd_array[i]);
                                                fd_array[i] = -1;
                                            }
                                        }
                                    }
                                }

                            }
                            else
                            {
                                if(empty_aux_pops_queue)
                                {
                                    //Se a lista auxiliar de pops estiver vazia, cria a lista
                                    aux_pops_queue_head = pops_queue_head;
                                    aux_pops_queue_tail = aux_pops_queue_head;
                                    pops_queue_head = getNext(pops_queue_head);
                                    setNext(aux_pops_queue_head, NULL);
                                    empty_aux_pops_queue = 0;
                                }
                                else
                                {
                                    //Se não estiver vazia, insere na tail
                                    setNext(aux_pops_queue_tail, pops_queue_head);
                                    aux_pops_queue_tail = pops_queue_head;
                                    pops_queue_head = getNext(pops_queue_head);
                                    setNext(aux_pops_queue_tail, NULL);
                                }
                            }
                        }
                    }
                //}
              /*  else
                {
                    //Isto está muito estranho. Permite redirecionar mas imprime coisas que nunca mais acabam, porque
                    //até chegarem as PR passa muitas vezes aqui
                    //Evita ficar preso aqui
                    if(counter_aux_pops == MAX_AUX_POPS)
                    {
                        query_id++;
                        redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops,
                                                              redirect_queue_head, &redirect_queue_tail);
                        waiting_pop_reply = 0;
                        received_pops = 0;
                        counter_aux_pops = 0;
                    }
                    else
                    {
                        //Para não causar atrasos, recorre à lista auxiliar de pops, caso esta não esteja vazia e
                        //dá um desses pops
                        if(empty_aux_pops_queue == 0)
                        {
                            //Lista "circular" como para o redirect
                            if(aux_pops_queue_aux == NULL)
                            {
                                aux_pops_queue_aux = aux_pops_queue_head;
                            }
                            popresp(fd_udp, streamID, getIP(aux_pops_queue_aux), getPORT(aux_pops_queue_aux));
                            aux_pops_queue_aux = getNext(aux_pops_queue_aux);
                        }
                        counter_aux_pops++;
                    }
                }*/
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
                freeQueue(aux_pops_queue_head);
                for(i = 0; i<tcp_sessions; i++)
                {
                    free(aux_buffer_sons[i]);
                }
                free(aux_buffer_sons);
                free(aux_ptr_sons);
                return;
            }
        }
    }
}

void interface_not_root(int fd_rs, struct addrinfo *res_rs, char* streamID, char *streamIP, char *streamPORT, int *is_root, char* ipaddr, char *uport,
        char *tport, int tcp_sessions, int tcp_occupied, int fd_tcp_server, int *fd_array, int bestpops,
        int fd_pop, char *pop_addr, char *pop_tport, char *rsaddr, char *rsport, int tsecs)
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
    struct _queue* aux = NULL;
   
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

    //Dados
    char *data = NULL;
    char *data_ptr = NULL;
    int data_len;


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
                    aux = getElementByIndex(redirect_queue_head, i);

                    if(n == 0)
                    {
                        redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                       &empty_redirect_queue, NULL, 1);
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

                        if(!strcmp("NP", buffer))
                        {
                            redirect_queue_head = receive_newpop(redirect_queue_head, &redirect_queue_tail, i, fd_array,
                                                                 &empty_redirect_queue, ptr);
                            aux = getElementByIndex(redirect_queue_head, i);
                            if(flag_d) printf("Mensagem recebida do novo par a jusante: NP %s:%s\n", getIP(aux), getPORT(aux));

                            redirect_queue_head = send_is_flowing_broken(is_flowing, fd_array, i, &tcp_occupied, redirect_queue_head,
                                    aux, &redirect_queue_tail, NULL, &empty_redirect_queue, 1);
                        }
                        else if(!strcmp("PR", buffer))
                        {

                            if(flag_d) printf("Mensagem recebida do par a jusante %s:%s: %s\n", getIP(aux), getPORT(aux), ptr);

                            query_id_rcvd_aux = receive_pop_reply(ptr, popreply_ip, popreply_port, &popreply_avails);
                            if(requested_pops - sent_pops > 0) //Se tiver pops por enviar
                            {
                                query_id_rcvd = query_id_rcvd_aux;

                                //Verifica o queryID recebido na reply é o mesmo que o da última query
                                if(query_id == query_id_rcvd)
                                {
                                    n = send_pop_reply(query_id, popreply_avails, popreply_ip, popreply_port, fd_pop);

                                    if(n == 0)
                                    {
                                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                        if(flag_d) printf("Perdida a ligação ao par a montante\n\n");
                                        close(fd_pop);
                                        fd_pop = -1;

                                        n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                                     fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                                     pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux, tsecs);
                                        if(n == 1) return;
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
                                if(flag_d) printf("Pop reply descartada, pois já foram recebidos todos os pops pedidos!\n\n");
                            }
                        }
                        else if(!strcmp("TR", buffer))
                        {
                            if(flag_d) printf("Mensagem recebida do para a jusante com índice %d: %s\n", i, ptr);
                            //Receber TR
                            n = receive_tree_reply_and_propagate(ptr, fd_pop, fd_array[i]);
                            if(n == 10)
                            {
                                redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                               &empty_redirect_queue, NULL, 1);
                            }
                            else if(n == 0)
                            {

                                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                if(flag_d) printf("Perdida a ligação ao par a montante...\n\n");
                                close(fd_pop);
                                fd_pop = -1;

                                n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                             fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                             pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux, tsecs);
                                if(n == 1) return;
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
                fd_pop = -1;

                //Readere à stream
                n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                             fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                             pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux, tsecs);
                if(n == 1) return;
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

                if(!strcmp("DA", buffer))
                {
                    //Recebe o header da mensagem
                    n = receive_data_header(&data_len, msg);
                    if(n == -1)
                    {
                        if(flag_d) printf("Erro no encapsulamento da mensagem\n");
                        ////////////////SAIR DA STREAM???????
                    }
                    if(n != -1)
                    {
                        data = (char *)malloc(sizeof(char)*(data_len+1));
                        if(data == NULL)
                        {
                            if(flag_d) printf("Erro: malloc: %s\n", strerror(errno));
                            continue;
                        }

                        data_ptr = data;
                        n = read(fd_pop, data, data_len);
                        if(n == 0 || n == -1)
                        {
                            //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                            if(flag_d) printf("Perdida a ligação ao par a montante...\n");
                            close(fd_pop);
                            fd_pop = -1;

                            n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                         fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                         pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux, tsecs);
                            if(n == 1) return;
                        }
                        else
                        {
                            data[n] = '\0';
                            if(flag_b) printf("%s", data_ptr);
                            fflush(stdout);


                            sprintf(msg, "DA %04X\n", data_len);
                            ptr = msg;

                            //Retransmitir
                            for(i = 0; i<tcp_sessions; i++)
                            {
                                if(fd_array[i] != -1)
                                {
                                    aux = getElementByIndex(redirect_queue_head, i);

                                    n = tcp_send(strlen(msg), msg, fd_array[i]);
                                    if(n == 0)
                                    {
                                        redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                                       &empty_redirect_queue, NULL, 1);
                                        continue;
                                    }
                                    if(flag_d) printf("Mensagem enviada ao para a jusante %s:%s: %s\n", getIP(aux),
                                            getPORT(aux), msg);


                                    n = tcp_send(data_len, data_ptr, fd_array[i]);
                                    if(n == 0)
                                    {
                                        redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                                       &empty_redirect_queue, NULL, 1);
                                        continue;
                                    }
                                    if(flag_d) printf("Mensagem enviada ao para a jusante %s:%s: %s\n", getIP(aux),
                                            getPORT(aux), data_ptr);
                                }
                            }



                        }

                        free(data);
                        data = NULL;
                    }




                }
                else if(!strcmp("BS", buffer))
                {
                    if(flag_d) printf("Mensagem recebida do par a montante: BS\n\n");

                    //Envia broken stream aos pares
                    is_flowing = 0;
                    redirect_queue_head = send_broken_stream_to_all(fd_array, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                                    &empty_redirect_queue);
                }
                else if(!strcmp("SF", buffer))
                {
                    if(flag_d) printf("Mensagem recebida do par a montante: SF\n\n");
                    is_flowing = 1;
                    redirect_queue_head = send_stream_flowing_to_all(fd_array, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                            &empty_redirect_queue);

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
                        if(tcp_sessions - tcp_occupied > 0)
                        {
                            //Envia os pops disponíveis
                            n = send_pop_reply(query_id, tcp_sessions - tcp_occupied, ipaddr, tport, fd_pop);
                            if(n == 0)
                            {
                                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                if(flag_d) printf("Perdida a ligação ao par a montante...\n");
                                close(fd_pop);
                                fd_pop = -1;
                                n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                             fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                             pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux, tsecs);
                                if(n == 1) return;
                            }
                            else
                            {
                                sent_pops += tcp_sessions - tcp_occupied; //Atualiza se a comunicação correu bem
                            }


                            if(tcp_sessions -tcp_occupied < requested_pops && tcp_occupied > 0)
                            {
                                //Se os disponíveis forem menos que os pedidos, faz pop_query
                                redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, requested_pops - sent_pops,
                                                                      redirect_queue_head, &redirect_queue_tail, &tcp_occupied,
                                                                      &empty_redirect_queue);
                                if(redirect_queue_head == NULL)
                                {
                                    //Se a a cabeça da lista de elementos diretamente a jusante for NULL, a lista ficou vazia
                                    empty_redirect_queue = 1;
                                    tcp_occupied = 0;
                                    for(i = 0; i<tcp_sessions; i++)
                                    {
                                        if(fd_array[i] != -1)
                                        {
                                            close(fd_array[i]);
                                            fd_array[i] = -1;
                                        }
                                    }
                                }
                            }
                        }
                        else if(tcp_occupied > 0)
                        {
                            redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, requested_pops,
                                                                  redirect_queue_head, &redirect_queue_tail, &tcp_occupied,
                                                                  &empty_redirect_queue);

                            if(redirect_queue_head == NULL)
                            {
                                //Se a a cabeça da lista de elementos diretamente a jusante for NULL, a lista ficou vazia
                                empty_redirect_queue = 1;
                                tcp_occupied = 0;
                                for(i = 0; i<tcp_sessions; i++)
                                {
                                    if(fd_array[i] != -1)
                                    {
                                        close(fd_array[i]);
                                        fd_array[i] = -1;
                                    }
                                }
                            }
                        }
                    }

                }
                else if(!strcmp("TQ", buffer))               
                {                   
                    if(flag_d) printf("Mensagem recebida do par a montante: %s\n", ptr);
                    n = receive_tree_query(ptr, treequery_ip, treequery_port);

                    if(n != -1)
                    {
                        validate_treequery = compare_ip_and_port(treequery_ip, treequery_port, ipaddr, tport);

                        if(validate_treequery == 0) //O ip e o porto de destino forem o desta iamroot
                        {                        //Responder com um TREE_REPLY com capacidade e ocupação do ponto de acesso
                            n = send_tree_reply(treequery_ip, treequery_port, tcp_sessions, tcp_occupied, redirect_queue_head, redirect_queue_tail, fd_pop);
                            if(n == 0)
                            {
                                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                if(flag_d) printf("Perdida a ligação ao par a montante...\n");
                                close(fd_pop);
                                fd_pop = -1;

                                n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                             fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                             pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux, tsecs);
                                if(n == 1) return;
                            }
                        }
                        else //Caso contrário deverá replicar a mensagem a jusante a menos que não tenha pares a jusante
                        {
                            //Percorrer a lista redirect e enviar para cada um deles a mesma mensagem -> send_tree_query

                            redirect_queue_head = send_tree_query(treequery_ip, treequery_port, fd_array, tcp_sessions,
                                    &tcp_occupied, redirect_queue_head, &redirect_queue_tail, &empty_redirect_queue);

                        }
                    }
                    else
                    {
                        printf("Mensagem Tree Query inválida!\n\n");
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
    else if(!strcasecmp(buffer, "tree\n"))
    {
        if(is_root)
        {
            //Apresentar estrutura da transmissão
            printf("Estrutura de transmissão em árvore\n");
            //meter flag global a 1 e na interface_root se tiver flag ativa chamar função q envia tree query aos filhos
            flag_tree = 1;
        }
        else
        {
            printf("O comando tree apenas funciona para a raíz\n");
        }

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
    if(token == NULL)
    {
        if(flag_d) printf("Mensagem New Pop inválida\n");
        return redirect_queue_head;
    }
    token = NULL;

    token = strtok(NULL, ":");
    if (token == NULL)
    {
        if (flag_d) printf("Endereço IP inválido\n");
        return redirect_queue_head;
    }
    strcpy(ip_aux, token);
    token = NULL;

    token = strtok(NULL, "\n");
    if (token == NULL)
    {
        if (flag_d) printf("Porto inválido\n");
        return redirect_queue_head;
    }
    strcpy(port_aux, token);
    token = NULL;

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
        queue **redirect_queue_tail, int *tcp_occupied, int *empty_redirect_queue)
{
    int i;
    int n;
    queue *previous = NULL, *aux = NULL;

    aux = redirect_queue_head;
    while(aux != NULL)
    {
        i = getIndex(aux);

        n = pop_query(query_id, bestpops, fd_array[i]);
        if(n == 0)
        {
            redirect_queue_head = lost_son(aux, fd_array, i, tcp_occupied, redirect_queue_head, redirect_queue_tail,
                                           empty_redirect_queue, previous, 0);
        }
        else if(n == 1)
        {
            if(flag_d) printf("Mensagem enviada ao par %s:%s: PQ %04X %d\n\n", getIP(aux), getPORT(aux), query_id, bestpops);
        }

        previous = aux;
        aux = getNext(aux);
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
        //Se o query id recebido na mensagem do POP REPLY fôr igual ao queryid atual
        if(query_id_rcvd == query_id)
        {
            *received_pops += available_sessions;


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
        if(flag_d) printf("Pop reply descartada, pois já foram recebidas todas as pedidas!\n\n");
    }


    return pops_queue_head;   
}

//retorna 1 se for para acabar o programa. caso em que passou a ser raiz e depois saiu
//retorna 0 se for para continuar. caso em que nunca chegou a ser raiz
int readesao(struct addrinfo *res_rs, int fd_rs, char *streamID, char *rsaddr, char *rsport, char *ipaddr, char *uport,
        queue **redirect_queue_head, queue **redirect_queue_tail, int *fd_array, int *tcp_occupied, int tcp_sessions,
        int *empty_redirect_queue, int *is_root, char *pop_addr, char *pop_tport, int *fd_pop, char *streamIP,
        char *streamPORT, char *tport, int fd_tcp_server, int bestpops, queue *redirect_aux, int tsecs)
{
    //Variáveis para readesão
    struct addrinfo *res_udp = NULL;
    int fd_udp = -1;
    int welcome_flag = 0;
    char rasaddr[IP_LEN + 1];
    char rasport[PORT_LEN + 1];
    int fd_ss = -1;
    char *msg_readesao = NULL;
    char buffer_readesao[BUFFER_SIZE];

    //Envia broken stream
    *redirect_queue_head = send_broken_stream_to_all(fd_array, tcp_occupied, *redirect_queue_head, redirect_queue_tail,
              empty_redirect_queue);


    /////////////////////////////// Aderir novamente à stream //////////////////////////////////
    msg_readesao = find_whoisroot(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport);

    if(!strcmp(msg_readesao, "ERROR"))
    {
        freeQueue(*redirect_queue_head);
        return 1;
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

            //Envia SF
            *redirect_queue_head = send_stream_flowing_to_all(fd_array, tcp_occupied, *redirect_queue_head, redirect_queue_tail,
                    empty_redirect_queue);

            //////////////////////////// 4. executar a interface de utilizador //////////////////////////////////////
            interface_root(fd_ss, fd_rs, res_rs, streamID, *is_root, ipaddr, uport, tport, tcp_sessions, *tcp_occupied,
                           fd_udp, fd_tcp_server, fd_array, bestpops, *redirect_queue_head, *redirect_queue_tail,
                           redirect_aux, *empty_redirect_queue, tsecs, rsaddr, rsport);
            return 1; //Se ele sair da inferface_root é porque o programa foi terminado

        }
        else if(!strcmp(buffer_readesao, "ROOTIS"))
        {
            //Obtém o IP e o porto do servidor de acesso
            get_root_access_server(rasaddr, rasport, msg_readesao, res_rs, fd_rs);

            *fd_pop = -1;
            while(*fd_pop == -1)
            {
                ///////////// 1. Solicita ao servidor de acesso da raíz o IP e porto TCP do ponto de acesso ////
                do
                {
                    fd_udp = get_access_point(rasaddr, rasport, &res_udp, fd_rs, res_rs, pop_addr, pop_tport, 1, ipaddr);
                }while(fd_udp == -2);

                if(fd_udp == -1)
                {
                    //falha na comunicação com o servidor de acessos. Podes significar que a raiz saiu
                    //Vamos tentar uma nova readesão
                    readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, redirect_queue_head, redirect_queue_tail,
                            fd_array, tcp_occupied, tcp_sessions, empty_redirect_queue, is_root, pop_addr, pop_tport,
                            fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux, tsecs);
                    return 1;
                }

                welcome_flag = 0;
                while(welcome_flag == 0) //Enquanto não tiver recebido um WELCOME com a stream esperada
                {
                    //////////////////////// 2. Estabelece sessão TCP com o ponto de acesso ///////////////////////////
                    *fd_pop = connect_to_peer(pop_addr, pop_tport, fd_rs, fd_udp, res_rs, 1);
                    if(*fd_pop == -1)
                    {
                        close(fd_udp);
                        fd_udp = -1;
                        welcome_flag = 1;
                    }
                    else
                    {
                        //////////////////////////// 3. Aguarda confirmação de adesão /////////////////////////////////////
                        welcome_flag = wait_for_confirmation(pop_addr, pop_tport, fd_rs, res_rs, fd_udp, *fd_pop, streamID);
                    }
                }
            }
            welcome_flag = 0;
            close(fd_udp);
            fd_udp = -1;

            send_new_pop(*fd_pop, ipaddr, tport, fd_rs, res_rs, fd_tcp_server, fd_array);
        }
    }
    return 0;

}

//retorna 0 em caso de falha
int receive_data_root(char *data, int fd_ss, int tcp_sessions, queue **redirect_queue_head, queue **redirect_queue_tail,
        int *empty_redirect_queue, int *is_flowing, int *fd_array, int *tcp_occupied)
{
    int n;
    queue *aux = NULL;
    queue *previous = NULL;
    int index;


    n = read(fd_ss, data, BUFFER_SIZE - 1);
    if(n == 0 || n == -1)
    {
        //Mandar broken stream em caso de perda de ligação
        if (flag_d) printf("Perdida a ligação ao servidor fonte\n");
        *is_flowing = 0;

        aux = *redirect_queue_head;
        while(aux != NULL)
        {
            index = getIndex(aux);
            n = broken_stream(fd_array[index]);
            if(n == 0)
            {
                *redirect_queue_head = lost_son(aux, fd_array, index, tcp_occupied, *redirect_queue_head, redirect_queue_tail,
                                               empty_redirect_queue, previous, 0);
            }
            else if(n == -1)
            {
                if(flag_d) printf("Falha ao comunicar com o par a jusante %s:%s\n", getIP(aux), getPORT(aux));
            }

            previous = aux;
            aux = getNext(aux);
        }

        return 0;
    }

    return n;
}

queue *send_data_root(char *data, int data_len, int tcp_sessions, int *fd_array, int *tcp_occupied, queue *redirect_queue_head,
                      queue **redirect_queue_tail, int *empty_redirect_queue)
{
    int n, index;
    queue *aux = NULL;
    queue *previous = NULL;
    char data_header[DATA_HEADER_LEN];
    char *ptr = NULL;

    sprintf(data_header, "DA %04X\n", data_len);

    aux = redirect_queue_head;
    while(aux != NULL)
    {
        index = getIndex(aux);
        ptr = data_header;
        n = tcp_send(strlen(ptr), ptr, fd_array[index]);
        if(n == 0)
        {
            redirect_queue_head = lost_son(aux, fd_array, index, tcp_occupied, redirect_queue_head, redirect_queue_tail,
                                           empty_redirect_queue, previous, 0);
        }
        else
        {
            n = tcp_send(data_len, data, fd_array[index]);
            if(n == 0)
            {
                redirect_queue_head = lost_son(aux, fd_array, index, tcp_occupied, redirect_queue_head, redirect_queue_tail,
                        empty_redirect_queue, previous, 0);
            }
        }

        previous = aux;
        aux = getNext(aux);
    }

    return redirect_queue_head;
}

queue *send_is_flowing_broken(int is_flowing, int *fd_array, int index, int *tcp_occupied, queue *redirect_queue_head,
        queue *element, queue **redirect_queue_tail, queue *previous, int *empty_redirect_queue, int remove_by_index)
{
    int n;

    if(is_flowing)
    {
        n = stream_flowing(fd_array[index]);
    }
    else
    {
        n = broken_stream(fd_array[index]);
    }

    if(n == 0)
    {
        redirect_queue_head = lost_son(element, fd_array, index, tcp_occupied, redirect_queue_head, redirect_queue_tail,
                empty_redirect_queue, previous, remove_by_index);

    }
    if(is_flowing)
    {
        if(flag_d) printf("Mensagem enviada ao par a jusante %s:%s: SF\n\n", getIP(element), getPORT(element));
    }
    else
    {
        if(flag_d) printf("Mensagem enviada ao par a jusante %s:%s: BS\n\n", getIP(element), getPORT(element));
    }




    return redirect_queue_head;
}

queue *send_broken_stream_to_all(int *fd_array, int *tcp_occupied, queue *redirect_queue_head, queue **redirect_queue_tail,
        int *empty_redirect_queue)
{
    queue *aux = NULL;
    queue *previous = NULL;
    int n, index;

    aux = redirect_queue_head;
    while(aux != NULL)
    {
        index = getIndex(aux);
        n = broken_stream(fd_array[index]);

        if(n == 0)
        {
            redirect_queue_head = lost_son(aux, fd_array, index, tcp_occupied, redirect_queue_head, redirect_queue_tail,
                                           empty_redirect_queue, previous, 0);
        }

        if(flag_d) printf("Mensagem enviada ao par a jusante %s:%s: BS\n\n", getIP(aux), getPORT(aux));
        previous = aux;
        aux = getNext(aux);
    }

    return redirect_queue_head;
}

queue *send_stream_flowing_to_all(int *fd_array, int *tcp_occupied, queue *redirect_queue_head, queue **redirect_queue_tail,
                                 int *empty_redirect_queue)
{
    queue *aux = NULL;
    queue *previous = NULL;
    int n, index;

    aux = redirect_queue_head;
    while(aux != NULL)
    {
        index = getIndex(aux);
        n = stream_flowing(fd_array[index]);

        if(n == 0)
        {
            redirect_queue_head = lost_son(aux, fd_array, index, tcp_occupied, redirect_queue_head, redirect_queue_tail,
                    empty_redirect_queue, previous, 0);
        }

        if(flag_d) printf("Mensagem enviada ao par a jusante %s:%s: SF\n\n", getIP(aux), getPORT(aux));
        previous = aux;
        aux = getNext(aux);
    }

    return redirect_queue_head;
}

queue* lost_son(queue *aux, int *fd_array, int i, int *tcp_occupied, queue *redirect_queue_head, queue **redirect_queue_tail,
        int *empty_redirect_queue, queue *previous, int remove_by_index)
{
    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
    if(getIP(aux) != NULL && getPORT(aux) != NULL)
    {
        if(flag_d) printf("Perdida a ligação ao par a jusante %s:%s\n\n", getIP(aux), getPORT(aux));
    }
    else
    {
        if(flag_d) printf("Perdida a ligação ao par a jusante acabado de ligar\n\n");
    }

    close(fd_array[i]);
    fd_array[i] = -1;
    (*tcp_occupied)--;
    if(remove_by_index)
    {
        redirect_queue_head = removeElementByIndex(redirect_queue_head, redirect_queue_tail, i) ;
    }
    else
    {
        redirect_queue_head = removeElement(aux, redirect_queue_head, redirect_queue_tail, previous);
    }
    if(redirect_queue_head == NULL) *empty_redirect_queue = 1;

    return redirect_queue_head;
}

int buffer_interm_sons(char **aux_ptr_sons, char **aux_buffer_sons, int* nread_sons, int tcp_sessions)
{
    int i;
    int j;

    aux_ptr_sons = (char **)malloc(sizeof(char*)*tcp_sessions);
    if(aux_ptr_sons == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        return -1;
    }

    aux_buffer_sons = (char **)malloc(sizeof(char*)*tcp_sessions);
    if(aux_buffer_sons == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        free(aux_ptr_sons);
        return -1;
    }

    for(i = 0; i<tcp_sessions; i++)
    {
        aux_buffer_sons[i] = NULL;
        aux_buffer_sons[i] = (char *)malloc(sizeof(char)*BUFFER_SIZE);
        if(aux_buffer_sons[i] == NULL)
        {
            for(j = 0; j<i; j++)
            {
                free(aux_buffer_sons[j]);
            }
            free(aux_buffer_sons);
            free(aux_ptr_sons);
            if(flag_d) fprintf(stdout, "Erro: malloc: %s\n\n", strerror(errno));
            return -1;
        }
        //aux_buffer_sons[i] = '\0';
        strcpy(aux_buffer_sons[i], "\0");
        aux_ptr_sons[i] = aux_buffer_sons[i];
    }

    nread_sons = (int *)malloc(sizeof(int)*tcp_sessions);
    if(nread_sons == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        for(i = 0; i<tcp_sessions; i++)
        {
            free(aux_buffer_sons[i]);
        }
        free(aux_buffer_sons);
        free(aux_ptr_sons);
        return -1;
    }

    for(i = 0; i<tcp_sessions; i++)
    {
        nread_sons[i] = 0;
    }

    return 0;
}











