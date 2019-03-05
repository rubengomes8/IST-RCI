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
	char *token;



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

				strcpy(streamID, argv1);
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


void arguments_reading(int argc, char *argv[], int has_stream, char ipaddr[], char tport[], char uport[], char rsaddr[], char rsport[], int *tcp_sessions, int *bestpops, int *tsecs, int *flag_h){
	int i=0, n=0;
	char *token = NULL;

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
		if(n + has_stream > argc - 1)
		{
			//printf("Argumentos inválidos!\n");
			//exit(1);
			break;
		}

		if(!strcmp(argv[n + has_stream], "-i"))
		{
			if(n + has_stream + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			strcpy(ipaddr, argv[n + has_stream + 1]);
			if(strlen(ipaddr) > IP_LEN)
			{
				printf("ippaddr tem tamanho superior ao possível!\n");
				exit(1);
			}
			printf("ipaddr: %s\n", ipaddr);
		}
		else if(!strcmp(argv[n + has_stream], "-t"))
		{
			if(n + has_stream + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			strcpy(tport, argv[n + has_stream + 1]);
			if(strlen(tport) > PORT_LEN)
			{
				printf("tport tem tamanho superior ao possível!\n");
				exit(1);
			}
			printf("tport: %s\n", tport);
		}
		else if(!strcmp(argv[n + has_stream], "-u"))
		{
			if(n + has_stream + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			strcpy(uport, argv[n + has_stream + 1]);
			if(strlen(uport) > PORT_LEN)
			{
				printf("uport tem tamanho superior ao possível!\n");
				exit(1);
			}
			printf("uport: %s\n", uport);
		}
		else if(!strcmp(argv[n + has_stream], "-s"))
		{
			if(n + has_stream + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			i = count_specific_char(argv[n + has_stream + 1], ':');
			if(i == 1)
			{
				//split into rsaddr e rsport
				token = strtok(argv[n + has_stream + 1], ":");
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
				strcpy(rsaddr, argv[n + has_stream + 1]);
				printf("rsaddr: %s\n", rsaddr);
			}
			else
			{
				printf("You should type <-s ip[:port]>\n");
				exit(-1);
			}
		}
		else if(!strcmp(argv[n + has_stream], "-p"))
		{
			if(n + has_stream + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			*tcp_sessions = atoi(argv[n + has_stream + 1]);
			if(*tcp_sessions < 1)
			{
				printf("tcpsessions must be at least 1!\n");
				exit(-1);
			}
			printf("tcp_sessions: %d\n", *tcp_sessions);
		}
		else if(!strcmp(argv[n + has_stream], "-n"))
		{
			if(n + has_stream + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			*bestpops = atoi(argv[n + has_stream + 1]);
			if(*bestpops < 1)
			{
				printf("bestpops must be at least 1!\n");
				exit(-1);
			}
			printf("bestpops: %d\n", *bestpops);
		}
		else if(!strcmp(argv[n + has_stream], "-x"))
		{
			if(n + has_stream + 1> argc - 1)
			{
				printf("Argumentos inválidos!\n");
				exit(1);
			}
			*tsecs = atoi(argv[n + has_stream + 1]);
			if(*tsecs < MIN_TSECS)
			{
				printf("tsecs must be at least 1!\n");
				exit(-1);
			}
			printf("tsecs: %d\n", *tsecs);
		}
		else if(!strcmp(argv[n + has_stream], "-b"))
		{
			flag_b = 0;
			n--;
			printf("b: %d\n", flag_b);
		}
		else if(!strcmp(argv[n + has_stream], "-d"))
		{
			flag_d = 1;
			n--;
			printf("d: %d\n", flag_d);
		}
		else if(!strcmp(argv[n + has_stream], "-h"))
		{
			*flag_h = 1;
			n--;
			printf("h: %d\n", *flag_h);
		}
		else
		{
			printf("Usage of incorrect flags!\n"); // talvez imprimir o metodo e invocação
			exit(-1);
		}
	}

}
