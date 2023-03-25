// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <fstream> 
#include <algorithm>

using namespace std;
	
#define PORT	 8081
#define MAXLINE 1537
#define BUFFER_SIZE 2000
#define NUM_OF_COMPS 20

vector<int> lfr(NUM_OF_COMPS+1, 0);
vector<string> files(NUM_OF_COMPS+1, "");

// for Go_Back_N protocol
#define RWS 1

// Part 1
//int lfr = 0;

vector<int> aquireFrameInfo(char buffer[]) {
	cout << "fffffffffffff" << buffer << endl;
	string infoS[3] = {""};
	int start = 0;
	for(int j = 0; j < 3; j++) 
		for(int i = start; i < BUFFER_SIZE; i++) {
			if(buffer[i] == ' ') {
				start = i+1;
				break;
			}
			infoS[j] += buffer[i];
		}
	return vector<int>{stoi(infoS[0]), stoi(infoS[1]), stoi(infoS[2])};
}

void copyFrameContent(char buffer[], int dest, int size) {
	for(int i = 0; i < size; i++) 
		files[dest] += buffer[i];
	
	files[dest] += '\n';
}
	
// Driver code
int main() {
	int sockfd;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in servaddr, cliaddr;
	cout << "PC B is Running...\n";
		
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
		
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
		
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);
		
	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
		
	socklen_t len;
	buffer[2048] = '\0';
	len = sizeof(cliaddr);

	// for Part 1(simultaneous exit)
	// recvfrom(sockfd, (char *)buffer, BUFFER_SIZE,
	// 			MSG_WAITALL, ( struct sockaddr *) &cliaddr,
	// 			(socklen_t*)&len);

	// int numOfFrames = atoi(buffer);
	// cout << numOfFrames << endl;
	bool hi = false;

	while(true) {
		memset(&buffer, '$', sizeof(buffer)); 
		
		recvfrom(sockfd, (char *)buffer, BUFFER_SIZE,
				MSG_WAITALL, ( struct sockaddr *) &cliaddr,
				(socklen_t*)&len);

		vector<int> packetInfo = aquireFrameInfo(buffer);
		int frameIndex = packetInfo[0];
		int destination = packetInfo[1];
		int packetSize = packetInfo[2];

		cout << "received packet sequence number is " << frameIndex << endl;
		// cout << "Packet content is: \n";
		// cout << buffer << endl;

		
		if(frameIndex <= lfr[destination]+1) {
			int n = to_string(frameIndex).length() + to_string(destination).length()+2;
			n += to_string(n).length();
			char *ack =  (char*)((to_string(frameIndex)+" " + to_string(destination)+" "+to_string(n)).c_str());

			sendto(sockfd, ack, strlen(ack),
				MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
					len);
			if(frameIndex == lfr[destination]+1) {
				lfr[destination]++;
				copyFrameContent(buffer, destination, packetSize);
				cout << "ack sent for packet " << frameIndex << endl;
			} else 
				cout << "ack resent for packet " << frameIndex << endl;

		} 
		cout << endl;

		// for part one, uncomment for simultaneous exit
		// if(frameIndex > numOfFrames)
		// 	break;
		ofstream MyFile("filename1.txt");
  		MyFile << files[1];
  		MyFile.close();
	}

	return 0;
}