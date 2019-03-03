#include "udp.h"
#include "tcp.h"
#include "utils.h"

#define BUFFER_SIZE 64
#define RS_IP "193.136.138.142"
#define RS_PORT "59000"
#define WIR_LEN 103 //whoisroot length 9+1+64+1+20+1+6+1=103
#define RIS_LEN 100 //rootis length 6+1+64+1+20+1+6+1=100
#define URR_LEN 82 //urroot length 6+1+64+1=82

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
	/* Conexão com o servidor de raízes */
  int fd_rs;
	struct addrinfo *res_rs;
	char rasaddr[20]; //endereço IP do servidor de acesso da raiz
	char rasport[6]; //porto UDP do servidor de acesso da raiz
	char *msg = NULL;
	int msg_len;
	struct sockaddr_in addr;
  unsigned int addrlen;

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
			}
			else
			{
				printf("stream ID format: <Name:IPAddress:Port\n");
				exit(-1);
			}
		}
		else
		{
			//has_stream=0 -> apresentar lista
		}
	}
	else
	{
		//has_stream=0 -> apresentar lista
	}

	//Se houver stream começa a ler do argv[3], se não do argv[2]
	//Acho que isto dá problema se executarmos com uma das flags isoladas (b, d ou h) antes de uma das outras.
	arguments_reading(argc, argv, has_stream, ipaddr, tport, uport, rsaddr, rsport, &tcp_sessions, &bestpops, &tsecs, &flag_b, &flag_d, &flag_h);

	/* Conectar-se ao servidor de raízes com protocolo UDP */
	fd_rs = udp_socket(RS_IP, RS_PORT, &res_rs);

	if(has_stream)
	{
		//solicitar ao root_server o endereço IP e porto UDP do root_access_server associado ao streamID
		msg = (char*) malloc(sizeof(char)*WIR_LEN);
		if(msg == NULL)
    {
    	fprintf(stderr, "Error: malloc: %s\n", strerror(errno));
    	exit(-1);
    }
		sprintf(msg, "WHOISROOT %s %s:%s\n", streamID, ipaddr, uport);
		msg_len = strlen(msg);
		//printf("msg: %s", msg);
		udp_send(fd_rs, msg, msg_len, 0, res_rs);
		free(msg);

		//Recebe como resposta ROOTIS - 100 ou URROOT - 82
		msg = (char*) malloc(sizeof(char)*RIS_LEN);
		if(msg == NULL)
    {
    	fprintf(stderr, "Error: malloc: %s\n", strerror(errno));
    	exit(-1);
    }
		n = RIS_LEN;
		n = udp_receive(fd_rs, &n, msg, 0, &addr, &addrlen)
		printf("Received by Root Server: %s", msg);
		//print_sender(addr, addrlen, 0);
		if(n == 5) //Recebeu Error
		{
			printf("Make sure the stream ID, the IPaddress and the port UDP are well formated\n");
			exit(-1);
		}
		else
		{
			strncpy(buffer ,msg, 6);
			buffer[6] = '\0';
			if(!strcmp(buffer, "URROOT"))
			{
				//caso não haja nenhuma raiz associada ao streamID
				//aplicação fica registada como sendo a raiz da nova árvore e escoamento

				//1. Estabelecer sessão TCP com o servidor fonte

				//2. instalar servidor TCP para o ponto de acesso a jusante

				//3. instalar o servidor UDP de acesso de raiz

				//4. executar a interface de utilizador
			}
			else if (!strcmp(buffer, "ROOTIS"))
			{
				//caso já exista uma raiz associada à stream, a aplicação deverá

				//1. solicitar ao servidor de acesso da raiz o IP e porto TCP do ponto de acesso

				//2. estabelecer sessão TCP com o ponto de acesso

				//3. aguardar confirmação de adesão
			}
		}


		free(msg);




	}
	else
	{
		//apresentar lista
	}
	close(fd_rs);

}
