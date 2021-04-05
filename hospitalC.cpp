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

#define HospitalB_UDP_PORT 32666;
#define MAXBUFLEN 100

using namespace std;

/* global variables */
map<int, map<int, int>> mapMatrix;
int udp_sockfd;
struct sockaddr_in scheduler_addr;  

void bootup() {
	// open map file and read location information from it
	fstream mapFile;
	mapFile.open("map.txt", ios::in);
	string line;
	while (getline(mapFile, line)) {
		stringstream linestream(line);
		int l1, l2, dis;
		linestream >> l1 >> l2 >> dis;
		mapMatrix[l1][l2] = dis;
		mapMatrix[l2][l1] = dis;
	}
}

//edited from beej's tutorial
int udp_port_setup(char* totalCapacity, char* initialOccupancy) {
	struct addrinfo hints, *servinfo, *p; 
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr; 
	// char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4 
	hints.ai_socktype = SOCK_DGRAM;
	//hints.ai_flags = AI_PASSIVE; // use my IP
	if ((rv = getaddrinfo("127.0.0.1", HospitalC_UDP_PORT, &hints, &servinfo)) != 0) { 
		perror("Error: getaddrinfo"); 
		exit(1);
	}
	    // loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) { 
			perror("Error: socket"); 
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
				close(sockfd);
		        perror("Error:  bind");
				continue; 
		}
		break;
	}

	if (p == NULL) {
		perror("listener: failed to bind socket\n"); 
		exit(1);
	}

	freeaddrinfo(servinfo);

	
	addr_len = sizeof scheduler_addr;

	// send initial capacity and occupancy to scheduler
	if ((numbytes = sendto(sockfd, totalCapacity, MAXBUFLEN-1 , 0, (struct sockaddr *)&scheduler_addr, &addr_len)) == -1) { 
		perror("Error: recvfrom");
		exit(1);
	}
	if ((numbytes = sendto(sockfd, initialOccupancy, MAXBUFLEN-1 , 0, (struct sockaddr *)&scheduler_addr, &addr_len)) == -1) { 
		perror("Error: recvfrom");
		exit(1);
	}

	cout << "Hospital C is up and running using UDP on port " << HospitalC_UDP_PORT << "." << endl;
}

int main(int argc, char* argv[]) {
	int loc = argv[0];
	int totalCapacity = stoi(argv[1]);
	int initialOccupancy = stoi(argv[2]);

	// set scheduler info
	memset(&scheduler_addr, 0, sizeof(scheduler_addr));   
	scheduler_addr.sin_family = AF_UNSPEC;		
	scheduler_addr.sin_port = htons(33666);
	scheduler_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	if ((scheduler_sockfd = socket(AF_UNSPEC, SOCK_DGRAM, 0)) == -1) {
		perror("Error: scheduler socket error");
		exit(1);
	}

	bootup();
	udp_port_setup(argv[1], argv[2]);
	
	cout << "Hospital C has total capacity " << totalCapacity << "and initial occupancy " << initialOccupancy << "." << endl;

}
