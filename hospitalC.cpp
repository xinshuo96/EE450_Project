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

#include <limits>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>

#define Scheduler_UDP_PORT "33666"
#define HospitalC_UDP_PORT "32666"
#define MAXBUFLEN 100

using namespace std;

/* global variables */
map<int, map<int, float>> mapMatrix;
int loc;
int totalCapacity;
int Occupancy;

int udp_sockfd;
int scheduler_sockfd;
struct sockaddr_in scheduler_addr;  
int client_location;


void bootup() {
	// open map file and read location information from it
	fstream mapFile;
	mapFile.open("map.txt", ios::in);
	string line;
	while (getline(mapFile, line)) {
		stringstream linestream(line);
		int l1, l2;
		float dis;
		linestream >> l1 >> l2 >> dis;
		mapMatrix[l1][l2] = dis;
		mapMatrix[l2][l1] = dis;
	}
}

void set_up_scheduler_sock() {
	// set scheduler info
	memset(&scheduler_addr, 0, sizeof(scheduler_addr));   
	scheduler_addr.sin_family = AF_INET;		
	scheduler_addr.sin_port = htons(33666);

	scheduler_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


	if ((scheduler_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
			perror("Error: fail to create socket for scheduler");
			exit(1);
	}
	// if (bind(scheduler_sockfd,(struct sockaddr *)&scheduler_addr, sizeof(struct sockaddr))==-1) {
	// 		perror("Error: scheduler fail to bind");
	// 		exit(1);
	// }

}

void recieve_client_info() {
	char client_loc[MAXBUFLEN];
	int numbytes;
	socklen_t addr_len = sizeof scheduler_addr;
	if ((numbytes = recvfrom(udp_sockfd, client_loc, MAXBUFLEN-1 , 0, (struct sockaddr *)&scheduler_addr, &addr_len)) == -1) { 
		perror("Error: Hospital C fail to receive client location info.\n");
		exit(1);
	}
	
	client_loc[numbytes] = '\0';
	client_location = atoi(client_loc);
	cout << "Hospital C has received input from client at location " << client_loc << endl;
}

// dijkstra's algo helper function
int find_closest_vertex(map<int, bool> finalized, map<int, float> disFromSource) {
	float minDis = numeric_limits<float>::max();
	int minVertex;

	map<int, map<int, float> > :: iterator it;
	for (it = mapMatrix.begin(); it != mapMatrix.end(); it ++) {
		if (!finalized[it->first] && disFromSource[it->first] < minDis) {
			minDis = disFromSource[it->first];
			minVertex = it->first;
		}
	}
	return minVertex;
}

//dijkstra's algo
float find_shortest_distance() {
	// client location is not in the map or client is at the same location as the hospital
	if (mapMatrix.find(client_location) == mapMatrix.end() || client_location == loc) {
		return -1.0;
	}
	//create a map to record vertices whose distance to source has been finalized
	map<int, bool> finalized;
	//create a map to keep a record of distances from source vertex
	map<int, float> disFromSource;

	// initialize 
	map<int, map<int, float> >:: iterator it;

	for (it = mapMatrix.begin(); it != mapMatrix.end(); it ++) {
		finalized[it->first] = false;
		disFromSource[it->first] = numeric_limits<float>::max();
	} 

	disFromSource[client_location] = 0;
	for (int i = 0; i < mapMatrix.size(); i ++) {
		int closestVertex = find_closest_vertex(finalized, disFromSource);
	// 	cout << "============" << endl;
	// 	cout << "Closet vertex is " << closestVertex << endl;
	// 	cout << "Cur Distance frm source is " << disFromSource[closestVertex] << endl;
	// 	cout << "Dis between vertex 2 and 0 is " << mapMatrix[2][0] << endl;
		finalized[closestVertex] = true;
		if (closestVertex == loc) {
		
			break;
		}
		map<int, float>:: iterator itr;
		for (itr = mapMatrix[closestVertex].begin(); itr != mapMatrix[closestVertex].end(); itr ++) {
			int keyVtx = itr->first;
		//	cout << "destination vertex is " << keyVtx << endl;
			float valDis = itr->second;
		//	cout << " dis is  " << valDis << endl;;
			if (!finalized[keyVtx] && disFromSource[keyVtx] > valDis + disFromSource[closestVertex]) {
				disFromSource[keyVtx] = valDis + disFromSource[closestVertex];
			}
		}
	}
	return disFromSource[loc];


}

void send_message_to_scheduler(void* message) {
	int numbytes;

	if ((numbytes = sendto(udp_sockfd, message, MAXBUFLEN-1 , 0, (struct sockaddr *)&scheduler_addr, sizeof(struct sockaddr))) == -1) { 
		perror("Error: hosptial C fail sendto()\n");
		exit(1);
	}


}

float cal_score(float dis) {
	if (dis <= 0) {
		return dis;
	}
	float avail = (float)(totalCapacity-Occupancy)/(float)totalCapacity;
	float score = 1.0/(dis*(1.1-avail));

	cout << "Hospital C has the score = " << score << endl;

	return score;
}


//edited from beej's tutorial
int udp_port_setup(char* totalCapacity, char* initialOccupancy) {
	struct addrinfo hints, *servinfo, *p; 
	int rv;
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
		if ((udp_sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) { 
			perror("Error: socket"); 
			continue;
		}
		if (bind(udp_sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
				close(udp_sockfd);
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

	
	// addr_len = sizeof scheduler_addr;

	// send initial capacity and occupancy to scheduler
	send_message_to_scheduler(totalCapacity);
	send_message_to_scheduler(initialOccupancy);
	
	// if ((numbytes = sendto(scheduler_sockfd, totalCapacity, MAXBUFLEN-1 , 0,  (struct sockaddr *)&scheduler_addr, sizeof(struct sockaddr))) == -1) { 
	// 	perror("Error: recvfrom\n");
	// 	exit(1);
	// }
	// if ((numbytes = sendto(scheduler_sockfd, initialOccupancy, MAXBUFLEN-1, 0,  (struct sockaddr *)&scheduler_addr, sizeof(struct sockaddr))) == -1) { 
	// 	perror("Error: recvfrom\n");
	// 	exit(1);
	// }

	cout << "Hospital C is up and running using UDP on port " << HospitalC_UDP_PORT << "." << endl;
}
char* float_to_charptr(float f) {
		string scorestr = to_string(f);
	//	cout << "string score is " << scorestr << endl;
		int n = scorestr.length();
		char* scorechar = (char*)malloc((n+1)*sizeof(char));
		strcpy(scorechar, scorestr.c_str());
	//	cout << "char array score is " << scorechar << endl;
		return scorechar;
		//cout << "string format score is " << res << endl;
}

// void float_to_charptr(float f, char* res) {
// 		string scorestr = to_string(f);
// 		int n = scorestr.length();
// 		char scorechar[n+1];
// 		strcpy(scorechar, scorestr.c_str());
// 		res = scorechar;
// }

int main(int argc, char* argv[]) {
	loc = atoi(argv[1]);
	totalCapacity = atoi(argv[2]);
	Occupancy = atoi(argv[3]);

	// set scheduler info
	// memset(&scheduler_addr, 0, sizeof(scheduler_addr));   
	// scheduler_addr.sin_family = AF_UNSPEC;		
	// scheduler_addr.sin_port = htons(33666);
	// scheduler_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	// if ((scheduler_sockfd = socket(AF_UNSPEC, SOCK_DGRAM, 0)) == -1) {
	// 	perror("Error: scheduler socket error\n");
	// 	exit(1);
	// }

	bootup();
	set_up_scheduler_sock();
	udp_port_setup(argv[2], argv[3]);
	
	cout << "Hospital C has total capacity " << totalCapacity << "and initial occupancy " << Occupancy << "." << endl;

	
//	recieve_client_info();
	while (true) {
		recieve_client_info();
		if (mapMatrix.find(client_location) == mapMatrix.end()) {
			cout << "HospitalC does not have the location " << client_location << " in map" << endl;
			
			char tmp[] = "location not found";
			char* meg  = tmp;
			send_message_to_scheduler(meg);
			cout << "Hospital C has sent \"location not found\" to the Scheduler" << endl;
			continue;
		}
		float minDistance = find_shortest_distance();
		cout << "Hospital C has found the shortest path to client, distance = " << minDistance << endl;
		float score = cal_score(minDistance);
		// char* scoreMessage;
		// char* disMessage;
		char* disMessage = float_to_charptr(minDistance);
		char* scoreMessage = float_to_charptr(score);

	//	set_up_scheduler_sock();
		send_message_to_scheduler(disMessage);
		send_message_to_scheduler(scoreMessage);
		cout << "Hospital C has sent score = " << scoreMessage << " and distance = " << disMessage << " to the Scheduler" << endl;
	}
	

}
