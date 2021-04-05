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

#define TCP_PORT 34666;
#define MAXBUFLEN 100

int sockfd, new_fd;
struct sockaddr_storage scheduler_addr;

// edited from beej's tutorial
void tcp_connection_setup() {
	struct addrinfo hints, *servinfo, *p;
	
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN]; 
	int rv;

	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_STREAM; 
	//hints.ai_flags = AI_PASSIVE;   // use my IP
	if ((rv = getaddrinfo("127.0.0.1", TCP_PORT, &hints, &servinfo)) != 0) { 
		perror("Error: getaddrinfo"); 
		exit(1);
	}
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) { 
			perror("Error: client socket"); 
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
	        perror("Error: setsockopt");
			exit(1); 
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
			close(sockfd);
	        perror("Error: client bind");
			continue; 
		}
		break; 
	}
	freeaddrinfo(servinfo);

	if(p == NULL){
		perror("server: failed to bind\n"); 
		exit(1);
	}
	if (listen(sockfd, BACKLOG) == -1) { 
		perror("Error: listen\n");
		exit(1);
	}

}

// edited from beej's tutorial
void send_loc_to_scheduler(char* loc) {
	sin_size = sizeof scheduler_addr;
	new_fd = accept(sockfd, (struct sockaddr *)&scheduler_addr, &sin_size);

	if (new_fd == -1) { 
		perror("Error: accept"); 
		exit(1);
	}
	if (!fork()) { // this is the child process 
		close(sockfd); // child doesn't need the listener 
		if (send(new_fd, loc, 13, 0) == -1) {
                perror("Error: send\n");
                exit(1);
        }
        close(new_fd);
	}
	close(new_fd);
	cout << "The client has sent query to Scheduler using TCP: client location " << loc << endl;
}

int main(int argc, char* argv[]) {
	if (argc <= 0 || argc > 1) {
		perror("Error: wrong client usage information format.\n");
		exit(1);
	}
	cout << "The client is up and running." << endl;

	tcp_connection_setup();
	send_loc_to_scheduler(argv[0]);

}