#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string>
#include <string.h>
#include <sstream> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <thread>
#include <csignal>
#include <fstream>
#include <sys/time.h>
#include <algorithm>
#include <queue>
#include <chrono>
using namespace std::chrono;
using namespace std;

#define PORT     8080 
#define MAXLINE 1536
#define BUFFER_SIZE 2000
#define SWS 4
#define INTERVAL 200


int lar = 0, lfs = 0;
pthread_t tid;
int sockfd; 
char buffer[BUFFER_SIZE]; 
struct sockaddr_in servaddr; 
vector<string> frames;
vector<int> unregisteredAcks;
bool lastAckRegistered = false;
int computerNumber;
int numOfTimeOuts = 0, numOfRecs = 0;
int currSWS = SWS;

// Part 2
vector<int> aquireFrameInfo(char buffer[]) {
	string infoS[2] = {""};
	int start = 0;
	for(int j = 0; j < 2; j++) 
		for(int i = start; i < MAXLINE; i++) {
			if(buffer[i] == ' ') {
				start = i+1;
				break;
			}
			infoS[j] += buffer[i];
		}

	return vector<int>{stoi(infoS[0]), stoi(infoS[1])};
}

string readFile(string fileName) {
    string line;
    string input = "";
    ifstream MyReadFile(fileName);
    while(getline(MyReadFile, line)) {
        input += line + "\n";
    }
    MyReadFile.close(); 
    input.pop_back();
    return input;
}

void alarm_handler(int a) {
    cout << "time out occured!\n\n";
    numOfTimeOuts++;
    if(numOfTimeOuts >= 1)
        currSWS /= 2;
    currSWS = currSWS == 0 ? 1 : currSWS;
    unregisteredAcks.clear();
    lfs = lar;
}

void* recieveAck(void* args) {
    while(true) {
        struct itimerval it_val;
        it_val.it_value.tv_sec = INTERVAL/1000;
        it_val.it_value.tv_usec = (INTERVAL*1000) % 1000000;   
        it_val.it_interval = it_val.it_value;
        memset(&buffer, 0, sizeof(buffer)); 
        signal(SIGALRM, alarm_handler);
        setitimer(ITIMER_REAL, &it_val, NULL);
        recvfrom(sockfd, (char *)buffer, BUFFER_SIZE+1,
			MSG_WAITALL, (struct sockaddr *) &servaddr,
			(socklen_t*)&servaddr);

        vector<int> frameInfo = aquireFrameInfo(buffer);
        int SN = frameInfo[0];
        int dest = frameInfo[1];

        // Part 2
        if(dest != computerNumber)
            continue;

        cout << "ack received for packet " << atoi(buffer) << endl << endl;
        if(SN == frames.size() && lar+1 == frames.size()) {
            lastAckRegistered = true;
            break;
        }
        if(lar+1 == SN) {
            cout << "sliding the window to the right...\n";
            if(currSWS < SWS) currSWS++;
            numOfTimeOuts = 0;
            sort(unregisteredAcks.begin(), unregisteredAcks.end(), greater <>());
            lar++;
            for(int i = unregisteredAcks.size()-1; i >= 0; i--) 
                if(lar+1 == unregisteredAcks[i]) {
                    lar++;
                    unregisteredAcks.pop_back();
                }
        } else {
            unregisteredAcks.push_back(SN);
        }
    }  
    // char *transmitComplete =  (char*)(to_string(frames.size()+1).c_str());
    // sendto(sockfd, transmitComplete, BUFFER_SIZE+1, 
    //         MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
    //             sizeof(servaddr));
    return nullptr;
}

vector<string> breakIntoFrames(int frameSize, string input) {
    vector<string> frames;
    int i = 0;
    for(i = 0; i < input.size(); i += frameSize)
        frames.push_back(input.substr(i, frameSize - 1));
        
    if (i < input.size() - 1){
        i -= frameSize;
        frames.push_back(input.substr(i, input.size() - i - 1));
    }
    return frames;
}


void writeSendLog(int lfs,char char_array[]) {
    cout << "sending packet with sequence number " << lfs+1 << endl;
    // cout << "packet content is:\n";
    // cout << char_array << endl;
}

int main(int argc, char** argv) { 

    computerNumber = atoi(argv[1]);
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    } 
    
    memset(&servaddr, 0, sizeof(servaddr)); 
        
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n;
    socklen_t len; 
    cout << ("PC A number "+ to_string(computerNumber) +" is Running...\n");

    string input = readFile("sample/sample.txt");
    frames = breakIntoFrames(MAXLINE, input);

    cout << frames.size() << endl;


    // for Part 1(simultaneous exit)
    // if(computerNumber == 1) {
    //     char num_char[10 + sizeof(char)];
    //     std::sprintf(num_char, "%lu", frames.size());

    //     sendto(sockfd, num_char, BUFFER_SIZE+1, 
    //         MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
    //             sizeof(servaddr)); 
    // }

    auto start = high_resolution_clock::now();
    pthread_create(&tid, nullptr, recieveAck, nullptr);

    // for Part 1(simultaneous exit)
    while(!lastAckRegistered) {
        while(lfs >= frames.size());
        if(lfs-lar < currSWS) {
            string SNS = to_string(lfs+1);
            int n = to_string(computerNumber).length()+SNS.length()+frames[lfs].length()+3;
            n += to_string(n).length();
            char char_array[n + 1];
            strcpy(char_array, (SNS+" "+to_string(computerNumber)+" "+to_string(n)+" "+frames[lfs]).c_str());
            writeSendLog(lfs, char_array);
            sendto(sockfd, char_array, sizeof(char_array), 
                MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                    sizeof(servaddr)); 
            lfs++;
        }
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    pthread_join(tid, nullptr);     
    cout << "Duration of transmission for a 1MB file sample.txt: " << duration.count() << " microseconds" << endl; 
    close(sockfd); 
    return 0; 
}