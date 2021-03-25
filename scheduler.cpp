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

#define Scheduler_UDP_PORT "33666";
#define Scheduler_TCP_PORT "34666";
#define MAXBUFLEN 100

struct sockaddr_in hospitalA_addr;  
struct sockaddr_in hospitalB_addr;  
struct sockaddr_in hospitalC_addr;  

void listen_to_hosital(int sockfd, struct sockaddr_in hosp_addr) {
	addr_len = sizeof hosp_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&hosp_addr, &addr_len)) == -1) { 
		perror("Error: scheduler recvfrom()");
		exit(1);
	}
	
	buf[numbytes] = '\0';
	printf("Scheduler: packet contains \"%s\"\n", buf);
}

//edited from beej's tutorial
void udp_port_setup() {
	int sockfd;
	struct addrinfo hints, *servinfo, *p; 
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr; 
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4 hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) { 
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); 
		return 1;
	}
	    // loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) { 
			perror("Error: socket"); continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
				close(sockfd);
	            perror("Error: bind");
				continue; 
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "Error: failed to bind socket\n"); 
		return 2;
	}
	freeaddrinfo(servinfo);
	printf("listener: waiting to recvfrom...\n");
	
	listen_to_hosital(sockfd, hospitalA_addr);
	listen_to_hosital(sockfd, hospitalB_addr);
	listen_to_hosital(sockfd, hospitalC_addr);
	
	close(sockfd);
}

int main(int argc, char* argv[]) {
	// set hospital info
	memset(&hospitalA_addr, 0, sizeof(hospitalA_addr));   
	hospitalA_addr.sin_family = AF_UNSPEC;		
	hospitalA_addr.sin_port = htons(30666);
	hospitalA_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	memset(&hospitalB_addr, 0, sizeof(hospitalB_addr));   
	hospitalB_addr.sin_family = AF_UNSPEC;		
	hospitalB_addr.sin_port = htons(31666);
	hospitalB_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	memset(&hospitalC_addr, 0, sizeof(hospitalC_addr));   
	hospitalC_addr.sin_family = AF_UNSPEC;		
	hospitalC_addr.sin_port = htons(32666);
	hospitalC_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


}
