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
#define _IP_ADDRESS_ "127.0.0.1"
#define _INVALID_SEGMENT_ -100

#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define RESET "\033[0m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"


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
    for(int i=0;i<numberOfSegments;i++){
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
    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(serverSocket < 0){
        fprintf(stderr, RED"Socket creation failed\n"RESET);
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(serverSocket, F_GETFL);
    if(flags == -1){
        fprintf(stderr, RED"fcntl\n"RESET);
        exit(EXIT_FAILURE);
    }
    flags = flags | O_NONBLOCK;
    if(fcntl(serverSocket, F_SETFL, flags) == -1){
        fprintf(stderr, RED"fcntl\n"RESET);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr(_IP_ADDRESS_);


    char* string = (char*)malloc(_BUFFER_SIZE_*sizeof(char));
    if(string == NULL){
        fprintf(stderr, RED"Memory allocation failed\n"RESET);
        exit(EXIT_FAILURE);
    }
    printf(YELLOW"Enter the string: "RESET);
    fgets(string, _BUFFER_SIZE_, stdin);
    int stringLength = strlen(string);
    string[stringLength-1] = '\0';

    int numberOfSegments = stringLength/_SEGMENT_SIZE_ + (stringLength%_SEGMENT_SIZE_ == 0 ? 0 : 1);
    // int serverAddressLength = sizeof(serverAddress);
    int returnStatus = sendto(serverSocket, &numberOfSegments, sizeof(numberOfSegments), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));   
    if(returnStatus < 0){
        fprintf(stderr, RED"Failed to send number of segments\n"RESET);
        exit(EXIT_FAILURE);
    }
    bool* sent = (bool*)calloc(numberOfSegments, sizeof(bool)); 
    SegmentList Head = createNode(_INVALID_SEGMENT_,"Shreyas");

    for(int i=0;i<numberOfSegments;i++){
        if(i){
            resendSegment(serverSocket, Head, serverAddress,&i);
        }
        if(sent[i]){
            continue;
        }
        char* dataString = generateDataString(string, i, numberOfSegments);

        DataSegment* currentSegment = createDataSegment(i, dataString);
        int serializedSize = generateSteralizedSize(currentSegment);
        char* serializedData = generateSteralizedData(currentSegment);

        int ret = sendto(serverSocket, serializedData, serializedSize, 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
        if(ret < 0){
            fprintf(stderr, RED"Could not send segment\n"RESET);
            exit(EXIT_FAILURE);
        }
        insertNode(Head, currentSegment);
        // free(dataString);
        free(serializedData);

        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        int selectStatus = select(serverSocket + 1, &readfds, NULL, NULL, &timeout);
        if(selectStatus == -1){
            fprintf(stderr, RED"Error in select\n"RESET);
            exit(EXIT_FAILURE);
        }
        else if(selectStatus == 0){
            continue;
        }
        else{
            int sizeofServerAddress = sizeof(serverAddress);
            int ack;
            int recvBytes = recvfrom(serverSocket, &ack, _ACKNOWLEDGEMENT_SIZE_, 0, (struct sockaddr*)&serverAddress,(socklen_t*) &sizeofServerAddress);
            if(recvBytes < 0){
                fprintf(stderr, RED"Receive failed\n"RESET);
                exit(EXIT_FAILURE);
            }
            printf(GREEN"ACK received for segment %d\n"RESET, ack);
            sent[ack] = true;

            removeSegment(Head, ack);
        }

    }
    if(Head->next){
        Node* curr = Head->next;
        while(curr){
            DataSegment* curSegment = curr->segment;
            // int i = curSegment->id;
           
            // char* dataString = strdup(curSegment->data);

            int dataSize = strlen(curSegment->data) + 1;
            int serializedSize = _ID_SIZE_ + _TIME_SIZE_ + dataSize;
            char* serializedData = (char*)malloc(serializedSize);
            memset(serializedData, 0, serializedSize);
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

            int ret = sendto(serverSocket, serializedData, serializedSize, 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
            if(ret < 0){
                fprintf(stderr, RED"Could not send segment\n"RESET);
                exit(EXIT_FAILURE);
            }
            free(serializedData);

            curr = curr->next;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    ////////////////// PART-B Receiving the final string from the server ///////////////
    ///////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////
    flags = fcntl(serverSocket, F_GETFL, 0);
    if(flags == -1){
        fprintf(stderr, RED"fcntl\n"RESET);
        exit(EXIT_FAILURE);
    }
    flags  = flags & ~O_NONBLOCK;
    if(fcntl(serverSocket, F_SETFL, flags) == -1){
        fprintf(stderr, RED"fcntl\n"RESET);
        exit(EXIT_FAILURE);
    }

    printf(YELLOW"Waiting for the final string\n"RESET);



   numberOfSegments = 0;
    
   int serverAddressSize = sizeof(serverAddress);
    ssize_t recvBytes = recvfrom(serverSocket, &numberOfSegments, sizeof(numberOfSegments), 0, (struct sockaddr*)&serverAddress,(socklen_t*) &serverAddressSize);
    if(recvBytes < 0){
        fprintf(stderr, RED"Receive failed\n"RESET);
        exit(EXIT_FAILURE);
    }

    // "numberOfSegments" contains the integer value sent by the client

    char** arrayOfSegments = (char**)malloc(numberOfSegments * sizeof(char*));
    bool* received = (bool*)calloc(numberOfSegments, sizeof(bool));

    while(!allRecieved(numberOfSegments, received)){
        char* receivedData = (char*)calloc(_BUFFER_SIZE_, sizeof(char));
        int recvBytes = recvfrom(serverSocket, receivedData, _BUFFER_SIZE_, 0, (struct sockaddr*)&serverAddress, (socklen_t*)&serverAddressSize);
        if(recvBytes < 0){
            fprintf(stderr, RED"Receive failed\n"RESET);
            continue;
        }

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

        int acknowledgement = id;

        ssize_t sendBytes = sendto(serverSocket, &acknowledgement, _ACKNOWLEDGEMENT_SIZE_, 0, (struct sockaddr*)&serverAddress, serverAddressSize);
        if(sendBytes < 0){
            fprintf(stderr, RED"Could not send ACK\n"RESET);
            continue;
        }

        printf(GREEN"ACK sent for segment %d\n"RESET, id);
    }

    char* finalString = (char*)calloc(_BUFFER_SIZE_, sizeof(char));
    for(int i=0;i<numberOfSegments;i++){
        strcat(finalString, arrayOfSegments[i]);
    }
    printf("Final string: %s\n", finalString);



    while(Head->next){
        Node* temp = Head->next;
        Head->next = temp->next;
        free(temp->segment->data);
        free(temp->segment);
        free(temp);
    }

    for(int i=0;i<numberOfSegments;i++){
        free(arrayOfSegments[i]);
    }
    free(arrayOfSegments);
    free(received);
    free(finalString);
    free(string);
    close(serverSocket);

    return 0;
}