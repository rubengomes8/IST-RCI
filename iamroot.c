#include "udp.h"
#include "tcp.h"
#include "utils.h"

#define BUFFER_SIZE 64
int main(int argc, char *argv[]){
		char streamID[BUFFER_SIZE]; //mystream:193.136.138.142:59000 (exemplo)
		char ipaddr[20];
		char tport[6] = "58000"; //tport - porto TCP onde a app aceita sessões de outros pares a jusante
		char uport[6] = "58000"; //uport - porto UDP do servidor de acesso
		char rsaddr[20] = "193.136.138.142"; //endereço IP do servidor de raízes
		char rsport[6] = "59000"; //porto UDP do servidor de raízes
		int tcp_sessions = 1; //número de sessões TCP que a aplicação aceita a jusante
		int bestpops = 1; //número de pontos de acesso, não inferior a um, a recolher por esta instância da aplicação, quando raiz, durante a descoberta de pontos de acesso
		int tsecs = 5; //período, em segundos, associado ao registo periódico que a raiz deve fazer no servidor de raízes
		int n = 0, i = 0, has_stream = 0;
		int flag_b = 1, flag_d = 0, flag_h = 0;
		char buffer[BUFFER_SIZE];

		//Só para ver se há stream ou não
		if(argc > 1)
		{
				if((n = strlen(argv[1])) > 2)
				{
						//Houve especificação da stream
						n = count_specific_char(argv[1], ':');
						if(n == 2)
						{
								has_stream = 1;
							  strcpy(streamID, argv[1]);
								printf("streamID: %s\n", streamID);
						}	else {
								printf("stream ID format: <Name:IPAddress:Port\n");
								exit(-1);
						}
				}	else {
						//Apresentar a lista de streams registados no servidor de raízes - IMPLEMENTAR

				}
		} else {
				//Apresentar a lista de streams registados no servidor de raízes - IMPLEMENTAR
		}

		//Se houver stream começa a ler do argv[3], se não do argv[2]
		//Acho que isto dá problema se executarmos com uma das flags isoladas (b, d ou h) antes de uma das outras.
		for (n = 1; n < argc - 1; n+=2)
    {
				//ler o resto da informação com as tags correspondentes
				if(!strcmp(argv[n + has_stream], "-i")){
							strcpy(ipaddr, argv[n + has_stream + 1]);
							printf("ipaddr: %s\n", ipaddr);
				}	else if(!strcmp(argv[n + has_stream], "-t")){
							strcpy(tport, argv[n + has_stream + 1]);
							printf("tport: %s\n", tport);
				} else if(!strcmp(argv[n + has_stream], "-u")){
							strcpy(uport, argv[n + has_stream + 1]);
							printf("uport: %s\n", uport);
				}	else if(!strcmp(argv[n + has_stream], "-s")){
							i = count_specific_char(argv[n + has_stream + 1], ':');
							if(i == 2){
									//split into rsaddr e rsport
									//rsaddr = strtok(argv[n + has_stream + 1], ":");
									//rsport = strtok(NULL, " \n");
									//printf("rsaddr: %s\n", rsaddr);
									//printf("rsaddr: %s\n", rsport);

							} else if (i == 1){
									//strcpy(rsadrr, argv[n + has_stream + 1]);
							}
				} else if(!strcmp(argv[n + has_stream], "-p")){
							tcp_sessions = atoi(argv[n + has_stream + 1]);
							if(tcp_sessions >= 1)
									break;
							else{
									printf("tcpsessions must be at least 1!\n");
									exit(-1);
							}
				} else if(!strcmp(argv[n + has_stream], "-n")){
							bestpops = atoi(argv[n + has_stream + 1]);
							if(bestpops >= 1)
									break;
							else{
									printf("bestpops must be at least 1!\n");
									exit(-1);
							}
				} else if(!strcmp(argv[n + has_stream], "-x")){
							tsecs = atoi(argv[n + has_stream + 1]);
							if(tsecs >= 1) //Estou a exigir que o período seja pelo menos 1 segundo mas podemos mudar este valor
									break;
							else{
									printf("tsecs must be at least 1!\n");
									exit(-1);
							}
				} else if(!strcmp(argv[n + has_stream], "-b")){
							flag_b = 0;
							n--;
				} else if(!strcmp(argv[n + has_stream], "-d")){
							flag_d = 1;
							n--;
				} else if(!strcmp(argv[n + has_stream], "-h")){
							flag_h = 1;
							n--;
				} else{
							printf("Usage of incorrect flags!\n"); // talvez imprimir o metodo e invocação
							exit(-1);
				}

    }

		printf("\n%d\n", argc);


}
