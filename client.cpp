#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>

using namespace std;

#define TCP_PORT "34666"
#define MAXBUFLEN 100
#define MAXDATASIZE 100
#define BACKLOG 10

int sockfd;
struct sockaddr_storage scheduler_addr;

// get sockaddr, IPv4 or IPv6:
// void *get_in_addr(struct sockaddr *sa) {
// 	if (sa->sa_family == AF_INET) {
// 		return &(((struct sockaddr_in*)sa)->sin_addr);
// 	}
// 	return &(((struct sockaddr_in6*)sa)->sin6_addr); 
// }

// edited from beej's tutorial
void send_loc_to_scheduler(char* loc) {
	struct addrinfo hints, *servinfo, *p;
	
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN]; 
	int rv;

	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM; 
	//hints.ai_flags = AI_PASSIVE;   // use my IP
	if ((rv = getaddrinfo("127.0.0.1", TCP_PORT, &hints, &servinfo)) != 0) { 
		perror("Error: getaddrinfo"); 
		exit(1);
	}
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) { 
			perror("Error: client socket\n"); 
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
			close(sockfd);
            perror("client: connect");
			continue; 
		}
		break;
	}
	

	if(p == NULL){
		perror("Error: client failed to bind\n"); 
		exit(1);
	}
	
	freeaddrinfo(servinfo);
	if (send(sockfd, loc, MAXDATASIZE-1, 0) == -1)
                perror("Error: client fail to send location.\n");
}



int main(int argc, char* argv[]) {
	if (argc <= 0 || argc > 2) {
		perror("Error: wrong client usage information format.\n");
		exit(1);
	}
	cout << "The client is up and running." << endl;

	
	send_loc_to_scheduler(argv[1]);

}