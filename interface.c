#include "interface.h"
#include "utils.h"

extern int flag_d;
extern int flag_b;
extern int ascii;

#define MAX_BYTES 256

//Fazer verificação se a queue não está vazia antes de tentar aceder
//Ver no programa os sítios que fazem exit() e mudar/certificar que a saída do programa é suave, ou seja,
//com libertação de memória e tudo o resto


void interface_root(int fd_ss, int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char * ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied, int fd_udp, int fd_tcp_server, int *fd_array, int bestpops)
{
    int maxfd, counter;
    fd_set fd_read_set;
    int exit_flag = 0;
    int i;
    int n;
    int query_id = 0;

    //Variáveis para lista de apps iamroot ligadas imadiatamente a jusante
    //Vão formar uma lista utilizada para redirects
    struct _queue* redirect_queue_head = NULL;
    struct _queue* redirect_queue_tail = NULL;
    struct _queue* redirect_aux = NULL;
    int empty_redirect_queue = 1; //indica que a queue está vazia quando é igual a 1


    //Queue de pops. A raiz nunca faz parte da lista
    struct _queue *pops_queue_head = NULL;
    struct _queue *pops_queue_tail = NULL;
    int empty_pops_queue = 1;


    char msg[MAX_BYTES]; msg[0] = '\0';
    char *ptr = NULL;
    char buffer[MAX_BYTES]; buffer[0] = '\0';

    int waiting_pop_reply = 0;
    int received_pops = 0;


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
        fd_array_set(fd_array, &fd_read_set, &maxfd, tcp_sessions);

        //Espera que 1 ou mais file descriptors estejam prontos
        counter = select(maxfd + 1, &fd_read_set, (fd_set *)NULL, (fd_set *)NULL,  (struct timeval *)NULL);
        if(counter <= 0)
        {
            if(flag_d) fprintf(stderr, "Error: select: %s\n", strerror(errno));
            return;
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

                        //recebeu um POP_REPLY
                        if(!strcmp("PR", buffer))
                        {
                            if(flag_d) printf("Mensagem recebida do para a jusante com índice %d: %s\n", i, ptr);
                            pops_queue_head = get_data_pop_reply(pops_queue_head, &pops_queue_tail, ptr, &empty_pops_queue, query_id, &received_pops, waiting_pop_reply);
                            if(received_pops >= bestpops)
                            {
                                waiting_pop_reply = 0;
                                received_pops = 0;
                            }



                            //VERIFICAR QUERY ID
                        }
                        //recebeu um TREE_REPLY
                        else if(!strcmp("TR", buffer))
                        {
                            if(flag_d) printf("Mensagem recebida do para a jusante com índice %d: %s\n", i, ptr);
                            //Receber TR
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
                redirect_queue_head = receive_newpop(redirect_queue_head, &redirect_queue_tail, i, fd_array, &empty_redirect_queue);
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
                    query_id++; //prinft("query_id: %d", query_id);
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

                                //Se a lista de pops estiver vazia, faz pop_query
                                query_id++;
                                redirect_queue_head = pop_query_peers(tcp_sessions, fd_array, query_id, bestpops, redirect_queue_head,
                                                                      &redirect_queue_tail);
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
            exit_flag = read_terminal(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied);
            if(exit_flag == 1)
            {
                freeQueue(redirect_queue_head);
                freeQueue(pops_queue_head);
                return;
            }
        }
    }
}

void interface_not_root(int fd_rs, struct addrinfo *res_rs, char* streamID, int is_root, char* ipaddr, char *uport,
        char *tport, int tcp_sessions, int tcp_occupied, int fd_udp, int fd_tcp_server, int *fd_array, int bestpops,
        int fd_pop)
{
    int maxfd, counter;
    fd_set fd_read_set;
    int exit_flag = 0;
    int i;
    int n;

    int query_id = 0;
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

    //Variáveis que vieram do tree query
    char treequery_ip[IP_LEN +1];
    char treequery_port[PORT_LEN +1];
    int validate_treequery = -1;



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

                        //POP_REPLY
                        if(!strcmp("PR", buffer))
                        {
                            query_id_rcvd_aux = receive_pop_reply(ptr, popreply_ip, popreply_port, &popreply_avails);
                            if(requested_pops - sent_pops > 0) //Se tiver pops por enviar
                            {
                                if(flag_d) printf("Mensagem recebida do para a jusante com índice %d: %s\n", i, ptr);
                                //Dúvida - Não sei se falta fazer um malloc ao ptr!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                                query_id_rcvd = query_id_rcvd_aux;
                                //Receber os dados do pop reply vindos de jusante

                                if(query_id == query_id_rcvd) //Verifica o queryID recebido na reply é o mesmo que o da última query
                                {
                                    //Responder para cima
                        
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
                                else
                                {
                                    //Se o query_id fôr diferente ignora e não envia nada para cima pq não é o mais atual logo significa que a root já tinha feito outro pedido
                                }
                            }
                            else
                            {
                                if(flag_d) printf("Pop reply descartada, pois já tinham sido recebidos todos os pops pedidos!\n");
                            }
                            


                            /******************************************************************************************************
                            variaveis: 
                            sent_pops
                            query_id

                                receber um pop-reply de baixo:
                                    Verifica se já enviou todos:
                                        se já: não faz nada
                                        se não: vem pra baixo
                                            Tem de ver se tem o query_id_rcvd > atual
                                                Se fôr > : ignora e não envia para cima
                                                Se não fôr, envia pra cima 
w
                            ******************************************************************************************************/
                            
                            //Ver se recebeu todos os requested_pops
                            //Enviar os que recebeu
                            //Ter flag a dizer se ainda falta receber ou se já chegaram todos os pedidos waiting_pop_reply = 0;


                            


                        }
                            //TREE_REPLY
                        else if(!strcmp("TR", buffer))
                        {
                            if(flag_d) printf("Mensagem recebida do para a jusante com índice %d: %s\n", i, ptr);
                            //Receber TR
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
                //Fazer nova adesão
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

                if(!strcmp("PQ", buffer))
                {

                    /*****************************************************************************************************************

                    receber um pop-query de cima com um nº bestpops:  
                                    
                    recebeu novo pedido logo atualiza a variavel queryID e inicializa a variavel sent_pops 
                    
                            Tem de ver se tem o número de pops para responder
                                Se tiver: envia pop reply, se não faz popquery (e pop reply) 
                        

                    ******************************************************************************************************************/

                    if(flag_d) printf("Mensagem recebida do par a montante: %s\n", ptr);
                    //Retornar os pontos ip:port e o número de pontos de acessos disponíveis aqui
                    //Se não tiver suficientes fazer pop_query ele próprio
                    receive_pop_query(ptr, &requested_pops, &query_id); //Verifica quantos pops lhe foram pedidos e qual o queryID associado
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
                        }
                        else if(n == -1)
                        {
                            if(flag_d) printf("Falha ao comunicar com o peer a montante...\n");
                        }
                        else
                        {
                            sent_pops =  sent_pops + (tcp_sessions - tcp_occupied); //Atualiza se a comunicação correu bem
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
                            //Fazer nova adesão
                        }
                        else if(n == -1)
                        {
                            if(flag_d) printf("Falha ao comunicar com o peer a montante...\n");
                        }
                        else
                        {         
                            sent_pops = sent_pops + (tcp_sessions - tcp_occupied); //Atualiza se a comunicação correu bem
                        }
                        
                    }
                    else
                    {
                        requested_pops -= tcp_sessions - tcp_occupied;

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
                else if(!strcmp("TQ", buffer))               
                {                   
                    if(flag_d) printf("Mensagem recebida do par a montante: %s\n", ptr);
                    receive_tree_query(ptr, treequery_ip, treequery_port);
                    validate_treequery = compare_ip_and_port(treequery_ip, treequery_port, ipaddr, tport);

                    if(validate_treequery == 0) //O ip e o porto foram bem especificados
                    {
                        //Responder com um TREE_REPLY com capacidade e ocupação do ponto de acesso
                        //Dúvida - ainda não percebi muito bem  de quem é o porto e o ip que vai na mensagem tree reply !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        //send_tree_reply();
                       


                    }
                    else //Caso contrário deverá replicar a mensagem a jusante a menos que não tenha pares a jusante
                    {
                        //Percorrer a lista redirect e enviar para cada um deles a mesma mensagem -> send_tree_query
                    }
                }
            }
            ptr = NULL;
            msg[0] = '\0';
            buffer[0] = '\0';
        }


        //Aceita ou redireciona novas ligações
        if(FD_ISSET(fd_tcp_server, &fd_read_set))
        {
            if(flag_d) printf("Novo pedido de conexão...\n");
            if(tcp_sessions > tcp_occupied)
            {
                i = welcome(tcp_sessions, &tcp_occupied, fd_tcp_server, fd_array, streamID);
                redirect_queue_head = receive_newpop(redirect_queue_head, &redirect_queue_tail, i, fd_array,
                        &empty_redirect_queue);
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

        if(FD_ISSET(0, &fd_read_set))
        {
            exit_flag = read_terminal(fd_rs, res_rs, streamID, is_root, ipaddr, uport, tport, tcp_sessions, tcp_occupied);
            if(exit_flag == 1)
            {
                freeQueue(redirect_queue_head);
                return;
            }
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

int welcome(int tcp_sessions, int *tcp_occupied, int fd_tcp_server, int *fd_array, char *streamID)
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

    return i;
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

queue *receive_newpop(queue *redirect_queue_head, queue **redirect_queue_tail, int i, int *fd_array, int *empty_queue)
{
    char buffer[MAX_BYTES]; buffer[0] = '\0';
    char msg[MAX_BYTES]; msg[0] = '\0';
    char *ptr = NULL;
    char *token = NULL;
    char ip_aux[IP_LEN + 1]; ip_aux[0] = '\0';
    char port_aux[PORT_LEN + 1]; port_aux[0] = '\0';
    int n;


    //Recebe mensagem de fd_array[i]
    ptr = msg;
    n = tcp_receive(MAX_BYTES, ptr, fd_array[i]);
    if (n == -1)
    {
        //Significa que houve um problema na leitura
        return redirect_queue_head;
    }
    else if (n == 0)
    {
        //Ligação perdida
        close(fd_array[i]);
        fd_array[i] = -1;
        redirect_queue_head = removeElementByIndex(redirect_queue_head, redirect_queue_tail, i);
        return redirect_queue_head;
    }


    strncpy(buffer, msg, 2);
    buffer[2] = '\0';
    if (!strcmp(buffer, "NP"))
    {
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

        printf("ip %s, interface.c função receive_new_pop\n", ip_aux);
        printf("port %s\n", port_aux);

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
                if(flag_d) printf("Pop query enviado ao par com índice %d\n", i);
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





