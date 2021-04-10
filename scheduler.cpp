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

#define HospitalA_PORT "30666"
#define HospitalB_PORT "31666"
#define HospitalC_PORT "32666"
#define Scheduler_UDP_PORT "33666"
#define TCP_PORT "34666"
#define MAXBUFLEN 100
#define MAXDATASIZE 100 // max number of bytes we can get at once
#define BACKLOG 10

using namespace std;

struct hospital_info
{
	int capacity;
	int occupancy;
	float distance;
	float score;
};

struct sockaddr_in scheduler_addr;
struct sockaddr_in hospitalA_addr;  
struct sockaddr_in hospitalB_addr;  
struct sockaddr_in hospitalC_addr;  

hospital_info hosp_A;
hospital_info hosp_B;
hospital_info hosp_C;

int udp_sockfd;
int tcp_sockfd;

void set_up_hospital_sock() {
	// set hospital info
	memset(&hospitalA_addr, 0, sizeof(hospitalA_addr));   
	hospitalA_addr.sin_family = AF_INET;		
	hospitalA_addr.sin_port = htons(30666);
	hospitalA_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	memset(&hospitalB_addr, 0, sizeof(hospitalB_addr));   
	hospitalB_addr.sin_family = AF_INET;		
	hospitalB_addr.sin_port = htons(31666);
	hospitalB_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	memset(&hospitalC_addr, 0, sizeof(hospitalC_addr));   
	hospitalC_addr.sin_family = AF_INET;		
	hospitalC_addr.sin_port = htons(32666);
	hospitalC_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

}

void recv_from_hosp(char* buf, struct sockaddr_in hosp_addr) {
	int numbytes;
	socklen_t addr_len = sizeof hosp_addr;
//	cout << "waiting for message from hospital" << endl;
	if ((numbytes = recvfrom(udp_sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&hosp_addr, &addr_len)) == -1) { 
		perror("Error: scheduler recvfrom() error for hospital capacity.");
		exit(1);
	}
	
	buf[numbytes] = '\0';
}

void listen_to_hospital(struct sockaddr_in hosp_addr, string hosp_name) {
//	cout << "listen to hospital " << hosp_name << endl;
	char buf_cap[MAXBUFLEN];
	char buf_occ[MAXBUFLEN];

	recv_from_hosp(buf_cap, hosp_addr);
	recv_from_hosp(buf_occ, hosp_addr);
	if (hosp_name.compare("A") == 0) {
		hosp_A.capacity = atoi(buf_cap);
		hosp_A.occupancy = atoi(buf_occ);
		// initialize score and distance to invalid value -1
		hosp_A.score = -1;
		hosp_A.distance = -1;
	}else if (hosp_name.compare("B") == 0) {
		hosp_B.capacity = atoi(buf_cap);
		hosp_B.occupancy = atoi(buf_occ);
		// initialize score and distance to invalid value -1
		hosp_B.score = -1;
		hosp_B.distance = -1;
	}else if (hosp_name.compare("C") == 0) {
		hosp_C.capacity = atoi(buf_cap);
		hosp_C.occupancy = atoi(buf_occ);
		// initialize score and distance to invalid value -1
		hosp_C.score = -1;
		hosp_C.distance = -1;
	}
}

void recv_dis_and_score(struct sockaddr_in hosp_addr, string hosp_name) {
//	cout << "listen to hospital " << hosp_name << endl;
	char buf_dis[MAXBUFLEN];
	char buf_scr[MAXBUFLEN];
	const char* notfound = "location not found";
	recv_from_hosp(buf_dis, hosp_addr);
	
	if (strcmp(buf_dis, notfound) == 0) {
		return;
	}
	recv_from_hosp(buf_scr, hosp_addr);
	if (hosp_name.compare("A") == 0) {
		hosp_A.distance = atof(buf_dis);
		hosp_A.score = atof(buf_scr);
	}else if (hosp_name.compare("B") == 0) {
		hosp_B.distance = atof(buf_dis);
		hosp_B.score = atof(buf_scr);
	}else if (hosp_name.compare("C") == 0) {
		hosp_C.distance = atof(buf_dis);
		hosp_C.score = atof(buf_scr);
	
	}
}


void udp_port_setup() {
	
	memset(&scheduler_addr, 0, sizeof(scheduler_addr));   
	scheduler_addr.sin_family = AF_INET;		
	scheduler_addr.sin_port = htons(33666);
	scheduler_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
 
	
	if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) { 
		perror("Error: socket\n"); 
		exit(1);
	}
	if (bind(udp_sockfd, (struct sockaddr *)&scheduler_addr, sizeof(struct sockaddr)) == -1) { 
			close(udp_sockfd);
            perror("Error: bind\n");
			exit(1); 
		}

}

void initialize_hospital_info() {	

	listen_to_hospital(hospitalA_addr, "A");
	cout << "The Scheduler has received information from Hospital A: total capacity is ​" << hosp_A.capacity << " and initial occupancy is ​" << hosp_A.occupancy << endl;
	listen_to_hospital(hospitalB_addr, "B");
	cout << "The Scheduler has received information from Hospital B: total capacity is ​" << hosp_B.capacity << " and initial occupancy is ​" << hosp_B.occupancy << endl;
	listen_to_hospital(hospitalC_addr, "C");
	cout << "The Scheduler has received information from Hospital C: total capacity is ​" << hosp_C.capacity << " and initial occupancy is ​" << hosp_C.occupancy << endl;
	
}

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
 * after scheduler gets the location of client, send the location info to hospital that has available slots
 */
void send_client_info_to_hospital(char* client_loc) {
	int numbytesA;
	int numbytesB;
	int numbytesC;
	if (hosp_A.capacity > hosp_A.occupancy) {
		if ((numbytesA = sendto(udp_sockfd, client_loc, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalA_addr, sizeof(struct sockaddr))) == -1) { 
			perror("Error: scheduler fails to send client location to hospital A\n");
			exit(1);
		}
		cout << "The Scheduler has sent client location to Hospital A using UDP over port " << Scheduler_UDP_PORT << endl;
	}
	if (hosp_B.capacity > hosp_B.occupancy) {
		if ((numbytesB = sendto(udp_sockfd, client_loc, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalB_addr, sizeof(struct sockaddr))) == -1) { 
			perror("Error: scheduler fails to send client location to hospital B\n");
			exit(1);
		}
		cout << "The Scheduler has sent client location to Hospital B using UDP over port " << Scheduler_UDP_PORT << endl;
	}
	if (hosp_C.capacity > hosp_C.occupancy) {
		if ((numbytesC = sendto(udp_sockfd, client_loc, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalC_addr, sizeof(struct sockaddr))) == -1) { 
			perror("Error: scheduler fails to send client location to hospital C\n");
			exit(1);
		}
		cout << "The Scheduler has sent client location to Hospital C using UDP over port " << Scheduler_UDP_PORT << endl;
	}
}


int assign_hospital() {
	if (hosp_A.score == -1 || hosp_B.score == -1 || hosp_C.score == -1) {
		return -1;
	}
	if (hosp_A.score == hosp_B.score && hosp_A.score > hosp_C.score) {
		if (hosp_B.distance < hosp_A.distance) {
			return 2;
		}else {
			return 1;
		}
	}else if (hosp_A.score == hosp_C.score && hosp_A.score > hosp_B.score) {
		if (hosp_C.distance < hosp_A.distance) {
			return 3;
		}else {
			return 1;
		}
	}else if (hosp_B.score == hosp_C.score && hosp_B.score > hosp_A.score) {
		if (hosp_B.distance < hosp_C.distance) {
			return 2;
		}else {
			return 3;
		}
	}else if (hosp_A.score == hosp_B.score && hosp_B.score == hosp_C.score) {
		if (hosp_A.distance < hosp_B.distance && hosp_A.distance < hosp_C.distance) {
			return 1;
		}else if (hosp_B.distance < hosp_A.distance && hosp_B.distance < hosp_C.distance) {
			return 2;
		}else {
			return 3;
		}
	}
	if (hosp_A.score > hosp_B.score && hosp_A.score > hosp_C.score) {
		return 1;
	}else if (hosp_B.score > hosp_C.score && hosp_B.score > hosp_A.score) {
		return 2;
	}else {
		return 3;
	}
}

void assign_client() {
	int new_fd;
	int numbytes;
	char buf[MAXBUFLEN];
	struct addrinfo hints, *servinfo, *p; 
	struct sockaddr_storage client_addr;
	int rv;
	int yes = 1;
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo("127.0.0.1", TCP_PORT, &hints, &servinfo)) != 0) { 
		perror("Error: getaddrinfo\n"); 
		exit(1);
	}
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((tcp_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) { 
			perror("Error: socket()\n"); 
			continue;
		}
		if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        	perror("Error :setsockopt\n");
			exit(1); 
		}
		if (bind(tcp_sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
			close(tcp_sockfd);
        	perror("Error: server bind\n");
			continue; 
		}

		break; 
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL) {
		perror("Error: failed to connect.\n");
		exit(1);
	}
	if (listen(tcp_sockfd, BACKLOG) == -1) { 
		perror("listen");
		exit(1);
	}
//s	cout << "server: waiting for connections..." << endl;
	while(1) { // main accept() loop
		sin_size = sizeof client_addr;
		new_fd = accept(tcp_sockfd, (struct sockaddr *)&client_addr, &sin_size);
		if (new_fd == -1) { 
			perror("accept"); 
			continue;
		}
		inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
    //    printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process 
			close(tcp_sockfd); // child doesn't need the listener 
			if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
            	perror("Error: scheduler fail to receive from client\n");
			}
			buf[numbytes] = '\0';
			cout << "The Scheduler has received client at location " << buf << " from the client using TCP over port " << TCP_PORT << endl;
 
            
			send_client_info_to_hospital(buf);
		
			recv_dis_and_score(hospitalA_addr, "A");
			recv_dis_and_score(hospitalB_addr, "B");
			recv_dis_and_score(hospitalC_addr, "C");

			if (hosp_A.score == -1 || hosp_B.score == -1 || hosp_C.score == -1) {
				cout << "The Scheduler has received map information from Hospital A, the score = None and the distance = None" << endl;
				cout << "The Scheduler has received map information from Hospital B, the score = None and the distance = None" << endl;
				cout << "The Scheduler has received map information from Hospital C, the score = None and the distance = None" << endl;
			}else {
				cout << "The Scheduler has received map information from Hospital A, the score = " << hosp_A.score << " and the distance = " << hosp_A.distance << endl;
				cout << "The Scheduler has received map information from Hospital B, the score = " << hosp_B.score << " and the distance = " << hosp_B.distance << endl;
				cout << "The Scheduler has received map information from Hospital C, the score = " << hosp_C.score << " and the distance = " << hosp_C.distance << endl;
			}
		
		// 	if (hosp_B.score == -1 || hosp_B.score == -1 || hosp_C.score == -1) {
		// 		cout << "The Scheduler has received map information from Hospital B, the score = None and the distance = None" << endl;
		// 	}else {
		// 		cout << "The Scheduler has received map information from Hospital B, the score = " << hosp_B.score << " and the distance = " << hosp_B.distance << endl;
		// 	}
		// //	cout << "The Scheduler has received map information from Hospital B, the score = " << hosp_B.score << " and the distance = " << hosp_B.distance << endl;
		
		// //	cout << "The Scheduler has received map information from Hospital C, the score = " << hosp_C.score << " and the distance = " << hosp_C.distance << endl;	
		// 	if (hosp_C.score == -1 || hosp_B.score == -1 || hosp_C.score == -1) {
		// 		cout << "The Scheduler has received map information from Hospital C, the score = None and the distance = None" << endl;
		// 	}else {
		// 		cout << "The Scheduler has received map information from Hospital C, the score = " << hosp_C.score << " and the distance = " << hosp_C.distance << endl;
		// 	}

			int assign_res = assign_hospital();
			const char* ass;
			int numbytesA;
			int numbytesB;
			int numbytesC;
			const char* assigned = "1";
			const char* unassigned = "0";
			
			// send assignment result to hospital
			if (assign_res == 1) {
				ass = "A";
				cout << "The scheduler has assigned Hospital A to the client" << endl;
				hosp_A.occupancy += 1;
				if ((numbytesA = sendto(udp_sockfd, assigned, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalA_addr, sizeof(struct sockaddr))) == -1) { 
					perror("Error: scheduler fails to send result to hospital A\n");
					exit(1);
				}
				if ((numbytesB = sendto(udp_sockfd, unassigned, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalB_addr, sizeof(struct sockaddr))) == -1) { 
					perror("Error: scheduler fails to send result to hospital B\n");
					exit(1);
				}
				if ((numbytesC = sendto(udp_sockfd, unassigned, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalC_addr, sizeof(struct sockaddr))) == -1) { 
					perror("Error: scheduler fails to send result to hospital ACn");
					exit(1);
				}
				cout << "The scheduler has sent the result to Hospital A using UDP over port " << Scheduler_UDP_PORT << endl;
			}else if (assign_res == 2) {
				ass = "B";
				cout << "The scheduler has assigned Hospital B to the client" << endl;
				hosp_B.occupancy += 1;
				if ((numbytesA = sendto(udp_sockfd, unassigned, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalA_addr, sizeof(struct sockaddr))) == -1) { 
					perror("Error: scheduler fails to send result to hospital A\n");
					exit(1);
				}
				if ((numbytesB = sendto(udp_sockfd, assigned, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalB_addr, sizeof(struct sockaddr))) == -1) { 
					perror("Error: scheduler fails to send result to hospital B\n");
					exit(1);
				}
				if ((numbytesC = sendto(udp_sockfd, unassigned, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalC_addr, sizeof(struct sockaddr))) == -1) { 
					perror("Error: scheduler fails to send result to hospital C\n");
					exit(1);
				}
				cout << "The scheduler has sent the result to Hospital B using UDP over port " << Scheduler_UDP_PORT << endl;
			}else if (assign_res == 3) {
				ass = "C";
				cout << "The scheduler has assigned Hospital C to the client" << endl;
				hosp_C.occupancy += 1;
				if ((numbytesA = sendto(udp_sockfd, unassigned, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalA_addr, sizeof(struct sockaddr))) == -1) { 
					perror("Error: scheduler fails to send result to hospital A\n");
					exit(1);
				}
				if ((numbytesB = sendto(udp_sockfd, unassigned, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalB_addr, sizeof(struct sockaddr))) == -1) { 
					perror("Error: scheduler fails to send result to hospital B\n");
					exit(1);
				}
				if ((numbytesC = sendto(udp_sockfd, assigned, MAXBUFLEN-1 , 0, (struct sockaddr *)&hospitalC_addr, sizeof(struct sockaddr))) == -1) { 
					perror("Error: scheduler fails to send result to hospital C\n");
					exit(1);
				}
				cout << "The scheduler has sent the result to Hospital C using UDP over port " << Scheduler_UDP_PORT << endl;
			}else {
				ass = "None";
			}
			// send assignment result to client
			if ((numbytes = send(new_fd, ass, MAXDATASIZE-1, 0)) == -1) {
            	perror("Error: scheduler fail to send assignment to client\n");
			}
			cout << "The Scheduler has sent the result to client using TCP over port " << TCP_PORT << endl;
			close(new_fd);
			exit(0); 
		}
		close(new_fd); // parent doesn't need this
	}
	

}


int main(int argc, char* argv[]) {
	set_up_hospital_sock();
	udp_port_setup();
	cout << "The Scheduler is up and running." << endl;
 
	initialize_hospital_info();
	assign_client();
//	recv_dis_and_score(hospitalA_addr, "A");
//	initialize_hospital_info();
}
