/*
 This is a basic proxy server.
 function: 	- Block web pages 
			- Manipulate web content (Feature not added but
			can be implemented at comment labeled A123);
 
 To run: compile and "./Main port"
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>

#define BUFFER_SIZE 8192
int secondSocket;
//wbState if 1 BLOCK TRAFFIC
int wbState = 1;
				
// Prototypes
void Terminate();
int tcpCON(char host[], char path[], int port_HTTP);
int run(int port);

void Terminate() {
	exit(-1);
}

// Use TCP to connect to the server
int tcpCON(char host[], char path[], int port_HTTP) {
	char REQ[BUFFER_SIZE];
	struct hostent *server;
	struct sockaddr_in serveraddr;
	int tcpSocket = -1;

	tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (tcpSocket < 0){
	   printf("FAILED!");
	}
	else{
	   printf("Socket OK.\n");
	}

	// get the server by the url address
	server = gethostbyname(host);
	
	memset((char *) &serveraddr, 0, sizeof(serveraddr));
	
	serveraddr.sin_family = AF_INET;
	memcpy((char *) &serveraddr.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
	serveraddr.sin_port = htons(port_HTTP);

	if (connect(tcpSocket, (struct sockaddr *) &serveraddr, sizeof(serveraddr))< 0)
	   perror("connect() failed.\n");

	memset(REQ, 0, BUFFER_SIZE);
	memset(REQ, 0, BUFFER_SIZE);
	
	sprintf(REQ, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);

	if (send(tcpSocket, REQ, strlen(REQ), 0) < 0) {
	   perror("send() failed.\n");
	}
	return tcpSocket;
}

int run(int port){
	int primarySocket;
	int pid;
	char recieve[BUFFER_SIZE];
	struct sockaddr_in server;
	
	printf("--Connected--");

	// Initial && Listening 
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY );

	if ((primarySocket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
	   perror("Exiting set up, socket() failure.\n");
	   exit(1);
	}
	if (bind(primarySocket, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) == -1) {
	   perror("Exiting set up, bind() failure.\n");
	   exit(1);
	}

	if (listen(primarySocket, 50) == -1) {
	   perror("Exiting, listen() failed.\n");
	   exit(1);
	}
	
	
	// Listen
	for (;;) {
        // accept a connection 
        if ((secondSocket = accept(primarySocket, NULL, NULL )) == -1) {
            perror("Exiting\n");
            exit(1);
        }

        // fork a child process for client 
        pid = fork();

        // pid returned by fork to decide what to do next 
        if (pid < 0) {
            perror("Exiting\n");
            exit(1);
        } else if (pid == 0) {
            // Using child process now
            close(primarySocket);

            // obtain the message from this client 
            while (recv(secondSocket, recieve, BUFFER_SIZE, 0) > 0) {
			
                //GET REQ, URL extracted
                char *pathname = strtok(recieve, "\r\n");
                char URL[BUFFER_SIZE];
                puts(pathname);
                if (sscanf(pathname, "GET http://%s", URL) == 1) {
                    printf("URL = %s\n", URL);
                }


                char HOST[BUFFER_SIZE];
                char PATH[BUFFER_SIZE];
                char REQ[BUFFER_SIZE];
                char response[BUFFER_SIZE];
                char header[BUFFER_SIZE];
                int port_HTTP = 80;

                // seperate the host and path
                int i;
                for (i = 0; i < strlen(URL); i++) {
                    if (URL[i] == '/') {
						//copy hostname
                        strncpy(HOST, URL, i);
                        HOST[i] = '\0';
                        break;
                    }
                }
				
				memset(PATH, 0, BUFFER_SIZE);
				
                for (; i < strlen(URL); i++) {
					//copy out the path
                    strcat(PATH, &URL[i]);
                    break;
                }

                printf("HOST NAME: %s\n", HOST);
                printf("PATH NAME: %s\n", PATH);

                // Establish a connection to the internet
                int tcpSocket = tcpCON(HOST, PATH, port_HTTP);

                int readBytes = 0;
                int sizeOfHead = 0;
                int inHead = 1;
				
				memset(REQ, 0, BUFFER_SIZE);
				memset(header, 0, BUFFER_SIZE);
				
                while ((readBytes = read(tcpSocket, response, BUFFER_SIZE)) > 0) {

                    //-- A123 -- OPERATION THAT YOU WANT TO DO TO THE HEADER OR BODY In responsee
					//ie. change text, header, etc.
					
                    //If wbState is 1 BLOCK PAGE 
                    if (wbState == 1) {                 
                        close(tcpSocket);
						//ERROR PAGE -- NOT SET <INSERT HERE>
                        sprintf(HOST, "");
                        sprintf(PATH, "");
                        tcpSocket = tcpCON(HOST, PATH, port_HTTP);
                        while ((readBytes = read(tcpSocket, response, BUFFER_SIZE)) > 0) {
                            send(secondSocket, response, readBytes, 0);
                        }
                        for (i = 0; i < BUFFER_SIZE; i++) {
                            response[i] = '\0';
                        }
                        signal(SIGTERM, Terminate);
                        signal(SIGINT, Terminate);
                        close(secondSocket);
                        close(tcpSocket);
                        exit(0);
                    }

                    //send data back to the web browser
                    send(secondSocket, response, readBytes, 0);
                    for (i = 0; i < BUFFER_SIZE; i++) {
                        response[i] = '\0';
                    }
                }
                close(tcpSocket);
                close(secondSocket);

                /* clear out message strings again to be safe */
                for (i = 0; i < BUFFER_SIZE; i++) {
                    recieve[i] = '\0';
                }
            }

            //Client is no longer sending messages
            signal(SIGTERM, Terminate);
            signal(SIGINT, Terminate);
            close(secondSocket);
            exit(0);
        } 
        else {
            close(secondSocket);
        }
    }
	
	return 0;
}

int main(int argc, char *argv[]) {
	int port = -1;
	
	// Input args
	if (argc == 1) {
		char * arg1;
		arg1 = argv[1];
		port = atoi(arg1);		
	}
	
	else if( argc > 2 ) {
		 printf("Too many args.\n");
	}
	// Port Error
	else{
		printf("One arg expected\n");
		return 0;
	}
	
	run(port);
	
    return 0;
}