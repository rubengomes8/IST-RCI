#include "utils.h"

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


void arguments_reading(int argc, char *argv[], int has_stream, char ipaddr[], char tport[], char uport[], char rsaddr[], char rsport[], int *tcp_sessions, int *bestpops, int *tsecs, int *flag_b, int *flag_d, int *flag_h){
	int i=0,n=0;
	char *token;
	//Se houver stream começa a ler do argv[3], se não do argv[2]
	//Acho que isto dá problema se executarmos com uma das flags isoladas (b, d ou h) antes de uma das outras.
	for (n = 1; n < argc - 1; n+=2)
	{
		if(!strcmp(argv[n + has_stream], "-i"))
		{
			strcpy(ipaddr, argv[n + has_stream + 1]);
			printf("ipaddr: %s\n", ipaddr);
		}
		else if(!strcmp(argv[n + has_stream], "-t"))
		{
			strcpy(tport, argv[n + has_stream + 1]);
			printf("tport: %s\n", tport);
		}
		else if(!strcmp(argv[n + has_stream], "-u"))
		{
			strcpy(uport, argv[n + has_stream + 1]);
			printf("uport: %s\n", uport);
		}
		else if(!strcmp(argv[n + has_stream], "-s"))
		{
			i = count_specific_char(argv[n + has_stream + 1], ':');
			if(i == 1)
			{
				//split into rsaddr e rsport
				token = strtok(argv[n + has_stream + 1], ":");
				strcpy(rsaddr, token);
				while(token != NULL)
				{
					token = strtok(NULL, ":");
					if(token != NULL)
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
			*tcp_sessions = atoi(argv[n + has_stream + 1]);
			if(*tcp_sessions >= 1)
				break;
			else
			{
				printf("tcpsessions must be at least 1!\n");
				exit(-1);
			}
		}
		else if(!strcmp(argv[n + has_stream], "-n"))
		{
			*bestpops = atoi(argv[n + has_stream + 1]);
			if(*bestpops >= 1)
				break;
			else
			{
				printf("bestpops must be at least 1!\n");
				exit(-1);
			}
		}
		else if(!strcmp(argv[n + has_stream], "-x"))
		{
			*tsecs = atoi(argv[n + has_stream + 1]);
			if(*tsecs >= 1) //Estou a exigir que o período seja pelo menos 1 segundo mas podemos mudar este valor
				break;
			else
			{
				printf("tsecs must be at least 1!\n");
				exit(-1);
			}
		}
		else if(!strcmp(argv[n + has_stream], "-b"))
		{
			*flag_b = 0;
			n--;
		}
		else if(!strcmp(argv[n + has_stream], "-d"))
		{
			*flag_d = 1;
			n--;
		}
		else if(!strcmp(argv[n + has_stream], "-h"))
		{
			*flag_h = 1;
			n--;
		}
		else
		{
			printf("Usage of incorrect flags!\n"); // talvez imprimir o metodo e invocação
			exit(-1);
		}
	}

}
