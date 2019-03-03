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
		char *token;

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
		arguments_reading(argc, argv, has_stream, ipaddr, tport, uport, rsaddr, rsport, &tcp_sessions, &bestpops, &tsecs, &flag_b, &flag_d, &flag_h);

		printf("\n%d\n", argc);


}
