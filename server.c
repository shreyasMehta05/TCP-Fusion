#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<time.h>
#include<sys/time.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<fcntl.h>


#define PORT 8080
#define _BUFFER_SIZE_ 1000000
#define _SEGMENT_SIZE_ 3
#define _TIMEOUT_ 100

#define _ACKNOWLEDGEMENT_SIZE_ sizeof(int)
#define _ID_SIZE_ sizeof(int)
#define _TIME_SIZE_ sizeof(time_t)
#define _INVALID_SEGMENT_ -100

#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define RESET "\033[0m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"


#define _TEST_ 1

typedef struct DataSegment
{
    int id; char* data;
    struct timeval init_time;
} DataSegment;

DataSegment* createDataSegment(int id, char* data)
{
    DataSegment* newSegment = (DataSegment*)malloc(sizeof(DataSegment));
    if(newSegment == NULL)
    {
        fprintf(stderr, RED"Memory allocation failed\n"RESET);
        exit(EXIT_FAILURE);
    }
    gettimeofday(&(newSegment->init_time), NULL); 
    newSegment->id = id;
    newSegment->data = data;
    return newSegment;
}

typedef struct Node{
    DataSegment* segment;
    struct Node* next;
}Node;

typedef struct Node* SegmentList;

Node* createNode(int id,char* data){
    Node* newNode = (Node*)malloc(sizeof(Node));
    if(newNode == NULL)
    {
        fprintf(stderr, RED"Memory allocation failed\n"RESET);
        exit(EXIT_FAILURE);
    }
    newNode->segment = createDataSegment(id, data);
    newNode->next = NULL;
    return newNode;
}

Node* createNodefromSegment(DataSegment* segment){
    Node* newNode = (Node*)malloc(sizeof(Node));
    if(newNode == NULL)
    {
        fprintf(stderr, RED"Memory allocation failed\n"RESET);
        exit(EXIT_FAILURE);
    }
    newNode->segment = segment;
    newNode->next = NULL;
    return newNode;
}

void insertNode(SegmentList Head, DataSegment* segment){
    bool alreadyPresent = false;
    Node* temp = Head->next;
    while(temp){
        if(temp->segment->id == segment->id){
            alreadyPresent = true;
            break;
        }
        temp = temp->next;
    }
    if(!alreadyPresent){
        Node* newNode = createNodefromSegment(segment);
        Node* first = Head->next;
        Head->next = newNode;
        newNode->next = first;
    }
}
void printList(SegmentList Head){
    Node* temp = Head->next;
    if(!temp){
        printf("____\n");
        return;
    }
    while(temp){
        printf("%d ",temp->segment->id);
        temp = temp->next;
    }
    printf("\n");
}

bool allRecieved(int numberOfSegments, bool* received){
    // check if all segments have been received
    for(int i = 0; i < numberOfSegments; i++){
        if(!received[i]){
            return false;
        }
    }
    return true;
}

char* generateDataString(char* string, int iteration, int numberOfSegments){
    char* dataString = (char*)calloc(_BUFFER_SIZE_, sizeof(char));
    int start = iteration * _SEGMENT_SIZE_;
    int end;
    if(iteration == numberOfSegments - 1){
        end = start + strlen(string) % _SEGMENT_SIZE_;
    }
    else{
        end = start + _SEGMENT_SIZE_;
    }
    int k = 0;
    for(int i=start;i<end;i++){
        dataString[k] = string[i];
        k++;
    }
    dataString[k] = '\0';
    return dataString;
}



void resendSegment(int serverSocket, SegmentList Head, struct sockaddr_in serverAddress,int* packet){
    Node* temp = Head->next;
    while(temp){
        struct timeval tv;
        gettimeofday(&tv, NULL);
        if(tv.tv_usec - temp->segment->init_time.tv_usec > _TIMEOUT_){
            printf("No ACK received. Resend segment %d\n", temp->segment->id);
            *packet = temp->segment->id;
            break;
        }
        temp = temp->next;
    }
    // return packet;
}


int generateSteralizedSize(DataSegment* currentSegment){
    int dataSize = strlen(currentSegment->data) + 1;
    int serializedSize = _ID_SIZE_ + _TIME_SIZE_ + dataSize;
    return serializedSize;
}
char* generateSteralizedData(DataSegment* currentSegment){
    int serializedSize = generateSteralizedSize(currentSegment);
    char* serializedData = (char*)malloc(serializedSize);
    if(serializedData == NULL){
        fprintf(stderr, RED"Memory allocation failed\n"RESET);
        exit(EXIT_FAILURE);
    }
    char* ptr = serializedData;
    memcpy(ptr, &currentSegment->id, _ID_SIZE_);
    ptr += _ID_SIZE_;
    memcpy(ptr, &currentSegment->init_time, _TIME_SIZE_);
    ptr += _TIME_SIZE_;
    strcpy(ptr, currentSegment->data);
    return serializedData;
}

void removeSegment(SegmentList Head, int ack){
    if(Head->next){
        Node* curr = Head->next;
        Node* prev = Head;
        while(curr){
            if(curr->segment->id == ack){
                prev->next = curr->next;
                free(curr);
                curr = prev->next;
            }
            if(curr == NULL){
                break;
            }
            curr = curr->next;
            prev = prev->next;
        }
    }
}


int main(){

    // udp socket creation
    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(serverSocket < 0){
        fprintf(stderr, RED"Socket creation failed\n"RESET);
        exit(EXIT_FAILURE);
    }
    // server address
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    // bind the socket to the server address
    if(bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        fprintf(stderr, RED"Binding failed\n"RESET);
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    printf(YELLOW"Server is running...\n"RESET);

    // receive the number of segments
    int numberOfSegments;
    ssize_t recvBytes = recvfrom(serverSocket, &numberOfSegments, sizeof(numberOfSegments), 0, (struct sockaddr*)&clientAddress, &clientAddressLength);
    if(recvBytes < 0){
        fprintf(stderr, RED"Receive failed\n"RESET);
        exit(EXIT_FAILURE);
    }

    // "numberOfSegments" contains the integer value sent by the client

    char** arrayOfSegments = (char**)malloc(numberOfSegments * sizeof(char*)); 
    bool* received = (bool*)calloc(numberOfSegments, sizeof(bool));
    // for each segment, receive the data and send an acknowledgement
   
    while(!allRecieved(numberOfSegments, received)){
        // receive the segment from the client
        char* receivedData = (char*)calloc(_BUFFER_SIZE_, sizeof(char));
        int recvBytes = recvfrom(serverSocket, receivedData, _BUFFER_SIZE_, 0, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if(recvBytes < 0){
            fprintf(stderr, RED"Receive failed\n"RESET);
            continue;
        }

        // extract the string and add it to the string array
        int id;
        time_t initTime;
        char* dataString;

        char* ptr = receivedData;
        memcpy(&id, ptr, _ID_SIZE_);
        ptr += _ID_SIZE_;
        memcpy(&initTime, ptr, _TIME_SIZE_);
        ptr += _TIME_SIZE_;
        dataString = strdup(ptr);

        received[id] = true;
        arrayOfSegments[id] = strdup(dataString);

        // send an acknowledgement back to the client
        int acknowledgement = id;
        // for testing purposes, we are not sending the acknowledgement for every segment
        #ifndef _TEST_
        static int cnt = 0;
        if(acknowledgement%3==0 && cnt < 3){
            cnt++;
            continue;
        }
        #endif
        ssize_t sendBytes = sendto(serverSocket, &acknowledgement, _ACKNOWLEDGEMENT_SIZE_, 0, (struct sockaddr*)&clientAddress, clientAddressLength);
        if(sendBytes < 0){
            fprintf(stderr, RED"Could not send ACK\n"RESET);
            continue;
        }

        printf(GREEN"ACK sent for segment %d\n"RESET, id);
    }
    char* string = (char*)calloc(_BUFFER_SIZE_, sizeof(char));
    for(int i = 0; i < numberOfSegments; i++){
        strcat(string, arrayOfSegments[i]);
    }
    printf("Received string: %s\n"RESET, string);

     // free the allocated memory
    for(int i = 0; i < numberOfSegments; i++){
        free(arrayOfSegments[i]);
    }
    free(arrayOfSegments);
    free(received);
    free(string);
    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////
    ////////////////// PART-B Sending from Server to CLient        ///////////////////
    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////
    int flags = fcntl(serverSocket, F_GETFL, 0);
    if(flags == -1){
        fprintf(stderr, RED"fcntl\n"RESET);
        exit(EXIT_FAILURE);
    }
    flags  = flags | O_NONBLOCK;
    // set the socket to non-blocking
    if(fcntl(serverSocket, F_SETFL, flags) == -1){
        fprintf(stderr, RED"fcntl\n"RESET);
        exit(EXIT_FAILURE);
    }

    string= (char*)malloc(_BUFFER_SIZE_);
    if(string == NULL){
        fprintf(stderr, RED"Memory allocation failed\n"RESET);
        exit(EXIT_FAILURE);
    }
    printf(YELLOW"Enter the string: "BLUE);
    fgets(string, _BUFFER_SIZE_, stdin);
    printf(RESET);
    int stringLength = strlen(string);
    string[strlen(string) - 1] = '\0';
    // printf("String: %s\n", string);
    // send the number of segments to the client
    // numberOfSegments = strlen(string) / _SEGMENT_SIZE_;
    // if(strlen(string) % _SEGMENT_SIZE_){
    //     numberOfSegments++;
    // }
    numberOfSegments = stringLength/_SEGMENT_SIZE_ + (stringLength%_SEGMENT_SIZE_ == 0 ? 0 : 1);


    // send the number of segments to the client
    int ret = sendto(serverSocket, &numberOfSegments, sizeof(numberOfSegments), 0, (struct sockaddr*)&clientAddress, clientAddressLength);
    if(ret < 0){
        fprintf(stderr, RED"Could not send message\n"RESET);
        exit(EXIT_FAILURE);
    }

    // sent array to keep track of the segments that have been sent
    bool* sent = (bool*)calloc(numberOfSegments, sizeof(bool));
    SegmentList Head = createNode(_INVALID_SEGMENT_,"Shreyas");

    for(int i=0;i<numberOfSegments;i++){
        if(i!=0){
            resendSegment(serverSocket, Head, serverAddress,&i);
        }
        // if the segment has already been sent, skip it
        if(sent[i]){
            continue;
        }
        // // create the substring
        char* dataString = generateDataString(string, i, numberOfSegments);
        // create the segment
        DataSegment* currentSegment = createDataSegment(i, dataString);
        int serializedSize = generateSteralizedSize(currentSegment);
        char* serializedData = generateSteralizedData(currentSegment);
        // send the segment to the client

        int ret = sendto(serverSocket, serializedData, serializedSize, 0, (struct sockaddr*)&clientAddress, clientAddressLength);
        if(ret < 0){
            fprintf(stderr, RED"Could not send segment\n"RESET);
            exit(EXIT_FAILURE);
        }
        insertNode(Head, currentSegment); // insert the segment into the linked list
        free(dataString); // free the allocated memory
        free(serializedData); // free the allocated memory

        // check for the acknowledgement
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000; // 50 milliseconds
        // set the timeout
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        int selectStatus = select(serverSocket + 1, &readfds, NULL, NULL, &timeout);
        // check if the select was successful -> select is used to check if the socket is ready to read
        if(selectStatus == -1){
            fprintf(stderr, RED"Error in select\n"RESET);
            exit(EXIT_FAILURE);
        }
        else if(selectStatus == 0){
            // if the select timed out, continue which will help in resending the segment
            continue;
        }
        else{
            clientAddressLength = sizeof(clientAddress);
            int ack; // acknowledgement id
            int recvBytes = recvfrom(serverSocket, &ack, _ACKNOWLEDGEMENT_SIZE_, 0, (struct sockaddr*)&clientAddress, &clientAddressLength);
            if(recvBytes < 0){
                fprintf(stderr, RED"Receive failed\n"RESET);
                exit(EXIT_FAILURE);
            }
            printf(GREEN"ACK received for segment %d\n"RESET, ack);
            sent[ack] = true;
            // remove the segment from the linked list
            removeSegment(Head, ack);
        }

    }

    if(Head->next){
        // if there are still segments in the linked list, resend them
        Node* curr = Head->next;
        while(curr){
            DataSegment* curSegment = curr->segment;
            int i = curSegment->id;

            int dataSize = strlen(curSegment->data) + 1;
            int serializedSize = _ID_SIZE_ + _TIME_SIZE_ + dataSize;
            char* serializedData = (char*)malloc(serializedSize);
            if(serializedData == NULL){
                fprintf(stderr, RED"Memory allocation failed\n"RESET);
                exit(EXIT_FAILURE);
            }

            char* ptr = serializedData;
            memcpy(ptr, &curSegment->id, _ID_SIZE_);
            ptr += _ID_SIZE_;
            memcpy(ptr, &curSegment->init_time, _TIME_SIZE_);
            ptr += _TIME_SIZE_;
            strcpy(ptr, curSegment->data);

            int ret = sendto(serverSocket, serializedData, serializedSize, 0, (struct sockaddr*)&clientAddress, clientAddressLength);
            if(ret < 0){
                fprintf(stderr, RED"Could not send segment\n"RESET);
                exit(EXIT_FAILURE);
            }

            curr = curr->next;
        }
    }
    
    if(string)
    free(string);
    close(serverSocket);
    return 0;
}