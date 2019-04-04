#include "interface.h"
#include "utils.h"
#include "list_to_print.h"
#include "intermediate_list.h"

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
        queue *redirect_queue_tail, queue *redirect_aux, int empty_redirect_queue, int tsecs, char *rsaddr, char *rsport,
        char **aux_buffer_sons, char **aux_ptr_sons, int *nread_sons, int is_flowing, int query_id, char *streamIP,
        char *streamPORT)
{
    //Variáveis para o select
    int maxfd, counter;
    fd_set fd_read_set;

    //Indica se foi introduzido o comando exit
    int exit_flag = 0;

    //PQ/PR
    int waiting_pop_reply = 0;

    //Queue de pops. A raiz nunca faz parte da lista
    struct _queue *pops_queue_head = NULL;
    struct _queue *pops_queue_tail = NULL;
    int empty_pops_queue = 1;

    //Queue de pops auxiliares. Podem não estar atualizados, mas são utilizados quando
    //não há pops para dar e ainda não chegaram os resultados do PQ
    struct _queue *aux_pops_queue_head = NULL;
    struct _queue *aux_pops_queue_tail = NULL;
    struct _queue *aux_pops_queue_aux = NULL;
    int empty_aux_pops_queue = 1;

    //Buffers intermédios para leitura dos filhos
    //char **aux_buffer_sons = NULL;
    //char **aux_ptr_sons = NULL;
    //int *nread_sons = NULL;

    //Indica que o par vai ser removido por estar a mandar lixo
    int flag_remove = 0;

    //Dados da stream
    char *data = NULL;
    char buffer_data[DATA_SIZE];
    int data_len;

    //Timers e variáveis para whoisroot
    long reference_time_whoisroot = time(NULL);
    long reference_time_pop_query = time(NULL);
    long now = time(NULL);
    char *msg_whoisroot = NULL;

    //Servidor de acessos
    struct sockaddr_in addr;
    unsigned int addrlen;

    //Variáveis para ciclos for
    int i;

    //Variável de verificação de retorno
    int n;

    //Variável de verificação de headers de mensagens
    char buffer[MAX_BYTES]; buffer[0] = '\0';

    //Nó auxiliar relativo a um par a jusante
    queue *aux = NULL;
    queue *pops_aux = NULL;

     //////////////////////////////////////////
    struct _printlist *head_print = NULL;
    struct _printlist *tail_print = NULL;
    char *line;
    int missing = 0;

    struct _intermlist **interm_list = NULL;
    struct _intermlist **interm_tail = NULL;

    interm_tail = (struct _intermlist **)malloc(sizeof(struct _intermlist*)*tcp_sessions);
    if(interm_tail == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        return;
    }


    interm_list = (struct _intermlist **)malloc(sizeof(struct _intermlist*)*tcp_sessions);
    if(interm_list == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        free(interm_tail);
        return;
    }

    for(i = 0; i<tcp_sessions; i++) interm_list[i] = NULL;

    int *waiting_tr = NULL;

    waiting_tr = (int*)malloc(sizeof(int)*tcp_sessions);
    if(waiting_tr == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        free(interm_list);
        free(interm_tail);
        return;
    }
    for(i = 0; i<tcp_sessions; i++) waiting_tr[i] = 0;
    

    int insert_tail = 1;

    if(aux_buffer_sons == NULL && aux_ptr_sons == NULL && nread_sons == NULL)
    {
        //Inicializa as variáveis necessárias para os buffers intermédios a jusante
        n = buffer_interm_sons(&aux_ptr_sons, &aux_buffer_sons, &nread_sons, tcp_sessions);
        if(n == -1) return;
    }

    struct timeval *timeout = NULL;

    timeout = (struct timeval *)malloc(sizeof(struct timeval));
    if(timeout == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        free(interm_list);
        free(interm_tail);
        for(i = 0; i<tcp_sessions; i++)
        {
            free(aux_buffer_sons[i]);
        }
        free(aux_buffer_sons);
        free(aux_ptr_sons);
        free(nread_sons);
        return;
    }

    if(tsecs > POP_QUERY_TIMEOUT)
    {
        timeout->tv_sec = POP_QUERY_TIMEOUT;
    }
    else
    {
        timeout->tv_sec = tsecs;
    }

    timeout->tv_usec = 0;

    int refuse_fd = -1;

    query_list *head_pop_query_list = NULL;
    query_list *tail_pop_query_list = NULL;
    query_list *current_pop_query_list = NULL;
    query_list *previous_pop_query_list = NULL;

    int query_id_rcvd = 0;
    char ip_pop[IP_LEN + 1];
    char port_pop[PORT_LEN + 1];
    int available_sessions = 0;

    int ss_connect_counter = 0;

    int tree_to_print = 0;

    int *missing_tqs = NULL;

    missing_tqs = (int*)malloc(sizeof(int)*tcp_sessions);
    if(missing_tqs == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        free(interm_list);
        free(interm_tail);
        for(i = 0; i<tcp_sessions; i++)
        {
            free(aux_buffer_sons[i]);
        }
        free(aux_buffer_sons);
        free(aux_ptr_sons);
        free(nread_sons);
        return;
    }

    for(i = 0; i<tcp_sessions; i++) missing_tqs[i] = 0;

    printf("\n\nINTERFACE DE UTILIZADOR\n\n");

    while(1)
    {
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
            free(nread_sons);
            free(waiting_tr);
            for(i = 0; i<tcp_sessions; i++)
            {
                freeIntermList(interm_list[i]);
            }
            free(interm_list);
            free(interm_tail);
            free(timeout);
            freePrintList(head_print);
            freeQueryList(head_pop_query_list);
            free(missing_tqs);
            return;
        }

        //Verifica se flag_tree está ativa e se estiver envia TREE_QUERY a todos os filhos
        if(flag_tree)
        {
            redirect_queue_head = root_send_tree_query(redirect_queue_head, &redirect_queue_tail, fd_array,
                    &empty_redirect_queue, &tcp_occupied);
            missing = 0;
            for(i = 0; i<tcp_sessions; i++)
            {
                //Está ligado
                if(fd_array[i] != -1)
                {
                    missing_tqs[i] = 1;
                }
            }
            missing += tcp_occupied;
            flag_tree = 0;
            tree_to_print = 1;
        }

        if(tree_to_print == 1 && missing == 0)
        {
            printf("Estrutura de transmissão em árvore\n");
            print_tree(head_print, streamID, redirect_queue_head, ipaddr, tport, tcp_sessions);
            freePrintList(head_print);
            head_print = NULL;
            tree_to_print = 0;
        }


        now = time(NULL);
        if(now -reference_time_whoisroot >= tsecs)
        {
            msg_whoisroot = find_whoisroot(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, 0);
            if(msg_whoisroot != NULL)
            {
                if(flag_d) printf("Mensagem recebida do servidor de raízes: %s\n", msg_whoisroot);
                free(msg_whoisroot);
                msg_whoisroot = NULL;
                reference_time_whoisroot = time(NULL);
            }
        }
        if(now - reference_time_pop_query >= POP_QUERY_TIMEOUT && tcp_occupied > 0)
        {
            if(redirect_queue_head != NULL)
            {
                query_id++;
                redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops, redirect_queue_head,
                                                      &redirect_queue_tail, &tcp_occupied, &empty_redirect_queue);
                //está à espera da resposta do pop_query
                waiting_pop_reply = 1;

                if(head_pop_query_list == NULL)
                {
                    head_pop_query_list = newElementQuery(query_id, bestpops);
                    tail_pop_query_list = head_pop_query_list;
                }
                else
                {
                    insertTailQuery(query_id, bestpops, &tail_pop_query_list);
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
        counter = select(maxfd + 1, &fd_read_set, (fd_set *)NULL, (fd_set *)NULL,  timeout);
        if(counter < 0)
        {
            if(flag_d) fprintf(stderr, "Error: select: %s\n", strerror(errno));
            return;
        }

        //Servidor fonto
        if(FD_ISSET(fd_ss, &fd_read_set))
        {
            data = buffer_data;
            n = receive_data_root(data, fd_ss, tcp_sessions, &redirect_queue_head, &redirect_queue_tail,
                    &empty_redirect_queue, &is_flowing, fd_array, &tcp_occupied, missing_tqs, &missing);
            if(n != 0)
            {
                //Imprime os dados recebidos
                data[n] = '\0';
                data_len = n;


                if(flag_b)
                {
                    if(ascii)
                    {
                        printf("%s", data);
                        fflush(stdout);
                    }
                    else
                    {
                        print_hex(data);
                        fflush(stdout);
                    }
                }




                //Envia os dados aos pares a jusante. Recebe como retorno a head de lista de redirects
                redirect_queue_head = send_data_root(data, data_len, tcp_sessions, fd_array, &tcp_occupied,
                        redirect_queue_head, &redirect_queue_tail, &empty_redirect_queue, missing_tqs, &missing);
            }
            else
            {
                close(fd_ss);
                fd_ss = -1;
                ss_connect_counter = 0;
                while(ss_connect_counter < MAX_TRIES && fd_ss == -1)
                {
                    fd_ss = source_server_connect(fd_rs, res_rs, streamIP, streamPORT, streamID);
                    ss_connect_counter++;
                }

                if(fd_ss == -1)
                {
                    exit_flag = 1;
                }
            }
        }

        //Pares a jusante
        for(i = 0; i<tcp_sessions; i++)
        {
            if(fd_array[i] != -1)
            {
                if(FD_ISSET(fd_array[i], &fd_read_set))
                {
                    if(!waiting_tr[i])
                    {
                        flag_remove = 0;
                        if(nread_sons[i] >= SONS_BUFFER - 1)
                        {
                            flag_remove = 1;
                        }
                        nread_sons[i] = tcp_receive2(SONS_BUFFER - nread_sons[i] -1, aux_ptr_sons[i], fd_array[i]);
                        aux_ptr_sons[i] += nread_sons[i];
                        //ptr = aux_ptr_sons[i];

                        //n = tcp_receive(MAX_BYTES, ptr, fd_array[i]);
                        aux = getElementByIndex(redirect_queue_head, i);

                        if(flag_remove)
                        {
                            nread_sons[i] = 0;
                            aux_ptr_sons[i] = aux_buffer_sons[i];
                            flag_remove = 0;
                            if(flag_d)
                            {
                                if(getIP(aux) != NULL && getPORT(aux) != NULL)
                                {
                                    printf("O par %s:%s está a enviar lixo e será desconectado\n", getIP(aux), getPORT(aux));
                                }
                                else
                                {
                                    printf("O par acabado de ligar está a enviar lixo e será desconectado\n");
                                }
                            }
                        }
                    }


                    if(nread_sons[i] == 0)
                    {
                        redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                &empty_redirect_queue, NULL, 1);
                        missing -= missing_tqs[i];
                        missing_tqs[i] = 0;
                    }
                    else if(waiting_tr[i] || aux_buffer_sons[i][nread_sons[i] - 1] == '\n')
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

                            if(getIP(aux) != NULL && getPORT(aux) != NULL)
                            {
                                if(flag_d) printf("Mensagem recebida do novo par a jusante: NP %s:%s\n", getIP(aux), getPORT(aux));
                                //Depois de receber o NP, envia um SF/BS
                                redirect_queue_head = send_is_flowing(is_flowing, fd_array, i, &tcp_occupied, redirect_queue_head, aux,
                                                                             &redirect_queue_tail, NULL, &empty_redirect_queue, 1,
                                                                             missing_tqs, &missing);



                                if(waiting_pop_reply == 0)
                                {
                                    if(redirect_queue_head != NULL)
                                    {
                                        query_id++;
                                        redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops, redirect_queue_head,
                                                                              &redirect_queue_tail, &tcp_occupied, &empty_redirect_queue);
                                        //está à espera da resposta do pop_query
                                        waiting_pop_reply = 1;

                                        if(head_pop_query_list == NULL)
                                        {
                                            head_pop_query_list = newElementQuery(query_id, bestpops);
                                            tail_pop_query_list = head_pop_query_list;
                                        }
                                        else
                                        {
                                            insertTailQuery(query_id, bestpops, &tail_pop_query_list);
                                        }
                                    }
                                }

                            }
                            else
                            {
                                if(flag_d) printf("Mensagem New Pop do par acabado de ligar errada!\n");
                                if(flag_d) printf("O par vai ser desligado\n");
                                redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head,
                                        &redirect_queue_tail, &empty_redirect_queue, NULL, 1);
                                missing -= missing_tqs[i];
                                missing_tqs[i] = 0;
                            }


                            aux_buffer_sons[i][0] = '\0';
                            aux_ptr_sons[i] = aux_buffer_sons[i];
                            buffer[0] = '\0';
                            nread_sons[i] = 0;


                          /*  if(redirect_queue_head == NULL)
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
                            }*/


                        }
                        else if(!strcmp("PR", buffer))
                        {
                            if(flag_d) printf("Mensagem recebida do par a jusante %s:%s: %s\n", getIP(aux), getPORT(aux), aux_buffer_sons[i]);

                            query_id_rcvd = receive_pop_reply(aux_buffer_sons[i], ip_pop, port_pop, &available_sessions);

                            if(query_id_rcvd != -1)
                            {
                                current_pop_query_list = getElementById(head_pop_query_list, &previous_pop_query_list, query_id_rcvd);

                                if(current_pop_query_list != NULL)
                                {
                                    if(getBestPopsQuery(current_pop_query_list) > 0) //Se tiver pops por receber
                                    {
                                        waiting_pop_reply = 0;

                                        decreaseBestPops(current_pop_query_list, available_sessions);

                                        if(getBestPopsQuery(current_pop_query_list) <= 0)
                                        {
                                            removeElementQuery(previous_pop_query_list, current_pop_query_list,
                                                    &head_pop_query_list, &tail_pop_query_list);

                                            freeQueue(aux_pops_queue_head);
                                            aux_pops_queue_aux = NULL;
                                            aux_pops_queue_head = NULL;
                                            aux_pops_queue_tail = NULL;
                                            empty_aux_pops_queue = 1;
                                        }


                                        if(empty_pops_queue)
                                        {
                                            pops_queue_head = newElement(ip_pop, port_pop, available_sessions, -1);
                                            pops_queue_tail = pops_queue_head;
                                            empty_pops_queue = 0;
                                        }
                                        else
                                        {
                                            pops_aux = getElementByAddress(pops_queue_head, ip_pop, port_pop);

                                            if(pops_aux != NULL)
                                            {
                                                setAvailable(pops_aux, available_sessions);
                                            }
                                            else
                                            {
                                                if(insert_tail)
                                                {
                                                    pops_queue_tail = insertTail(ip_pop, port_pop, available_sessions, -1, pops_queue_tail);
                                                    insert_tail = 0;
                                                }
                                                else
                                                {
                                                    insert_tail = 1;
                                                    pops_queue_head = insertHead(ip_pop, port_pop, available_sessions, -1, pops_queue_head);
                                                }
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    if(flag_d) printf("Não existe nenhuma query associada ao query ID recebido\n");
                                }
                            }
                            else
                            {
                                if(flag_d) printf("Mensagem Pop Reply mal formatada\n");
                                if(flag_d) printf("O par que enviou a mensagem será desconectado\n");
                                redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head,
                                                               &redirect_queue_tail, &empty_redirect_queue, NULL, 1);
                                missing -= missing_tqs[i];
                                missing_tqs[i] = 0;
                            }

                            aux_buffer_sons[i][0] = '\0';
                            aux_ptr_sons[i] = aux_buffer_sons[i];
                            buffer[0] = '\0';
                            nread_sons[i] = 0;
                        }
                        else if(!strcmp("TR", buffer) || waiting_tr[i])
                        {

                            aux = getElementByIndex(redirect_queue_head, i);
                            if(flag_d && waiting_tr[i] != 1) printf("Mensagem recebida do par a jusante %s:%s: %s\n", getIP(aux), getPORT(aux), aux_buffer_sons[i]);
                            //Receber o tree reply

                            //está a receber o header, por isso vai construir o primeiro nó
                            if(waiting_tr[i] != 1)
                            {
                                interm_list[i] = construct_interm_list_header(interm_list[i], aux_buffer_sons[i]);
                                interm_tail[i] = interm_list[i];
                                if(interm_list[i] == NULL)
                                {
                                    redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head,
                                            &redirect_queue_tail, &empty_redirect_queue, NULL, 1);
                                    missing -= missing_tqs[i];
                                    missing_tqs[i] = 0;

                                    //Pensar o que fazer aqui
                                }


                                //prepara a leitura do resto
                                aux_buffer_sons[i][0] = '\0';
                                aux_ptr_sons[i] = aux_buffer_sons[i];
                                buffer[0] = '\0';
                                nread_sons[i] = 0;

                                waiting_tr[i] = 1;
                            }



                            while(1)
                            {
                                flag_remove = 0;
                                if(nread_sons[i] >= SONS_BUFFER - 1)
                                {
                                    flag_remove = 1;
                                }
                                nread_sons[i] = tcp_receive2(SONS_BUFFER - nread_sons[i] -1, aux_ptr_sons[i], fd_array[i]);
                                aux_ptr_sons[i] += nread_sons[i];

                                aux = getElementByIndex(redirect_queue_head, i);
                                aux_buffer_sons[i][nread_sons[i]] = '\0';
                                if(flag_d) printf("Mensagem recebida do par a jusante %s:%s: %s\n", getIP(aux), getPORT(aux), aux_buffer_sons[i]);


                                if(flag_remove)
                                {
                                    nread_sons[i] = 0;
                                    aux_ptr_sons[i] = aux_buffer_sons[i];
                                    flag_remove = 0;
                                    if(flag_d)
                                    {
                                        if(getIP(aux) != NULL && getPORT(aux) != NULL)
                                        {
                                            printf("O par %s:%s está a enviar lixo e será desconectado\n", getIP(aux), getPORT(aux));
                                        }
                                        else
                                        {
                                            printf("O par acabado de ligar está a enviar lixo e será desconectado\n");
                                        }
                                    }
                                }

                                if(nread_sons[i] == -1)
                                {
                                    waiting_tr[i] = 0;
                                    break;
                                }
                                else if(nread_sons[i] == 0)
                                {
                                    waiting_tr[i] = 0;
                                    redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head,
                                            &redirect_queue_tail, &empty_redirect_queue, NULL, 1);
                                    missing -= missing_tqs[i];
                                    missing_tqs[i] = 0;
                                    break;
                                }
                                else if(aux_buffer_sons[i][0] == '\n')
                                {
                                    //Vai pegar no interm_list[i] e adicionar um nó à lista print_list
                                    if(head_print == NULL)
                                    {
                                        line = construct_line(interm_list[i]);
                                        head_print = newElementPrint(line);
                                        tail_print = head_print;
                                        free(line);
                                    }
                                    else
                                    {
                                        line = construct_line(interm_list[i]);
                                        tail_print = insertTailPrint(line, tail_print);
                                        free(line);
                                    }

                                    //Fazer free da interm_list[i]
                                    freeIntermList(interm_list[i]);
                                    interm_list[i] = NULL;
                                    interm_tail[i] = NULL;


                                    missing_tqs[i] = missing_tqs[i] -1;
                                    missing--;
                                    // Verifica se foi o úlitmo TR que necessitava de receber. Caso for imprime a lista
                                    if(missing == 0)
                                    {
                                        printf("Estrutura de transmissão em árvore\n");
                                        print_tree(head_print, streamID, redirect_queue_head, ipaddr, tport, tcp_sessions);
                                        freePrintList(head_print);
                                        head_print = NULL;
                                        tree_to_print = 0;
                                    }



                                    waiting_tr[i] = 0;
                                    aux_buffer_sons[i][0] = '\0';
                                    aux_ptr_sons[i] = aux_buffer_sons[i];
                                    buffer[0] = '\0';
                                    nread_sons[i] = 0;
                                    break;
           
                                }
                                else if(aux_buffer_sons[i][nread_sons[i] - 1] == '\n')
                                {
                                    //Se a mensagem estiver mal formatada aqui recebemos NULL
                                    //No entento isso não devia acontecer, porque perdemos o resto da lista e não lhe podemos dar free
                                    //É ainda preciso decrementar missing e pôr missing_tqs[i] = 0
                                    //FIX ME
                                    //Ideia, criar ponteiro auxiliar. Se não for NULL, interm_list[i]=auxiliar
                                    //Se for NULL, liberta-se interm_list[i] e desliga-se o filho
                                    interm_list[i] = construct_interm_list_nodes(interm_list[i], aux_buffer_sons[i], fd_array,
                                            tcp_sessions, &tcp_occupied, &redirect_queue_head, &redirect_queue_tail,
                                            &empty_redirect_queue, &missing, &(interm_tail[i]), i, aux, missing_tqs);


                                    aux_buffer_sons[i][0] = '\0';
                                    aux_ptr_sons[i] = aux_buffer_sons[i];
                                    buffer[0] = '\0';
                                    nread_sons[i] = 0;
                                }
                                else
                                {
                                    break;
                                }

                            }

                        }
                        else
                        {
                            aux = getElementByIndex(redirect_queue_head, i);
                            if(aux != NULL)
                            {
                                if(flag_d) printf("Recebida mensagem mal formatada do par %s:%s\n", getIP(aux), getPORT(aux));
                            }
                            else
                            {
                                if(flag_d) printf("Recebida mensagem mal formatada do par acabado de se ligar\n");
                            }


                            if(flag_d) printf("O par irá ser desligado\n");

                            redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                    &empty_redirect_queue, NULL, 1);
                            missing -= missing_tqs[i];
                            missing_tqs[i] = 0;
                        }

                  /*      aux_buffer_sons[i][0] = '\0';
                        aux_ptr_sons[i] = aux_buffer_sons[i];
                        buffer[0] = '\0';
                        nread_sons[i] = 0;*/
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
                nread_sons[i] = 0;
                aux_buffer_sons[i][0] = '\0';
                aux_ptr_sons[i] = aux_buffer_sons[i];
                flag_remove = 0;
                missing_tqs[i] = 0;

            }
            else if(tcp_sessions == tcp_occupied)
            {
                //Vai à lista de iamroot imediatamente a jusante para procurar um endereço para onde fazer redirect
                if(redirect_aux == NULL)
                {
                    redirect_aux = redirect_queue_head;
                }


                if(redirect_aux != NULL)
                {
                    redirect(fd_tcp_server, getIP(redirect_aux), getPORT(redirect_aux));
                    //Para não redirecionar sempre para o mesmo
                    //Vai "circulando" e quando chega a NULL e volta a este troço, faz-se redirect_aux = redirect_head_queue
                    redirect_aux = getNext(redirect_aux);
                }
                else
                {
                    refuse_fd = tcp_accept(fd_tcp_server);
                    if(refuse_fd == -1)
                    {
                        if(flag_d) printf("Falha ao aceitar o novo par\n");
                    }
                    else
                    {
                        close(refuse_fd);
                        refuse_fd = -1;
                    }
                }
            }
        }

        //Servidor de acessos - fizeram um pedido POPREQ
        if(FD_ISSET(fd_udp, &fd_read_set))
        {
            n = popreq_receive(fd_udp, &addrlen, &addr);

            if(n == 0)
            {
                if(tcp_sessions > tcp_occupied)
                {
                    //Se tiver sessões disponíveis diz para se ligarem a ele próprio
                    popresp(fd_udp, streamID, ipaddr, tport, addrlen, addr);
                    //Não se atualiza aqui o nº de sessões ocupadas. Apenas quando se envia welcome

                    //porquê tcp_sessions == tcp_occupied +1?
                    if(tcp_occupied > 0 && empty_pops_queue && waiting_pop_reply == 0)
                    {
                        if(redirect_queue_head != NULL)
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

                            if(head_pop_query_list == NULL)
                            {
                                head_pop_query_list = newElementQuery(query_id, bestpops);
                                tail_pop_query_list = head_pop_query_list;
                            }
                            else
                            {
                                insertTailQuery(query_id, bestpops, &tail_pop_query_list);
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
                        if(waiting_pop_reply == 0)
                        {
                            if(redirect_queue_head != NULL)
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

                                if(head_pop_query_list == NULL)
                                {
                                    head_pop_query_list = newElementQuery(query_id, bestpops);
                                    tail_pop_query_list = head_pop_query_list;
                                }
                                else
                                {
                                    insertTailQuery(query_id, bestpops, &tail_pop_query_list);
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
                            popresp(fd_udp, streamID, getIP(aux_pops_queue_aux), getPORT(aux_pops_queue_aux), addrlen, addr);
                            decreaseAvailableSessions(aux_pops_queue_aux);
                            //Não dá o elemento mais que 5 vezes além do limite
                            if(getAvailableSessions(aux_pops_queue_aux) <= -5)
                            {
                                aux_pops_queue_head = removeElementByAddress(aux_pops_queue_head, &aux_pops_queue_tail,
                                        getIP(aux_pops_queue_aux), getPORT(aux_pops_queue_aux));

                                if(aux_pops_queue_aux == NULL)
                                {
                                    //Lista auxiliar ficou vazia
                                    empty_aux_pops_queue = 1;
                                    aux_pops_queue_head = NULL;
                                    aux_pops_queue_tail = NULL;
                                    aux_pops_queue_aux = NULL;
                                }
                            }



                            aux_pops_queue_aux = getNext(aux_pops_queue_aux);
                        }
                        else
                        {
                            //Em último recurso envia para ele próprio, mesmo sabendo que vai redirecionar
                            popresp(fd_udp, streamID, ipaddr, tport, addrlen, addr);
                        }
                    }
                    else
                    {
                        //Caso ainda haja elementos na lista de pops, indicar o endereço do primeiro
                        popresp(fd_udp, streamID, getIP(pops_queue_head), getPORT(pops_queue_head), addrlen, addr);
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


                                if(tcp_occupied > 0 && waiting_pop_reply == 0)
                                {
                                    if(redirect_queue_head != NULL)
                                    {
                                        query_id++;
                                        redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops,
                                                                              redirect_queue_head, &redirect_queue_tail, &tcp_occupied, &empty_redirect_queue);
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

                                        if(head_pop_query_list == NULL)
                                        {
                                            head_pop_query_list = newElementQuery(query_id, bestpops);
                                            tail_pop_query_list = head_pop_query_list;
                                        }
                                        else
                                        {
                                            insertTailQuery(query_id, bestpops, &tail_pop_query_list);
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


        }

        //O utilizador introduziu um comando de utilizador
        if(FD_ISSET(0, &fd_read_set))
        {
            exit_flag = read_terminal(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied,
                    redirect_queue_head, NULL, NULL, is_flowing);
        }
    }
}

void interface_not_root(int fd_rs, struct addrinfo *res_rs, char* streamID, char *streamIP, char *streamPORT, int *is_root, char* ipaddr, char *uport,
        char *tport, int tcp_sessions, int tcp_occupied, int fd_tcp_server, int *fd_array, int bestpops,
        int fd_pop, char *pop_addr, char *pop_tport, char *rsaddr, char *rsport, int tsecs)
{
    //Select
    int maxfd, counter;
    fd_set fd_read_set;

    //Indica se foi invocado o comando exit
    int exit_flag = 0;

    //PQ e PR
    int query_id = -1;
    int requested_pops;
    int sent_pops = 0;

    //Variáveis que vieram do pop reply
    char popreply_ip[IP_LEN +1];
    char popreply_port[PORT_LEN +1];
    int popreply_avails;
    int query_id_rcvd_aux = 0;

    //Redirects
    struct _queue* redirect_queue_head = NULL;
    struct _queue* redirect_queue_tail = NULL;
    struct _queue* redirect_aux = NULL;
    int empty_redirect_queue = 1; //indica que a queue está vazia quando é igual a 1
    struct _queue* aux = NULL;

    //Mensagens
    char buffer[MAX_BYTES]; buffer[0] = '\0';
    //char *ptr = NULL;
    char msg[MAX_BYTES]; msg[0] = '\0';

    //SF/BS
    int is_flowing = 0;

    //Variáveis que vieram do tree query
    char treequery_ip[IP_LEN +1];
    char treequery_port[PORT_LEN +1];
    int validate_treequery = -1;

    //Dados
    char *data = NULL;
    //char *data_ptr = NULL;
    int data_len;

    //Buffers intermédios para leitura dos filhos
    char **aux_buffer_sons = NULL;
    char **aux_ptr_sons = NULL;
    int *nread_sons = NULL;

    //Ciclos
    int i;

    //Variável de verificação de retorno
    int n;

    //Filho vai ser removido por enviar lixo
    int flag_remove = 0;

    long flowing_reference = time(NULL);
    long now = time(NULL);

    n = buffer_interm_sons(&aux_ptr_sons, &aux_buffer_sons, &nread_sons, tcp_sessions);
    if(n == -1) return;

    struct timeval *timeout = NULL;

    timeout = (struct timeval *)malloc(sizeof(struct timeval));
    if(timeout == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        for(i = 0; i<tcp_sessions; i++)
        {
            free(aux_buffer_sons[i]);
        }
        free(aux_buffer_sons);
        free(aux_ptr_sons);
        free(nread_sons);
        return;
    }

    timeout->tv_sec = SF_TIMEOUT;
    timeout->tv_usec = 0;

    //Buffers intermédios para leitura do pai
    char *aux_buffer_dad = NULL;
    char *aux_ptr_dad = NULL;
    int nread_dad = 0;

    aux_buffer_dad = (char *)malloc(sizeof(char)*DADS_BUFFER);
    if(aux_buffer_dad == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        for(i = 0; i<tcp_sessions; i++)
        {
            free(aux_buffer_sons[i]);
        }
        free(aux_buffer_sons);
        free(aux_ptr_sons);
        free(nread_sons);
        free(timeout);
        return;
    }


    int *waiting_tr = NULL;

    waiting_tr = (int*)malloc(sizeof(int)*tcp_sessions);
    if(waiting_tr == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        for(i = 0; i<tcp_sessions; i++)
        {
            free(aux_buffer_sons[i]);
        }
        free(aux_buffer_sons);
        free(aux_ptr_sons);
        free(nread_sons);
        free(timeout);
        free(aux_buffer_dad);
        return;
    }

    for(i = 0; i<tcp_sessions; i++)
    {
        waiting_tr[i] = 0;
    }

    printlist **tree_reply_head = NULL;
    printlist **tree_reply_tail = NULL;
    printlist *tree_reply_aux = NULL;

    tree_reply_head = (printlist **)malloc(sizeof(printlist*)*tcp_sessions);
    if(tree_reply_head == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        return;
    }

    for(i = 0; i<tcp_sessions; i++) tree_reply_head[i] = NULL;

    tree_reply_tail = (printlist **)malloc(sizeof(printlist*)*tcp_sessions);
    if(tree_reply_tail == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        return;
    }


    aux_buffer_dad[0] = '\0';

    int data_to_read = 0;

    query_list *head_pop_query_list = NULL;
    query_list *tail_pop_query_list = NULL;
    query_list *current_pop_query_list = NULL;
    query_list *previous_pop_query_list = NULL;


    printf("\n\nINTERFACE DE UTILIZADOR\n\n");

    while(1)
    {

        now = time(NULL);
        if(!is_flowing)
        {
            if(now-flowing_reference >= SF_TIMEOUT)
            {
                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                if(flag_d) printf("Não foi recebido SF, passados %d segundos\n\n", SF_TIMEOUT);
                close(fd_pop);
                fd_pop = -1;

                n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                             fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                             pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                             redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 0, query_id_rcvd_aux);
                if(n == 1)
                {
                    free(timeout);
                    free(aux_buffer_dad);
                    freeQueryList(head_pop_query_list);
                    free(waiting_tr);
                    for(i=0; i<tcp_sessions; i++)
                    {
                        freePrintList(tree_reply_head[i]);
                    }
                    free(tree_reply_head);
                    free(tree_reply_tail);
                    return;
                }
                flowing_reference = time(NULL);
            }
        }



        FD_ZERO(&fd_read_set);
        FD_SET(0, &fd_read_set);
        maxfd = 0;
        FD_SET(fd_tcp_server, &fd_read_set);
        maxfd = max(maxfd, fd_tcp_server);
        FD_SET(fd_pop, &fd_read_set);
        maxfd = max(maxfd, fd_pop);
        fd_array_set(fd_array, &fd_read_set, &maxfd, tcp_sessions);

        //Espera por uma mensagem
        counter = select(maxfd + 1, &fd_read_set, (fd_set *)NULL, (fd_set *)NULL, timeout);
        if(counter < 0)
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
                nread_sons[i] = 0;
                aux_buffer_sons[i][0] = '\0';
                aux_ptr_sons[i] = aux_buffer_sons[i];
                flag_remove = 0;
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
                  //  ptr = msg;
                  //  n = tcp_receive(MAX_BYTES, ptr, fd_array[i]);
                  //  aux = getElementByIndex(redirect_queue_head, i);





                    flag_remove = 0;
                    if(nread_sons[i] >= SONS_BUFFER - 1)
                    {
                        flag_remove = 1;
                    }
                    nread_sons[i] = tcp_receive2(SONS_BUFFER - nread_sons[i] -1, aux_ptr_sons[i], fd_array[i]);
                    aux_ptr_sons[i] += nread_sons[i];
                    //ptr = aux_ptr_sons[i];

                    //n = tcp_receive(MAX_BYTES, ptr, fd_array[i]);
                    aux = getElementByIndex(redirect_queue_head, i);

                    if(flag_remove)
                    {
                        nread_sons[i] = 0;
                        aux_ptr_sons[i] = aux_buffer_sons[i];
                        flag_remove = 0;
                        if(flag_d)
                        {
                            if(getIP(aux) != NULL && getPORT(aux) != NULL)
                            {
                                printf("O par %s:%s está a enviar lixo e será desconectado\n", getIP(aux), getPORT(aux));
                            }
                            else
                            {
                                printf("O par acabado de ligar está a enviar lixo e será desconectado\n");
                            }
                        }
                    }

                    if(nread_sons[i] == 0)
                    {
                        redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                       &empty_redirect_queue, NULL, 1);
                    }
                    else if(aux_buffer_sons[i][nread_sons[i] - 1] == '\n')
                    {
                        //Coloca um \0 no fim da mensagem recebida
                        //Se n fosse igual a MAX_BYTES estariamos a apagar o ultimo carater recebido
                       /* if (n < MAX_BYTES) {
                            *(ptr + n - 1) = '\0';
                        }*/
                        aux_buffer_sons[i][nread_sons[i]] = '\0';
                        //Copia os 2 primeiros carateres de msg para buffer
                        strncpy(buffer, aux_buffer_sons[i], 2);
                        buffer[2] = '\0';

                        if(!strcmp("NP", buffer))
                        {
                            redirect_queue_head = receive_newpop(redirect_queue_head, &redirect_queue_tail, i, fd_array,
                                                                 &empty_redirect_queue, aux_buffer_sons[i]);
                            aux = getElementByIndex(redirect_queue_head, i);
                            /*if(flag_d) printf("Mensagem recebida do novo par a jusante: NP %s:%s\n", getIP(aux), getPORT(aux));

                            redirect_queue_head = send_is_flowing(is_flowing, fd_array, i, &tcp_occupied, redirect_queue_head,
                                    aux, &redirect_queue_tail, NULL, &empty_redirect_queue, 1);*/





                            if(getIP(aux) != NULL && getPORT(aux) != NULL)
                            {
                                if(flag_d) printf("Mensagem recebida do novo par a jusante: NP %s:%s\n", getIP(aux), getPORT(aux));
                                //Depois de receber o NP, envia um SF/BS
                                redirect_queue_head = send_is_flowing(is_flowing, fd_array, i, &tcp_occupied, redirect_queue_head, aux,
                                                                             &redirect_queue_tail, NULL, &empty_redirect_queue, 1,
                                                                             NULL, NULL);
                            }
                            else
                            {
                                if(flag_d) printf("Mensagem New Pop do par acabado de ligar errada!\n");
                                if(flag_d) printf("O par vai ser desligado\n");
                                redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head,
                                                               &redirect_queue_tail, &empty_redirect_queue, NULL, 1);
                            }
                        }
                        else if(!strcmp("PR", buffer))
                        {

                            if(flag_d) printf("Mensagem recebida do par a jusante %s:%s: %s\n", getIP(aux), getPORT(aux), aux_buffer_sons[i]);

                            query_id_rcvd_aux = receive_pop_reply(aux_buffer_sons[i], popreply_ip, popreply_port, &popreply_avails);
                            if(query_id_rcvd_aux != -1)
                            {
                                current_pop_query_list = getElementById(head_pop_query_list, &previous_pop_query_list, query_id_rcvd_aux);

                                if(current_pop_query_list != NULL)
                                {
                                    if(getBestPopsQuery(current_pop_query_list) > 0) //Se tiver pops por enviar
                                    {
                                        n = send_pop_reply(query_id_rcvd_aux, popreply_avails, popreply_ip, popreply_port, fd_pop);

                                        if(n == 0)
                                        {
                                            //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                            if(flag_d) printf("Perdida a ligação ao par a montante\n\n");
                                            close(fd_pop);
                                            fd_pop = -1;

                                            n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                                         fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                                         pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                                         redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                                            if(n == 1)
                                            {
                                                free(timeout);
                                                free(aux_buffer_dad);
                                                freeQueryList(head_pop_query_list);
                                                free(waiting_tr);
                                                for(i=0; i<tcp_sessions; i++)
                                                {
                                                    freePrintList(tree_reply_head[i]);
                                                }
                                                free(tree_reply_head);
                                                free(tree_reply_tail);
                                                return;
                                            }
                                            flowing_reference = time(NULL);

                                        }
                                        else if(n == 1)
                                        {
                                            //Se correu bem decrementa o número de pops por enviar
                                            decreaseBestPops(current_pop_query_list, popreply_avails);

                                            if(getBestPopsQuery(current_pop_query_list) <= 0)
                                            {
                                                //Se ficar menor ou igual a 0, remove-se o elemento da lista
                                                removeElementQuery(previous_pop_query_list, current_pop_query_list,
                                                                   &head_pop_query_list, &tail_pop_query_list);
                                            }
                                        }

                                    }
                                }
                                else
                                {
                                    if(flag_d) printf("Não existe nenhum pedido associado ao query ID recebido no Pop Reply\nO pedido pode já ter sido satisfeito\n\n");
                                }
                            }
                            else
                            {
                                if(flag_d) printf("Mensagem Pop Reply do par %s:%s mal formatada\n", getIP(aux), getPORT(aux));
                                if(flag_d) printf("O par vai ser desligado\n");
                                redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head,
                                                               &redirect_queue_tail, &empty_redirect_queue, NULL, 1);
                            }


                        }
                        else if(!strcmp("TR", buffer) || waiting_tr[i])
                        {
                            aux = getElementByIndex(redirect_queue_head, i);
                            if(flag_d) printf("Mensagem recebida do par a jusante %s:%s: %s\n", getIP(aux), getPORT(aux), aux_buffer_sons[i]);
                            //Receber o tree reply

                            if(strcmp("TR", buffer) == 0 && waiting_tr[i])
                            {
                                waiting_tr[i] = 0;


                                freePrintList(tree_reply_head[i]);
                                tree_reply_head[i] = NULL;
                                tree_reply_tail[i] = tree_reply_head[i];
                            }

                            //Quando não está à espera é porque está a receber o primeiro elemento
                            if(waiting_tr[i] == 0)
                            {
                                tree_reply_head[i] = newElementPrint(aux_buffer_sons[i]);
                                tree_reply_tail[i] = tree_reply_head[i];
                                waiting_tr[i] = 1;
                            }
                            else
                            {
                                tree_reply_tail[i] = insertTailPrint(aux_buffer_sons[i], tree_reply_tail[i]);
                            }


                            if(aux_buffer_sons[i][0] == '\n')
                            {
                                //enviar para cima
                                tree_reply_aux = tree_reply_head[i];
                                while(tree_reply_aux != NULL)
                                {
                                    n = tcp_send(strlen(getLinePrint(tree_reply_aux)), getLinePrint(tree_reply_aux), fd_pop);
                                    if(n == 0)
                                    {
                                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                                        if(flag_d) printf("Perdida a ligação ao par a montante\n\n");
                                        close(fd_pop);
                                        fd_pop = -1;

                                        waiting_tr[i] = 0;
                                        freePrintList(tree_reply_head[i]);
                                        tree_reply_head[i] = NULL;
                                        tree_reply_tail[i] = NULL;
                                        tree_reply_aux = NULL;


                                        n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                                     fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                                     pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                                     redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                                        if(n == 1)
                                        {
                                            free(timeout);
                                            free(aux_buffer_dad);
                                            freeQueryList(head_pop_query_list);

                                            free(waiting_tr);
                                            for(i=0; i<tcp_sessions; i++)
                                            {
                                                freePrintList(tree_reply_head[i]);
                                            }
                                            free(tree_reply_head);
                                            free(tree_reply_tail);
                                            return;
                                        }
                                        flowing_reference = time(NULL);
                                        break;
                                    }

                                    tree_reply_aux = getNextPrint(tree_reply_aux);
                                }

                                waiting_tr[i] = 0;
                                freePrintList(tree_reply_head[i]);
                                tree_reply_head[i] = NULL;
                                tree_reply_tail[i] = tree_reply_head[i];
                            }
                        }
                        else
                        {
                            aux = getElementByIndex(redirect_queue_head, i);
                            if(aux != NULL)
                            {
                                if(flag_d) printf("Recebida mensagem mal formatada do par %s:%s\n", getIP(aux), getPORT(aux));
                            }
                            else
                            {
                                if(flag_d) printf("Recebida mensagem mal formatada do par acabado de se ligar\n");
                            }


                            if(flag_d) printf("O par irá ser desligado\n");

                            redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                           &empty_redirect_queue, NULL, 1);
                        }

                        aux_buffer_sons[i][0] = '\0';
                        aux_ptr_sons[i] = aux_buffer_sons[i];
                        buffer[0] = '\0';
                        nread_sons[i] = 0;
                    }




                  /*  ptr = NULL;
                    msg[0] = '\0';
                    buffer[0] = '\0';*/
                }
            }
        }


        //Par TCP a montante
        if(FD_ISSET(fd_pop, &fd_read_set))
        {
            //ptr = msg;
            //n = tcp_receive(MAX_BYTES, ptr, fd_pop);

            if(data_to_read == 0)
            {
                aux_ptr_dad = aux_buffer_dad;
                nread_dad = tcp_receive2(DADS_BUFFER - nread_dad -1, aux_ptr_dad, fd_pop);
                aux_ptr_dad += nread_dad;
                aux_buffer_dad[nread_dad] = '\0';
            }




            if(nread_dad == 0 && data_to_read == 0)
            {
                //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                if(flag_d) printf("Perdida a ligação ao par a montante...\n");
                close(fd_pop);
                fd_pop = -1;

                //Readere à stream
                n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                             fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                             pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux, tsecs,
                             aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                if(n == 1)
                {
                    free(timeout);
                    free(aux_buffer_dad);
                    freeQueryList(head_pop_query_list);
                    free(waiting_tr);
                    for(i=0; i<tcp_sessions; i++)
                    {
                        freePrintList(tree_reply_head[i]);
                    }
                    free(tree_reply_head);
                    free(tree_reply_tail);
                    return;
                }
                flowing_reference = time(NULL);
            }
            else if((nread_dad != 0 && aux_buffer_dad[nread_dad - 1] == '\n') || data_to_read > 0)
            {
                //Coloca um \0 no fim da mensagem recebida
                //Se n fosse igual a MAX_BYTES estariamos a apagar o ultimo carater recebido
               /* if(n < MAX_BYTES)
                {
                    *(ptr + n - 1) = '\0';
                }*/

                //Copia os 2 primeiros caracteres de msg para buffer
                if(data_to_read == 0)
                {
                    strncpy(buffer, aux_buffer_dad, 2);
                    buffer[2] = '\0';
                }

                if(!strcmp("DA", buffer) || data_to_read > 0)
                {
                    //Recebe o header da mensagem
                    if(data_to_read == 0)
                    {
                        n = receive_data_header(&data_len, aux_buffer_dad);
                    }
                    else
                    {
                        n = 0;
                    }


                    if(n == -1)
                    {
                        if(flag_d) printf("Erro no encapsulamento da mensagem\n");
                        if(flag_d) printf("A aplicação irá desligar-se do par a montante\n\n");

                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                        if(flag_d) printf("Perdida a ligação ao par a montante...\n");
                        close(fd_pop);
                        fd_pop = -1;

                        n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                     fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                     pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                     redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                        if(n == 1)
                        {
                            free(timeout);
                            free(aux_buffer_dad);
                            freeQueryList(head_pop_query_list);
                            free(waiting_tr);
                            for(i=0; i<tcp_sessions; i++)
                            {
                                freePrintList(tree_reply_head[i]);
                            }
                            free(tree_reply_head);
                            free(tree_reply_tail);
                            return;
                        }
                        flowing_reference = time(NULL);
                    }
                    if(n != -1)
                    {
                        aux_buffer_dad[0] = '\0';
                        aux_ptr_dad = aux_buffer_dad;
                        nread_dad = 0;
                        buffer[0] = '\0';


                        if(data_to_read > 0)
                        {
                            data_len = data_to_read;
                        }

                        //quantidade de bytes de dados para ler fica em data_to_read
                        //data_len fica com a quantidade a ler agora
                        if(data_len > DADS_BUFFER - 1)
                        {
                            data_to_read = data_len;
                            data_len = DADS_BUFFER - 1;
                        }
                        else
                        {
                            data_to_read = data_len;
                        }

                        //Depois da leitura decrementa-se à quantidade de dados para ler, o número de dados lido
                        nread_dad = read(fd_pop, aux_buffer_dad, data_len);
                        data_to_read -= nread_dad;
                        aux_ptr_dad += nread_dad;




                        /*data = (char *)malloc(sizeof(char)*(data_len+1));
                        if(data == NULL)
                        {
                            if(flag_d) printf("Erro: malloc: %s\n", strerror(errno));
                            continue;
                        }

                        data_ptr = data;
                        n = read(fd_pop, data, data_len);*/
                        if(nread_dad == 0 || nread_dad == -1)
                        {
                            //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                            close(fd_pop);
                            fd_pop = -1;

                            n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                         fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                         pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                         redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                            if(n == 1)
                            {
                                free(timeout);
                                free(aux_buffer_dad);
                                freeQueryList(head_pop_query_list);
                                free(waiting_tr);
                                for(i=0; i<tcp_sessions; i++)
                                {
                                    freePrintList(tree_reply_head[i]);
                                }
                                free(tree_reply_head);
                                free(tree_reply_tail);
                                return;
                            }
                            flowing_reference = time(NULL);
                        }
                        else
                        {
                            aux_buffer_dad[nread_dad] = '\0';

                            if(flag_b)
                            {
                                if(ascii)
                                {
                                    printf("%s", aux_buffer_dad);
                                    fflush(stdout);
                                }
                                else
                                {
                                    print_hex(aux_buffer_dad);
                                    fflush(stdout);
                                }
                            }



                           /* data[n] = '\0';
                            if(flag_b) printf("%s", data_ptr);
                            fflush(stdout);*/

                            sprintf(msg, "DA %04X\n", nread_dad);
                            //ptr = msg;

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
                                    //if(flag_d) printf("Mensagem enviada ao para a jusante %s:%s: %s\n", getIP(aux),
                                     //       getPORT(aux), msg);


                                   // n = tcp_send(data_len, data_ptr, fd_array[i]);
                                    n = tcp_send(nread_dad, aux_buffer_dad, fd_array[i]);
                                    if(n == 0)
                                    {
                                        redirect_queue_head = lost_son(aux, fd_array, i, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                                       &empty_redirect_queue, NULL, 1);
                                        continue;
                                    }
                                    //if(flag_d) printf("Mensagem enviada ao para a jusante %s:%s: %s\n", getIP(aux),
                                          //  getPORT(aux), data_ptr);
                                }
                            }



                        }

                        free(data);
                        data = NULL;
                    }

                    aux_buffer_dad[0] = '\0';
                    aux_ptr_dad = aux_buffer_dad;
                    nread_dad = 0;
                    buffer[0] = '\0';


                }
                else if(!strcmp("BS", buffer))
                {
                    if(flag_d) printf("Mensagem recebida do par a montante: BS\n\n");


                    if(is_flowing == 0)
                    {
                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                        if(flag_d) printf("Foram recebidos dois BS seguidios\n\n");
                        if(flag_d) printf("A desligar do par a montante\n\n");
                        close(fd_pop);
                        fd_pop = -1;

                        n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                     fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                     pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                     redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 0, query_id_rcvd_aux);
                        if(n == 1)
                        {
                            free(timeout);
                            free(aux_buffer_dad);
                            freeQueryList(head_pop_query_list);
                            free(waiting_tr);
                            for(i=0; i<tcp_sessions; i++)
                            {
                                freePrintList(tree_reply_head[i]);
                            }
                            free(tree_reply_head);
                            free(tree_reply_tail);
                            return;
                        }
                        flowing_reference = time(NULL);
                    }
                    else
                    {
                        //Envia broken stream aos pares
                        is_flowing = 0;
                        redirect_queue_head = send_broken_stream_to_all(fd_array, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                                        &empty_redirect_queue);
                    }

                    flowing_reference = time(NULL);
                    aux_buffer_dad[0] = '\0';
                    aux_ptr_dad = aux_buffer_dad;
                    nread_dad = 0;
                    buffer[0] = '\0';

                }
                else if(!strcmp("SF", buffer))
                {
                    if(flag_d) printf("Mensagem recebida do par a montante: SF\n\n");


                    if(is_flowing)
                    {
                        is_flowing = 0;
                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                        if(flag_d) printf("Foram recebidos dois SF seguidos\n\n");
                        if(flag_d) printf("A desligar do par a montante\n\n");
                        close(fd_pop);
                        fd_pop = -1;

                        n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                     fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                     pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux,
                                     tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 0, query_id_rcvd_aux);
                        if(n == 1)
                        {
                            free(timeout);
                            free(aux_buffer_dad);
                            freeQueryList(head_pop_query_list);
                            free(waiting_tr);
                            for(i=0; i<tcp_sessions; i++)
                            {
                                freePrintList(tree_reply_head[i]);
                            }
                            free(tree_reply_head);
                            free(tree_reply_tail);
                            return;
                        }
                        flowing_reference = time(NULL);
                    }
                    else
                    {
                        is_flowing = 1;
                        redirect_queue_head = send_stream_flowing_to_all(fd_array, &tcp_occupied, redirect_queue_head, &redirect_queue_tail,
                                                                         &empty_redirect_queue);
                    }

                    aux_buffer_dad[0] = '\0';
                    aux_ptr_dad = aux_buffer_dad;
                    nread_dad = 0;
                    buffer[0] = '\0';
                }
                else if(!strcmp("PQ", buffer))
                {
                    if(flag_d) printf("Mensagem recebida do par a montante: %s\n", aux_buffer_dad);
                    //Retornar os pontos ip:port e o número de pontos de acessos disponíveis aqui
                    //Se não tiver suficientes fazer pop_query ele próprio
                    n = receive_pop_query(aux_buffer_dad, &requested_pops, &query_id); //Verifica quantos pops lhe foram pedidos e qual o queryID associado

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
                                             pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                             redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                                if(n == 1)
                                {
                                    free(timeout);
                                    free(aux_buffer_dad);
                                    freeQueryList(head_pop_query_list);
                                    free(waiting_tr);
                                    for(i=0; i<tcp_sessions; i++)
                                    {
                                        freePrintList(tree_reply_head[i]);
                                    }
                                    free(tree_reply_head);
                                    free(tree_reply_tail);
                                    return;
                                }
                                flowing_reference = time(NULL);
                            }
                            else if(n == 1)
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


                                if(head_pop_query_list == NULL)
                                {
                                    head_pop_query_list = newElementQuery(query_id, requested_pops - sent_pops);
                                    tail_pop_query_list = head_pop_query_list;
                                }
                                else
                                {
                                    insertTailQuery(query_id, requested_pops - sent_pops, &tail_pop_query_list);
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

                            if(head_pop_query_list == NULL)
                            {
                                head_pop_query_list = newElementQuery(query_id, requested_pops - sent_pops);
                                tail_pop_query_list = head_pop_query_list;
                            }
                            else
                            {
                                insertTailQuery(query_id, requested_pops - sent_pops, &tail_pop_query_list);
                            }

                        }
                    }
                    else
                    {
                        if(flag_d) printf("Mensagem Pop Query recebida do par a montante mal formatada\n");
                        if(flag_d) printf("A aplicação irá desligar-se do par a montante\n");

                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                        close(fd_pop);
                        fd_pop = -1;

                        n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                     fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                     pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                     redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                        if(n == 1)
                        {
                            free(timeout);
                            free(aux_buffer_dad);
                            freeQueryList(head_pop_query_list);
                            free(waiting_tr);
                            for(i=0; i<tcp_sessions; i++)
                            {
                                freePrintList(tree_reply_head[i]);
                            }
                            free(tree_reply_head);
                            free(tree_reply_tail);
                            return;
                        }
                        flowing_reference = time(NULL);
                    }

                    aux_buffer_dad[0] = '\0';
                    aux_ptr_dad = aux_buffer_dad;
                    nread_dad = 0;
                    buffer[0] = '\0';

                }
                else if(!strcmp("TQ", buffer))               
                {                   
                    if(flag_d) printf("Mensagem recebida do par a montante: %s\n", aux_buffer_dad);
                    n = receive_tree_query(aux_buffer_dad, treequery_ip, treequery_port);

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
                                             pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                             redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                                if(n == 1)
                                {
                                    free(timeout);
                                    free(aux_buffer_dad);
                                    freeQueryList(head_pop_query_list);
                                    free(waiting_tr);
                                    for(i=0; i<tcp_sessions; i++)
                                    {
                                        freePrintList(tree_reply_head[i]);
                                    }
                                    free(tree_reply_head);
                                    free(tree_reply_tail);
                                    return;
                                }
                                flowing_reference = time(NULL);
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
                        if(flag_d) printf("Mensagem Tree Query inválida!\n");
                        if(flag_b) printf("A aplicação irá desligar-se do par a montante\n\n");

                        //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                        close(fd_pop);
                        fd_pop = -1;

                        n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                     fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                     pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                     redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                        if(n == 1)
                        {
                            free(timeout);
                            free(aux_buffer_dad);
                            freeQueryList(head_pop_query_list);
                            free(waiting_tr);
                            for(i=0; i<tcp_sessions; i++)
                            {
                                freePrintList(tree_reply_head[i]);
                            }
                            free(tree_reply_head);
                            free(tree_reply_tail);
                            return;
                        }
                        flowing_reference = time(NULL);

                    }

                    aux_buffer_dad[0] = '\0';
                    aux_ptr_dad = aux_buffer_dad;
                    nread_dad = 0;
                    buffer[0] = '\0';

                }
                else
                {
                    if(flag_d) printf("Mensagem mal formatada recebida do par a montante\n");
                    if(flag_d) printf("A aplicação irá desligar-se do par a montante\n\n");

                    //Perdeu-se a ligação ao par a montante, tentar entrar de novo
                    close(fd_pop);
                    fd_pop = -1;

                    n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, &redirect_queue_head, &redirect_queue_tail,
                                 fd_array, &tcp_occupied, tcp_sessions, &empty_redirect_queue, is_root, pop_addr,
                                 pop_tport, &fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops,
                                 redirect_aux, tsecs, aux_buffer_sons, aux_ptr_sons, nread_sons, &is_flowing, 1, query_id_rcvd_aux);
                    if(n == 1)
                    {
                        free(timeout);
                        free(aux_buffer_dad);
                        freeQueryList(head_pop_query_list);
                        free(waiting_tr);
                        for(i=0; i<tcp_sessions; i++)
                        {
                            freePrintList(tree_reply_head[i]);
                        }
                        free(tree_reply_head);
                        free(tree_reply_tail);
                        return;
                    }
                    flowing_reference = time(NULL);
                }
            }
            /*ptr = NULL;
            msg[0] = '\0';
            buffer[0] = '\0';*/
        }

        if(FD_ISSET(0, &fd_read_set))
        {
            exit_flag = read_terminal(fd_rs, res_rs, streamID, *is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied,
                    redirect_queue_head, pop_addr, pop_tport, is_flowing);
            if(exit_flag == 1)
            {
                freeQueue(redirect_queue_head);
                free(nread_sons);
                free(aux_ptr_sons);
                for(i = 0; i<tcp_sessions; i++)
                {
                    free(aux_buffer_sons[i]);
                }
                free(aux_buffer_sons);
                free(timeout);
                free(aux_buffer_dad);
                freeQueryList(head_pop_query_list);
                free(waiting_tr);
                for(i=0; i<tcp_sessions; i++)
                {
                    freePrintList(tree_reply_head[i]);
                }
                free(tree_reply_head);
                free(tree_reply_tail);

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
                    printf("Impossível comunicar com o servidor de raízes, após %d tentativas\n", MAX_TRIES);
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
            if(tcp_occupied == 0)
            {
                printf("Estrutura de transmissão em árvore\n");

                printf("%s\n", streamID);
                printf("%s:%s (%d)\n", ipaddr, tport, tcp_sessions);
            }
            else
            {
                //meter flag global a 1 e na interface_root se tiver flag ativa chamar função q envia tree query aos filhos
                flag_tree = 1;
            }
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
    if (token == NULL || strlen(token) > IP_LEN)
    {
        if (flag_d) printf("Endereço IP inválido\n");
        return redirect_queue_head;
    }
    strcpy(ip_aux, token);
    token = NULL;

    token = strtok(NULL, "\n");
    if (token == NULL || strlen(token) > PORT_LEN)
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

/*queue *get_data_pop_reply(queue *pops_queue_head, queue **pops_queue_tail, char *ptr, int *empty_pops_queue, int query_id,
        int *received_pops, int waiting_pop_reply, int *correct_info, int *insert_tail)
{
    char ip[IP_LEN + 1];
    char port[PORT_LEN + 1];
    int available_sessions;
    int query_id_rcvd;

    query_id_rcvd = receive_pop_reply(ptr, ip, port, &available_sessions);
    if(query_id_rcvd == -1)
    {
        *correct_info = -1;
        return pops_queue_head;
    }

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
                if(*insert_tail)
                {
                    *pops_queue_tail = insertTail(ip, port, available_sessions, -1, *pops_queue_tail);
                    *insert_tail = 0;
                }
                else
                {
                    *insert_tail = 1;
                    pops_queue_head = insertHead(ip, port, available_sessions, -1, pops_queue_head);
                }
            }
        }
    }
    else
    {
        if(flag_d) printf("Pop reply descartada, pois já foram recebidas todas as pedidas!\n\n");
    }


    return pops_queue_head;   
}*/

//retorna 1 se for para acabar o programa. caso em que passou a ser raiz e depois saiu
//retorna 0 se for para continuar. caso em que nunca chegou a ser raiz
int readesao(struct addrinfo *res_rs, int fd_rs, char *streamID, char *rsaddr, char *rsport, char *ipaddr, char *uport,
        queue **redirect_queue_head, queue **redirect_queue_tail, int *fd_array, int *tcp_occupied, int tcp_sessions,
        int *empty_redirect_queue, int *is_root, char *pop_addr, char *pop_tport, int *fd_pop, char *streamIP,
        char *streamPORT, char *tport, int fd_tcp_server, int bestpops, queue *redirect_aux, int tsecs, char **aux_buffer_sons,
        char **aux_ptr_sons, int *nread_sons, int *is_flowing, int send_broken, int query_id)
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
    int n;
    int counter_tries = 0;
    int counter_global_tries = 0;


    //Envia broken stream
    if(send_broken && *is_flowing == 1)
    {
        *redirect_queue_head = send_broken_stream_to_all(fd_array, tcp_occupied, *redirect_queue_head, redirect_queue_tail,
                                                         empty_redirect_queue);
    }

    *is_flowing = 0;


    /////////////////////////////// Aderir novamente à stream //////////////////////////////////
    //o 1 no fim é para ele sair do programa
    msg_readesao = find_whoisroot(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, 1);

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
            fd_ss = source_server_connect(fd_rs, res_rs, streamIP, streamPORT, streamID);

            ////////////////////// 3. instalar o servidor UDP de acesso de raiz /////////////////////////////////////
            fd_udp = install_access_server(ipaddr, fd_rs, fd_ss, res_rs, &res_udp, uport, fd_array, streamID);

            //Envia SF
            *redirect_queue_head = send_stream_flowing_to_all(fd_array, tcp_occupied, *redirect_queue_head, redirect_queue_tail,
                    empty_redirect_queue);
	        *is_flowing = 1;
		
            //////////////////////////// 4. executar a interface de utilizador //////////////////////////////////////
            interface_root(fd_ss, fd_rs, res_rs, streamID, *is_root, ipaddr, uport, tport, tcp_sessions, *tcp_occupied,
                           fd_udp, fd_tcp_server, fd_array, bestpops, *redirect_queue_head, *redirect_queue_tail,
                           redirect_aux, *empty_redirect_queue, tsecs, rsaddr, rsport, aux_buffer_sons, aux_ptr_sons, nread_sons,
                           *is_flowing, query_id, streamIP, streamPORT);
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
                    counter_tries++;
                    if(counter_tries == MAX_TRIES && fd_udp == -2)
                    {
                        if(flag_d) printf("Falha ao obter um ponto de acesso, após %d tentativas, a terminar a aplicação\n", MAX_TRIES);
                        return 1;
                    }
                    if(fd_udp == -2) sleep(2); //Espera 3 segundos até à nova tentativa, para não sobrecarregar o servidor de acessos
                }while(fd_udp == -2);
                counter_tries = 0;

                if(fd_udp == -1)
                {
                    //falha na comunicação com o servidor de acessos. Podes significar que a raiz saiu
                    //Vamos tentar uma nova readesão
                    n = readesao(res_rs, fd_rs, streamID, rsaddr, rsport, ipaddr, uport, redirect_queue_head, redirect_queue_tail,
                            fd_array, tcp_occupied, tcp_sessions, empty_redirect_queue, is_root, pop_addr, pop_tport,
                            fd_pop, streamIP, streamPORT, tport, fd_tcp_server, bestpops, redirect_aux, tsecs,
                            aux_buffer_sons, aux_ptr_sons, nread_sons, is_flowing, send_broken, query_id);

                    if(n == 0) return 0;
                    else if(n == 1) return 1;
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
                        welcome_flag = 0;
                    }
                    else
                    {
                        //////////////////////////// 3. Aguarda confirmação de adesão /////////////////////////////////////
                        welcome_flag = wait_for_confirmation(pop_addr, pop_tport, fd_rs, res_rs, fd_udp, *fd_pop, streamID);
                    }

                    counter_tries++;
                    if(counter_tries == MAX_TRIES && welcome_flag == 0)
                    {
                        if(flag_d) printf("Falha em ligar-se ao novo para a montante, após %d tentativas, a terminar a aplicação\n", MAX_TRIES);
                        return 1;
                    }
                    if(welcome_flag == 0) sleep(2); //Espera 3 segundos até à nova tentativa, para não sobrecarregar o servidor de acessos
                }

                counter_global_tries++;
                if(counter_global_tries == MAX_TRIES && *fd_pop == -1)
                {
                    if(flag_d) printf("Falha em readerir à stream, após %d tentativas\n", MAX_TRIES);
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
        int *empty_redirect_queue, int *is_flowing, int *fd_array, int *tcp_occupied, int*missing_tqs, int*missing)
{
    int n;
    queue *aux = NULL;
    queue *previous = NULL;
    int index;


    n = read(fd_ss, data, DATA_SIZE - 1);
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
                *missing = *missing - missing_tqs[index];
                missing_tqs[index] = 0;
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
                      queue **redirect_queue_tail, int *empty_redirect_queue, int*missing_tqs, int*missing)
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
            *missing = *missing - missing_tqs[index];
            missing_tqs[index] = 0;
        }
        else
        {
            n = tcp_send(data_len, data, fd_array[index]);
            if(n == 0)
            {
                redirect_queue_head = lost_son(aux, fd_array, index, tcp_occupied, redirect_queue_head, redirect_queue_tail,
                        empty_redirect_queue, previous, 0);
                *missing = *missing - missing_tqs[index];
                missing_tqs[index] = 0;
            }
        }

        previous = aux;
        aux = getNext(aux);
    }

    return redirect_queue_head;
}

queue *send_is_flowing(int is_flowing, int *fd_array, int index, int *tcp_occupied, queue *redirect_queue_head,
        queue *element, queue **redirect_queue_tail, queue *previous, int *empty_redirect_queue, int remove_by_index,
        int *missing_tqs, int *missing)
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
        if(missing != NULL && missing_tqs != NULL)
        {
            *missing = *missing - missing_tqs[index];
            missing_tqs[index] = 0;
        }


    }
    if(is_flowing)
    {
        if(flag_d) printf("Mensagem enviada ao par a jusante %s:%s: SF\n\n", getIP(element), getPORT(element));
    }
  /*  else
    {
        if(flag_d) printf("Mensagem enviada ao par a jusante %s:%s: BS\n\n", getIP(element), getPORT(element));
    }*/




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

int buffer_interm_sons(char ***aux_ptr_sons, char ***aux_buffer_sons, int** nread_sons, int tcp_sessions)
{
    int i;
    int j;

    *aux_ptr_sons = (char **)malloc(sizeof(char*)*tcp_sessions);
    if(*aux_ptr_sons == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        return -1;
    }

    *aux_buffer_sons = (char **)malloc(sizeof(char*)*tcp_sessions);
    if(*aux_buffer_sons == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        free(*aux_ptr_sons);
        return -1;
    }

    for(i = 0; i<tcp_sessions; i++)
    {
        (*aux_buffer_sons)[i] = NULL;
        (*aux_buffer_sons)[i] = (char *)malloc(sizeof(char)*SONS_BUFFER);
        if((*aux_buffer_sons)[i] == NULL)
        {
            for(j = 0; j<i; j++)
            {
                free((*aux_buffer_sons)[j]);
            }
            free(*aux_buffer_sons);
            free(*aux_ptr_sons);
            if(flag_d) fprintf(stdout, "Erro: malloc: %s\n\n", strerror(errno));
            return -1;
        }
        //aux_buffer_sons[i] = '\0';
        strcpy((*aux_buffer_sons)[i], "\0");
        (*aux_ptr_sons)[i] = (*aux_buffer_sons)[i];
    }

    *nread_sons = (int *)malloc(sizeof(int)*tcp_sessions);
    if(*nread_sons == NULL)
    {
        if(flag_d) fprintf(stderr, "Erro: malloc: %s\n\n", strerror(errno));
        for(i = 0; i<tcp_sessions; i++)
        {
            free((*aux_buffer_sons)[i]);
        }
        free(*aux_buffer_sons);
        free(*aux_ptr_sons);
        return -1;
    }

    for(i = 0; i<tcp_sessions; i++)
    {
        (*nread_sons)[i] = 0;
    }

    return 0;
}











