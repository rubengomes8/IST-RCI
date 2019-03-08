 #include "utils.h"

extern int flag_b;
extern int flag_d;


void sinopse(){
	printf("A aplicação iamroot é invocada da seguinte forma:\n\n");
	printf("./iamroot [<streamID>] [-i <ipaddr>] [-t <tport>] [-u <uport>]\n");
	printf("[-s <rsaddr>[:<rsport>]] [-p <tcpsessions>] [-n <bestpops>] [-x <tsecs>]\n");
	printf("[-b] [-d] [-h]\n\n");

	printf("+------------------------------------------------------------------------------------------------------------------+\n");
	printf("| <streamID>: identificação da stream <nome>:<source_ipaddr>:<source_tport>                                        |\n");
	printf("| <ipaddr>: endereço IP da interface de rede usada pela aplicação                                                  |\n");
	printf("| <tport>: porto TCP onde a aplicação aceita sessões de outros pares a jusante (58000 por omissão)                 |\n");
	printf("| <uport>: porto UDP do servidor de acesso (58000 por omissão)                                                     |\n");
	printf("| <rsaddr>: endereço IP do servidor de raízes (193.136.138.142 por omissão)                                        |\n");
	printf("| <rsport>: porto UDP do servidor de raízes (59000 por omissão)                                                    |\n");
	printf("| <tcpsessions>: nº de sessões TCP que a aplicação aceitará para ligação de pares a jusante (>=1)                  |\n");
	printf("| <bestpops>: nº de pontos de acesso a recolher por esta aplicação, quando raiz, durante a descoberta de pops (>=1)|\n");
	printf("| <tsecs>: período em segundos associado ao registo periódico que a raiz fará no servidor de raízes (5 por omissão)|\n");
	printf("| -b: desativa a apresentação de dados do stream na interface                                                      |\n");
	printf("| -d: usada para a aplicação apresentar informação detalhada do seu funcionamento                                  |\n");
	printf("| -h: força a aplicação a apresentar a sinopse da linha de comandos                                                |\n");
	printf("+------------------------------------------------------------------------------------------------------------------+\n");

}

int count_specific_char(char *string, char ch){
	int count = 0;
	int i;

	int length = strlen(string);

	for(i = 0; i < length; i++)
	{
		if(string[i] == ch)
		{
			count++;
		}
	}
	return count;
}


//Retorna 0 caso a stream não tenha sido especificada ou tenha sido mal especificada
//Retorna 1 caso haja especificação válida da stream
int validate_stream(int argc, char *argv1, char* streamID, char* streamNAME, char *streamIP, char* streamPORT)
{
	int n;
	char *token = NULL;

	//Só para ver se há stream ou não
	if(argc > 1)
	{
		if(strlen(argv1) > STREAM_ID_SIZE)
		{
			//Certifica-se que o o streamID tem dimensões válidas
			printf("O ID da stream inserido tem tamanho superior ao possível!\n");
			return 0;
		}
		else if(strlen(argv1) > 2)
		{
			//Houve especificação da stream
			n = count_specific_char(argv1, ':');
			if(n == 2)
			{
				strcpy(streamID, argv1);

				//Passa o streamID para minúsculas
				stream_id_to_lowercase(streamID);

				token = strtok(streamID, ":");
				if(token == NULL)
				{
					printf("streamID inválido!\n");
					exit(1);
				}
				if(strlen(token) > STREAM_NAME_LEN)
				{
					printf("O nome da stream especificada tem tamanho superior ao possível!\n");
					exit(1);
				}
				else
				{
					strcpy(streamNAME, token);
				}

				token = strtok(NULL, ":");
				if(token == NULL)
				{
					printf("streamIP inválido!\n");
					exit(1);
				}
				if(strlen(token) > IP_LEN)
				{
					printf("O endereço de IP da stream especificada tem tamanho superior ao possível!\n");
					exit(1);
				}
				else
				{
					strcpy(streamIP, token);
				}

				token = strtok(NULL, "");
				if(token == NULL)
				{
					printf("streamPort inválido!\n");
					exit(1);
				}
				if(strlen(token) > PORT_LEN)
				{
					printf("O porto da stream especificada tem tamanho superior ao possível!\n");
					exit(1);
				}
				else
				{
					strcpy(streamPORT, token);
				}

				//token = NULL;

				strcpy(streamID, argv1);

				//Passa o streamID para minúsculas
				stream_id_to_lowercase(streamID);
				printf("streamID: %s\n", streamID);
				printf("streamName: %s\n", streamNAME);
				printf("streamIP: %s\n", streamIP);
				printf("streamPort: %s\n", streamPORT);

				return 1;
			}
			else
			{
				printf("stream ID format: <Name:IPAddress:Port\n");
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}


int arguments_reading(int argc, char *argv[], char ipaddr[], char tport[], char uport[], char rsaddr[], char rsport[],
		int *tcp_sessions, int *bestpops, int *tsecs, int *flag_h, char *streamID, char *streamNAME, char *streamIP, char *streamPORT)
{
	int i=0, n=0;
	char *token = NULL;
	int has_stream = 0;

	//Validação dos argumentos com base no seu número
/*	if(has_stream == 1 && argc%2 != 0)
	{
		printf("%d\n", argc%2);
		printf("Argumentos inválidos!\n");
		exit(1);
	}
	else if(has_stream == 0 && argc%2 == 0)
	{
		printf("Argumentos inválidos!\n");
		exit(1);
	}*/

	//Se houver stream começa a ler do argv[3], se não do argv[2]
	//Acho que isto dá problema se executarmos com uma das flags isoladas (b, d ou h) antes de uma das outras.
	for (n = 1; n < argc; n+=2)
	{
		if(n > argc - 1)
		{
			//printf("Argumentos inválidos!\n");
			//exit(1);
			break;
		}

		if(!strcmp(argv[n], "-i"))
		{
			if(n + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			strcpy(ipaddr, argv[n + 1]);
			if(strlen(ipaddr) > IP_LEN)
			{
				printf("ippaddr tem tamanho superior ao possível!\n");
				exit(1);
			}
			printf("ipaddr: %s\n", ipaddr);
		}
		else if(!strcmp(argv[n], "-t"))
		{
			if(n + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			strcpy(tport, argv[n + 1]);
			if(strlen(tport) > PORT_LEN)
			{
				printf("tport tem tamanho superior ao possível!\n");
				exit(1);
			}
			printf("tport: %s\n", tport);
		}
		else if(!strcmp(argv[n], "-u"))
		{
			if(n + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			strcpy(uport, argv[n + 1]);
			if(strlen(uport) > PORT_LEN)
			{
				printf("uport tem tamanho superior ao possível!\n");
				exit(1);
			}
			printf("uport: %s\n", uport);
		}
		else if(!strcmp(argv[n], "-s"))
		{
			if(n + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			i = count_specific_char(argv[n + 1], ':');
			if(i == 1)
			{
				//split into rsaddr e rsport
				token = strtok(argv[n + 1], ":");
				if(token == NULL)
				{
					printf("rsaddr inválido!\n");
					exit(1);
				}
				if(strlen(token) > IP_LEN)
				{
					printf("rsaddr tem tamanho superior ao possível!\n");
					exit(1);
				}
				else
				{
					strcpy(rsaddr, token);
				}

				token = strtok(NULL, "");
				if(token == NULL)
				{
					printf("rsport inválido!\n");
					exit(1);
				}
				if(strlen(token) > PORT_LEN)
				{
					printf("rsport tem tamanho superior ao possível!\n");
					exit(1);
				}
				else
				{
					strcpy(rsport, token);
				}

				printf("rsaddr: %s\n", rsaddr);
				printf("rsport: %s\n", rsport);
			}
			else if (i == 0)
			{
				strcpy(rsaddr, argv[n +1]);
				printf("rsaddr: %s\n", rsaddr);
			}
			else
			{
				printf("You should type <-s ip[:port]>\n");
				exit(-1);
			}
		}
		else if(!strcmp(argv[n], "-p"))
		{
			if(n + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			*tcp_sessions = atoi(argv[n + 1]);
			if(*tcp_sessions < 1)
			{
				printf("tcpsessions must be at least 1!\n");
				exit(-1);
			}
			printf("tcp_sessions: %d\n", *tcp_sessions);
		}
		else if(!strcmp(argv[n], "-n"))
		{
			if(n + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			*bestpops = atoi(argv[n + 1]);
			if(*bestpops < 1)
			{
				printf("bestpops must be at least 1!\n");
				exit(-1);
			}
			printf("bestpops: %d\n", *bestpops);
		}
		else if(!strcmp(argv[n], "-x"))
		{
			if(n + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			*tsecs = atoi(argv[n + 1]);
			if(*tsecs < MIN_TSECS)
			{
				printf("tsecs must be at least 1!\n");
				exit(-1);
			}
			printf("tsecs: %d\n", *tsecs);
		}
		else if(!strcmp(argv[n], "-b"))
		{
			flag_b = 0;
			n--;
			printf("b: %d\n", flag_b);
		}
		else if(!strcmp(argv[n], "-d"))
		{
			flag_d = 1;
			n--;
			printf("d: %d\n", flag_d);
		}
		else if(!strcmp(argv[n], "-h"))
		{
			*flag_h = 1;
			n--;
			printf("h: %d\n", *flag_h);
		}
		else
		{
			has_stream = validate_stream(argc, argv[n], streamID, streamNAME, streamIP, streamPORT);
			n--;
		}
	}

	return has_stream;
}

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


void stream_id_to_lowercase(char *streamID)
{
	int len, i;

	len = strlen(streamID);

	for(i = 0; i<len; i++)
	{
		//Quando chegar aos dois pontos pára, porque o que vem a seguir são números
		if(streamID[i] == ':')
		{
			return;
		}

		//Converte todos os caracteres não numéricos em minúsculas
		if(streamID[i] >= 'A' && streamID[i] <= 'Z')
		{
			streamID[i] = streamID[i] - ('A' - 'a');
		}
	}

	return;
}

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
			printf("ipaddr inválido!\n");
			printf("Falha em obter o IP do servidor de acesso...\n");
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
			printf("uport inválido!\n");
			printf("Falha em obter o porto do servidor de acesso...\n");
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

int get_redirect(char *pop_addr, char *pop_tport, char *msg)
{
	char *token = NULL;

	
	token = strtok(msg, " ");

	token = strtok(NULL, ":");
	if(token == NULL)
	{
		if(flag_d) printf("ipaddr inválido!\n");
		return -1;
	}
	strcpy(pop_addr, token);

	token = strtok(NULL, "\n");
	if(token == NULL)
	{
		if(flag_d) printf("tport inválido!\n");
		return -1;
	}
	strcpy(pop_tport, token);

	return 0;
}


void free_and_close( int is_root, int fd_rs, int fd_udp, int fd_pop, int fd_ss, struct addrinfo *res_rs, struct addrinfo *res_udp, struct addrinfo *res_pop, struct addrinfo *res_ss, int *fd_array){

	if(fd_rs != -1) close(fd_rs);
    if(res_rs != NULL) freeaddrinfo(res_rs);
    if(fd_udp != -1) close(fd_udp);
    if(res_udp != NULL) freeaddrinfo(res_udp);
    if(is_root)
    {
    	if(fd_ss != -1) close(fd_ss);
   		if(res_ss != NULL) freeaddrinfo(res_ss);  
    }
    else
    {
    	if(fd_pop != -1) close(fd_pop);
   		if(res_pop != NULL) freeaddrinfo(res_pop);  
    }   

    if(fd_array != NULL) free(fd_array);
}

/////////////////////////////////////// Funções para coisas que antes estavam no iamroot ////////////////////////////////
//Se calhar não é o melhor sítio

int source_server_connect(int fd_rs, struct addrinfo *res_rs, struct addrinfo *res_ss, char *streamIP, char *streamPORT)
{
	int fd_ss = -1;

    if(flag_d)
    {
        printf("A estabelecer ligação TCP com o servidor fonte, no endereço %s:%s...\n", streamIP, streamPORT);
    }

    fd_ss = tcp_socket_connect(streamIP, streamPORT);
    if(fd_ss == -1)
    {
        if(flag_d) printf("A aplicação irá terminar...\n");
        if(fd_rs != -1) close(fd_rs);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(res_ss != NULL) freeaddrinfo(res_ss);
        exit(0);
    }

    if(flag_d)
    {
        printf("Ligação com o servidor fonte estabelecida com sucesso!\n");
    }

    return fd_ss;
}

int install_tcp_server(char *tport, int fd_rs, struct addrinfo *res_rs, int fd_ss, struct addrinfo *res_ss, char *ipaddr, int tcp_sessions)
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
		if(flag_d) printf("A aplicação irá terminar...\n");
		if(fd_rs != -1) close(fd_rs);
		if(res_rs != NULL) freeaddrinfo(res_rs);
		if(fd_ss != -1) close(fd_ss);
		if(res_ss != NULL) freeaddrinfo(res_ss);
		exit(0);
	}

	if(flag_d)
	{
		printf("Servidor TCP para comunicação a jusante instalado com sucesso, no endereço %s:%s\n", ipaddr, tport);
	}

	return fd_tcp_server;
}

int *create_fd_array(int tcp_sessions, int fd_rs, int fd_ss, struct addrinfo *res_rs, struct addrinfo *res_ss)
{
	int *fd_array;

	fd_array = fd_array_init(tcp_sessions);
	if(fd_array == NULL)
	{
		if(fd_rs != -1) close(fd_rs);
		if(fd_ss != -1) close(fd_ss);
		if(res_rs != NULL) freeaddrinfo(res_rs);
		if(res_rs != NULL) freeaddrinfo(res_rs);
		exit(0);
	}

	return fd_array;
}

int install_access_server(int fd_rs, int fd_ss, struct addrinfo *res_rs, struct addrinfo *res_ss,
		struct addrinfo **res_udp, char *uport, int *fd_array)
{
	int fd_udp = -1;

	fd_udp = udp_socket(NULL, uport, res_udp);
	if(fd_udp == -1)
	{
		if(flag_d) printf("A aplicação irá terminar...\n");
		if(fd_rs != -1) close(fd_rs);
		if(res_rs != NULL) freeaddrinfo(res_rs);
		if(fd_ss != -1) close(fd_ss);
		if(res_ss != NULL) freeaddrinfo(res_ss);
		if(*res_udp != NULL) freeaddrinfo(*res_udp);
		if(fd_array != NULL) free(fd_array);
		exit(0);
	}

	if(udp_bind(fd_udp, *res_udp) == -1)
	{
		if(flag_d) printf("A aplicação irá terminar...\n");
		if(fd_rs != -1) close(fd_rs);
		if(res_rs != NULL) freeaddrinfo(res_rs);
		if(fd_ss != -1) close(fd_ss);
		if(res_ss != NULL) freeaddrinfo(res_ss);
		if(fd_udp != -1) close(fd_udp);
		if(*res_udp != NULL) freeaddrinfo(*res_udp);
		if(fd_array != NULL) free(fd_array);
		exit(0);
	}

	return fd_udp;
}

int get_access_point(char *rasaddr, char *rasport, struct addrinfo **res_udp, int fd_rs, struct addrinfo *res_rs,
 char *pop_addr, char *pop_tport)
{
	int fd_udp = -1;
	int counter = 0;

	if(flag_d) printf("A ligar-se o servidor de acessos, no endereço %s:%s\n", rasaddr, rasport);
	fd_udp = udp_socket(rasaddr, rasport, res_udp);
	if(fd_udp == -1)
	{
		if(flag_d)
		{
			printf("Falha ao ligar ao servidor de acesso...\n");
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
			if(flag_d)
			{
				printf("\n");
				printf("Impossível comunicar com o servidor de acesso, após %d tentativas...\n", MAX_TRIES);
				printf("A terminar o programa...\n");
			}
			if(fd_rs != -1) close(fd_rs);
			if(res_rs != NULL) freeaddrinfo(res_rs);
			if(fd_udp != -1) close(fd_udp);
			if(*res_udp != NULL) freeaddrinfo(*res_udp);
			exit(0);
		}
	}

	return fd_udp;
}

int connect_to_peer(char *pop_addr, char *pop_tport, int fd_rs, int fd_udp, struct addrinfo *res_rs, struct addrinfo *res_udp, struct addrinfo* res_pop)
{
	int fd_pop = -1;

    if(flag_d) printf("A ligar-se ao par a montante, no endereço %s:%s\n", pop_addr, pop_tport);

    fd_pop = tcp_socket_connect(pop_addr, pop_tport);
    if(fd_pop == -1)
    {
        if(flag_d)
		{
        	printf("Falha ao ligar-se ao par a montante...\n");
        	printf("A aplicação irá terminar...\n");
		}
        if(fd_rs != -1) close(fd_rs);
        if(res_rs != NULL) freeaddrinfo(res_rs);
        if(fd_udp != -1) close(fd_udp);
        if(res_udp != NULL) freeaddrinfo(res_udp);
        if(res_pop != NULL) freeaddrinfo(res_pop);
        exit(0);
    }

    if(flag_d)
    {
        printf("Ligação com o par a montante estabelecida com sucesso!\n");
    }

    return fd_pop;
}

int wait_for_confirmation(char *pop_addr, char *pop_tport, int fd_rs, struct addrinfo *res_rs, int fd_udp, struct addrinfo *res_udp, int fd_pop, struct addrinfo *res_pop, char *streamID)
{
	char *msg = NULL;
	char buffer[BUFFER_SIZE];
	int counter = 0;
	int welcome = 0;

	if(flag_d)
	{
		printf("A tentar comunicar com o peer...\n");
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
				printf("Impossível comunicar com o peer, após %d tentativas...\n", MAX_TRIES);
				printf("A terminar o programa...\n");
			}
			if(flag_d) printf("A aplicação irá terminar...\n");
			if(fd_rs != -1) close(fd_rs);
			if(res_rs != NULL) freeaddrinfo(res_rs);
			if(fd_udp != -1) close(fd_udp);
			if(res_udp != NULL) freeaddrinfo(res_udp);
			if(fd_pop != -1) close(fd_pop);
			if(res_pop != NULL) freeaddrinfo(res_pop);
			exit(0);
		}
		if(flag_d)
		{
			printf("A tentar comunicar com o peer...\n");
		}
		msg = receive_confirmation(fd_pop, msg);
	}

	counter = 0; //Reset do contador, caso tenha sido possível comunicar
	if(flag_d)
	{
		printf("Mensagem recebida pelo peer: %s\n", msg);
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
			//Recebeu um WELCOME mas com a stream incorreta
			//Depois temos de ver melhor qual o progresso do programa nesta situação.
		}
		free(msg);
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
			if(res_udp != NULL) freeaddrinfo(res_udp);
			if(fd_pop != -1) close(fd_pop);
			if(res_pop != NULL) freeaddrinfo(res_pop);
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
			close(fd_pop);
		}
	}

	return welcome;
}

void send_new_pop(int fd_pop, char *ipaddr, char *tport, int fd_rs, struct addrinfo *res_rs, int fd_tcp_server, struct addrinfo* res_tcp, int fd_udp, struct addrinfo *res_udp, int *fd_array)
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
		if(res_tcp != NULL) freeaddrinfo(res_tcp);
		if(fd_udp != -1) close(fd_udp);
		if(res_udp != NULL) freeaddrinfo(res_udp);
		exit(0);
	}
}





















