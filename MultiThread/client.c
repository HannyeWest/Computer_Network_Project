#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#define NAME_SIZE 20
#define MSG_SIZE 100

char name[NAME_SIZE];
char msg[MSG_SIZE];

pthread_t send_tid;
int exitFlag;
int connection;

char SERVER_IP[20];
char SERVER_PORT[6];

int client_sock;

void interrupt(int arg){
	printf("\nYou typped Ctrl + C\n");
	printf("Bye\n");

	pthread_cancel(send_tid);
	pthread_join(send_tid, 0);
	
	close(client_sock);
	exit(1);
}

void *Msg(){
	char buf[NAME_SIZE + MSG_SIZE + 1];

	while (!exitFlag){
		printf("SSAFY << ");

		memset(buf, 0, NAME_SIZE + MSG_SIZE);
		fgets(msg, MSG_SIZE, stdin);
		
		if(!strcmp(msg, "exit\n")){
			exitFlag = 1;
			write(client_sock, msg, strlen(msg));
			break;
		} else if(!strcmp(msg, "close\n")){
			connection = 0;
			write(client_sock, msg, strlen(msg));
			close(client_sock);
			break;
		}

		if (exitFlag) break;

		char *op = strtok(msg, " ");
		if(strcmp(op, "save") == 0){
			char *key = strtok(NULL, " ");
			char *val = strtok(NULL, " ");
			sprintf(buf, "%s %s %s", op, key, val);
		} else if(strcmp(op, "read") == 0){
			char *key = strtok(NULL, " ");
			sprintf(buf, "%s %s", op, key);
		} else sprintf(buf, "%s %s", name, msg);
		
		write(client_sock, buf, strlen(buf));

		if(strcmp(op, "save") != 0){
			memset(buf, 0, NAME_SIZE + MSG_SIZE);
			int len = read(client_sock, buf, NAME_SIZE + MSG_SIZE - 1);
    			if (len == 0){
				printf("INFO :: Server Disconnected\n");
				kill(0, SIGINT);
				exitFlag = 1;
				break;
			}
			printf("%s\n", buf);
		}
	}
}

int main(int argc, char *argv[]){
	if( argc<2 ){
		printf("ERROR Input [User Name]\n");
		exit(1);
	}
	sprintf(name, "[%s]", argv[1]);

	signal(SIGINT, interrupt);
	
	client_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (client_sock == -1){
		printf("ERROR :: 1_Socket Create Error\n");
		exit(1);
	}

	char input[200];
	while(!exitFlag){
		if(connection == 0){
			printf("SSAFY << ");
			fgets(input, sizeof(input), stdin);

			if(strcmp(input, "exit\n") == 0){
				exitFlag = 1;
				break;
			}

			char *cmd = strtok(input, " ");
			if(strcmp(cmd, "connect") != 0) printf("No Connection\n");
			else {
				cmd = strtok(NULL, " ");
				strcpy(SERVER_IP, cmd);
				cmd = strtok(NULL, " ");
				strcpy(SERVER_PORT, cmd);
				connection = 1;

				client_sock = socket(AF_INET, SOCK_STREAM, 0);
 				if (client_sock == -1){
					printf("ERROR :: 1_Socket Create Error\n");
					exit(1); 
 				}

				struct sockaddr_in server_addr = {0};
				server_addr.sin_family = AF_INET;
				server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
				server_addr.sin_port = htons(atoi(SERVER_PORT));
				socklen_t server_addr_len = sizeof(server_addr);
				
				if (connect(client_sock, (struct sockaddr *)&server_addr, server_addr_len) == -1){
					printf("ERROR :: 2_Connect Error\n");
					exit(1);
				}
			}
		} else{
			pthread_create(&send_tid, NULL, Msg, NULL);
			pthread_join(send_tid, 0);
		}
	}
	close(client_sock);
	return 0;
}
