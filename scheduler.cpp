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
#define TCP_PORT "34666";
#define MAXBUFLEN 100

using namespace std;

struct hospital_info
{
	int capacity;
	int occupancy;
};

struct sockaddr_in hospitalA_addr;  
struct sockaddr_in hospitalB_addr;  
struct sockaddr_in hospitalC_addr;  

hospital_info hosp_A;
hospital_info hosp_B;
hospital_info hosp_C;

int udp_sockfd;
int tcp_sockfd;

void listen_to_hospital(int udp_sockfd, struct sockaddr_in hosp_addr, string hosp_name) {
	char buf_cap[MAXBUFLEN];
	char buf_occ[MAXBUFLEN];
	int numbytes1;
	int numbytes2;
	addr_len = sizeof hosp_addr;
	if ((numbytes1 = recvfrom(udp_sockfd, buf_cap, MAXBUFLEN-1 , 0, (struct sockaddr *)&hosp_addr, &addr_len)) == -1) { 
		perror("Error: scheduler recvfrom() error for hospital capacity.");
		exit(1);
	}
	buf_cap[numbytes1] = '\0';
	if ((numbytes2 = recvfrom(udp_sockfd, buf_occ, MAXBUFLEN-1 , 0, (struct sockaddr *)&hosp_addr, &addr_len)) == -1) { 
		perror("Error: scheduler recvfrom() error for hospital capacity.");
		exit(1);
	}
	buf_occ[numbytes2] = '\0';
	if (hosp_name.compare("A") == 0) {
		hosp_A.capacity = stoi(buf_cap);
		hosp_A.occupancy = stoi(buf_occ);
	}else if (hosp_name.compare("B") == 0) {
		hosp_B.capacity = stoi(buf_cap);
		hosp_B.occupancy = stoi(buf_occ);
	}else if (hosp_name.compare("C") == 0) {
		hosp_C.capacity = stoi(buf_cap);
		hosp_C.occupancy = stoi(buf_occ);
	}
}

//edited from beej's tutorial
void udp_port_setup() {	
	struct addrinfo hints, *servinfo, *p; 
	int rv;
	
 
	
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4 
	hints.ai_socktype = SOCK_DGRAM;
	// hints.ai_flags = AI_PASSIVE; // use my IP
	if ((rv = getaddrinfo("127.0.0.1", MYPORT, &hints, &servinfo)) != 0) { 
		perror("Error: getaddrinfo\n"); 
		return 1;
	}
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((udp_sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) { 
			perror("Error: socket\n"); 
			continue;
		}
		if (bind(udp_sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
				close(udp_sockfd);
	            perror("Error: bind\n");
				continue; 
		}
		break;
	}
	if (p == NULL) {
		perror("Error: failed to bind socket\n"); 
		exit(1);
	}
	freeaddrinfo(servinfo);
	
	listen_to_hospital(udp_sockfd, hospitalA_addr, "A");
	cout << "The Scheduler has received information from Hospital A: total capacity is ​" << hosp_A.capacity << "and initial occupancy is ​" << hosp_A.occupancy << endl;
	listen_to_hospital(udp_sockfd, hospitalB_addr, "B");
	cout << "The Scheduler has received information from Hospital B: total capacity is ​" << hosp_B.capacity << "and initial occupancy is ​" << hosp_B.occupancy << endl;
	listen_to_hospital(udp_sockfd, hospitalC_addr, "C");
	cout << "The Scheduler has received information from Hospital C: total capacity is ​" << hosp_C.capacity << "and initial occupancy is ​" << hosp_C.occupancy << endl;
	
}

void receive_from_client() {
	int numbytes;
	char buf[MAXBUFLEN];
	struct addrinfo hints, *servinfo, *p; 
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) { 
		perror("Error: getaddrinfo"); 
		exit(1);
	}
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((tcp_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) { 
			perror("client: socket"); 
			continue;
		}
		if (connect(tcp_sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
			close(tcp_sockfd);
            perror("Error: client tcp connect\n");
			continue; 
		}
		break; 
	}
	if (p == NULL) {
		perror("Error: failed to connect.\n");
		exit(1);
	}

	freeaddrinfo(servinfo); // all done with this structure

	if ((numbytes = recv(tcp_sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("Error: recv\n");
		exit(1);
	}
	buf[numbytes] = '\0';
	cout << "The Scheduler has received client at location ​" << buf << " from the client using TCP over port " << TCP_PORT << endl;
	close(tcp_sockfd);

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

	cout << "The Scheduler is up and running." << endl;
 
	udp_port_setup();
	receive_from_client();

}
