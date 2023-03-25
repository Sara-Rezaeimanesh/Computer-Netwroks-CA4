#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <iostream>
#include <random>
#include <queue>
#include <chrono>
#include <thread>

using namespace std;
using std::this_thread::sleep_for;
	
#define PORT1	 8080
#define PORT2	 8081
#define MAXLINE 2000
#define PACKET_BUFFER_SIZE 20
#define MIN 11
#define MAX 20


std::random_device rd;
std::mt19937 gen(rd());
pthread_t tid[3];
pthread_mutex_t lock;

int sockfd1, sockfd2;
char buffer[MAXLINE], recA[MAXLINE], recB[MAXLINE];
struct sockaddr_in servaddr1, cliaddr1;
struct sockaddr_in servaddr2, cliaddr2;
int sent = 0;
int numOfFrames = 0;
int dropProbability = 0;
std::uniform_int_distribution<> distrib(1, 10);

string convertToString(char buff[], int size) {
	string s = "";
	for(int i = 0; i < size; i++)
		s += buff[i];
	return s;
}

struct Packet {
	Packet(string _body, bool _dest, int frameSN, int _DN): dest(_dest), SN(frameSN), DN(_DN), body(_body) {};
	string body;
	bool dest; //0 for A and 1 for B
	int SN; // sequence number
	int DN; // destination number
};

queue<Packet> packetsBuffer;

void copyBuffer(char buff[], char temp[]) {
	for(int i = 0; i< MAXLINE; i++) 
		temp[i] = buff[i];
}

void updateDropProb() {
    dropProbability = packetsBuffer.size() >= MAX ? 10 :
					 packetsBuffer.size() - MIN > 5 ? (packetsBuffer.size() - MIN)/2+5 :
					 (packetsBuffer.size() - MIN)/2;
}

vector<int> aquireFrameInfo(char buffer[]) {
	try{
		string infoS[3] = {""};
		int start = 0;
		for(int j = 0; j < 3; j++) 
			for(int i = start; i < MAXLINE; i++) {
				if(buffer[i] == ' ') {
					start = i+1;
					break;
				}
				infoS[j] += buffer[i];
			}

		return vector<int>{stoi(infoS[0]), stoi(infoS[1]), stoi(infoS[2])};
	} catch(...) {
		cerr << "this is the buffer that caused problems:\n" << buffer << endl;
		abort();
	}
	
}

void* sendPackets(void* args) {
	while(true) {
		cerr << "size is " << packetsBuffer.size() << endl;
		while(packetsBuffer.empty());
		Packet p = packetsBuffer.front();
		if(p.dest) {
			std::cout << "forwarding packet " << p.SN << " from A" << p.DN << " to B...\n";
			sendto(sockfd2, (char*)p.body.c_str(), MAXLINE,
				MSG_CONFIRM, (const struct sockaddr *) &servaddr2,
				sizeof(cliaddr2));
		} else {
			std::cout << "forwarding ack for packet " << p.SN << " to A" << p.DN << "...\n";
			// if(p.SN == numOfFrames+1)
			// 	break;
			sendto(sockfd1, (char*)p.body.c_str(), MAXLINE, 
        		MSG_CONFIRM, (const struct sockaddr *) &cliaddr1,  
            	sizeof(cliaddr1)); 			
		}
		packetsBuffer.pop();

		cout << "end of forward\n";
		sent++;
		sleep_for(std::chrono::milliseconds(2));
	}
	return nullptr;
}

void addPacket(string mssg1, string mssg2, string mssg3, int sender, char recBuffer[]) {
	char* temp = new char[MAXLINE];
	memcpy(temp, recBuffer, MAXLINE-1);
	updateDropProb();
	vector<int> frameInfo = aquireFrameInfo(temp);
	int frameSN = frameInfo[0];
	int dest = frameInfo[1];
	int size = frameInfo[2];

	int loss = distrib(gen);
	if(loss <= dropProbability) {
		std::cout << mssg3 << frameSN << " from source A" << dest << endl;
		return;
	}

	if(packetsBuffer.size() < PACKET_BUFFER_SIZE) {
		if(!sender)
			std::cout << mssg1 << frameSN << mssg2 << dest << " to queue..." << endl;
		else 
			std::cout << mssg1 << frameSN << " of source A" << dest  << mssg2 << " to queue..." << endl;

		pthread_mutex_lock(&lock);
		packetsBuffer.push(Packet(convertToString(temp, size), !sender, frameSN, dest));
		pthread_mutex_unlock(&lock);
	} else 
		std::cout << mssg3 << frameSN << " from source A" << dest << endl;
}

void* receiveA(void* args) {
	socklen_t len1 = sizeof(cliaddr1);
	while(true) {
		memset(&recA, 0, sizeof(recA)); 
		recvfrom(sockfd1, (char *)recA, MAXLINE,
			MSG_WAITALL, ( struct sockaddr *) &cliaddr1,
			(socklen_t*)&len1);
		
		addPacket("Adding packet ", " from A",
		 "dropping data packet ", 0, recA);
	}
	return nullptr;
}

void* receiveB(void* args) {
	socklen_t len2 = sizeof(cliaddr2);
	while(true) {
		memset(&recB, 0, sizeof(recB)); 
		recvfrom(sockfd2, recB, MAXLINE,
				MSG_WAITALL, ( struct sockaddr *) &servaddr2,
				(socklen_t*)&len2);
		
		addPacket("Adding ack for packet ", " from B ",
		 "dropping ack packet for packet ", 1, recB);
	}
	return nullptr;
}
	
// Driver code
int main() {
	// setting up sockets
	if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    std::cout << "Router is Running..." << endl;
	
		
	if((sockfd1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

    if((sockfd2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	memset(&servaddr1, 0, sizeof(servaddr1));
	memset(&cliaddr1, 0, sizeof(cliaddr1));

    memset(&servaddr2, 0, sizeof(servaddr2));
	memset(&cliaddr2, 0, sizeof(cliaddr2));
		
	servaddr1.sin_family = AF_INET; // IPv4
	servaddr1.sin_addr.s_addr = INADDR_ANY;
	servaddr1.sin_port = htons(PORT1);

    servaddr2.sin_family = AF_INET; // IPv4
	servaddr2.sin_addr.s_addr = INADDR_ANY;
	servaddr2.sin_port = htons(PORT2);
		
	if (bind(sockfd1, (const struct sockaddr *)&servaddr1, sizeof(servaddr1)) < 0)
	{
		perror("bind1 failed");
		exit(EXIT_FAILURE);
	}


	// getting number of packets from A
	socklen_t len1 = sizeof(cliaddr1), len2 = sizeof(cliaddr2), n;
	buffer[1537] = '\0';

	memset(&buffer, 0, sizeof(buffer)); 

	// for Part 1(simultaneous exit)
	// recvfrom(sockfd1, (char *)buffer, 1537,
	// 			MSG_WAITALL, ( struct sockaddr *) &cliaddr1,
	// 			(socklen_t*)&len1);

	// sendto(sockfd2, buffer, strlen(buffer),
	// 		MSG_CONFIRM, (const struct sockaddr *) &servaddr2,
	// 			len2);

	// std::cout << buffer << endl;
	numOfFrames = atoi(buffer);

	// making threads for sending and receiving
	pthread_create(&tid[0], nullptr, sendPackets, nullptr);
	pthread_create(&tid[1], nullptr, receiveA, nullptr);
	pthread_create(&tid[2], nullptr, receiveB, nullptr);

	// waiting for send tread to close
	pthread_join(tid[0], NULL);
	pthread_cancel(tid[1]);
	pthread_cancel(tid[2]);       

	close(sockfd2); 
	return 0;
}