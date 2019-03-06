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

int get_root_access_server(char *rasaddr, char *rasport, char *msg)
{
	char *token = NULL;

	token = strtok(msg, " ");
	token = strtok(NULL, " ");

	token = strtok(NULL, ":");
	if(token == NULL)
	{
		if(flag_d) printf("ipaddr inválido!\n");
		return -1;
	}
	strcpy(rasaddr, token);

	token = strtok(NULL, "\n");
	if(token == NULL)
	{
		if(flag_d) printf("uport inválido!\n");
		return -1;
	}
	strcpy(rasport, token);

	return 0;
}
