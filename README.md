# **TCP-Fusion** 💻⚡
![Waketime](https://img.shields.io/badge/Waketime-17%20hrs%2025%20mins-blueviolet?style=flat&labelColor=black&logo=clock&logoColor=white)


---

## **Author** ✍️
**Shreyas Mehta** 

---

## **Simulating TCP Functionality over UDP** 🌐🔄

### **Description**
- This project was part of the `Operating Systems and Networks` course at the **IIIT Hyderabad** 🏫.
- This project implements a basic `client-server` system that simulates `TCP`-like behavior using `UDP`. It focuses on data transmission reliability by organizing data into smaller chunks, assigning sequence numbers, and ensuring successful delivery through retransmission mechanisms. The client and server communicate via `UDP` sockets, with the client acting as the sender and the server as the receiver.
- The client breaks the input data into smaller segments (chunks), each assigned a unique `sequence number`. The server receives and reassembles these chunks in order based on their sequence numbers to reconstruct the original input data. The client sends the total number of chunks to the server for proper reassembly.
- After sending a chunk, the client waits for an acknowledgment (`ACK`) from the server. If no `ACK` is received within a defined `timeout period` (0.1 seconds), the client retransmits the same chunk. The system employs a `non-blocking` approach to allow for continuous sending of chunks without waiting for acknowledgments of previous ones.
- While this implementation mimics `TCP`’s acknowledgment and retransmission mechanisms, it lacks some of the more sophisticated features of `TCP`, such as **flow control** and **congestion control**, which are essential for ensuring efficient data transmission and preventing network overload. In this simulation, sequence numbers are manually assigned to data chunks for ordering, whereas `TCP` dynamically handles sequence numbers for both reliability and in-order delivery. `TCP` offers additional robustness through features like `windowing`, `error detection`, and `management of network resources`, which are not included in this simplified implementation.
- The project is divided into two main sections: the `server` (receiver) and the `client` (sender). The server creates a `UDP socket`, receives the chunk size and total number of chunks from the client, processes received chunks, stores them in an array based on their sequence numbers, sends an acknowledgment (`ACK`) for each successfully received chunk, and reassembles the complete data after receiving all chunks. The client creates a `UDP socket` in non-blocking mode, accepts an input string, splits it into chunks, sends each chunk to the server while maintaining a record of which chunks have been sent and acknowledged, waits for acknowledgments for a short period (0.05 seconds), and retransmits any unacknowledged chunks if necessary. Once all chunks are successfully acknowledged by the server, the client switches to receiving the final string from the server.

---

## **Overview** 🌍

This project implements a basic `client-server` system that simulates `TCP`-like behavior using `UDP`. It focuses on data transmission reliability by organizing data into smaller chunks, assigning sequence numbers, and ensuring successful delivery through retransmission mechanisms. The client and server communicate via `UDP` sockets, with the client acting as the sender and the server as the receiver.

---

## **Key Features** 🔑

### 1. **Data Sequencing** 🔢
- The client breaks the input data into smaller segments (chunks), each assigned a unique `sequence number`.
- The server receives and reassembles these chunks in order based on their sequence numbers to reconstruct the original input data.
- The client sends the total number of chunks to the server for proper reassembly.

### 2. **Retransmission Handling** 🔄
- After sending a chunk, the client waits for an acknowledgment (`ACK`) from the server.
- If no `ACK` is received within a defined timeout period (0.1 seconds), the client retransmits the same chunk.
- The system employs a `non-blocking` approach to allow for continuous sending of chunks without waiting for acknowledgments of previous ones.

---

## **How it Works** ⚙️

### **Server (Receiver)** 🖥️
- Creates a `UDP socket`.
- Receives the chunk size and total number of chunks from the client.
- Receives each chunk of data, processes its sequence number, and sends an acknowledgment (`ACK`) for the received chunk.
- Stores the chunks in the correct order based on their sequence numbers.
- Once all chunks are received, the server reassembles the data to form the original input string.

### **Client (Sender)** 📱
- Creates a `UDP socket` and switches to non-blocking mode for continuous communication.
- Accepts an input string from the user, splits it into multiple chunks, and assigns each chunk a unique sequence number.
- Sends each chunk to the server while maintaining a record of which chunks have been sent and acknowledged.
- Waits for acknowledgments for a short period (0.05 seconds). If an `ACK` is received, the corresponding chunk is marked as acknowledged and the next chunk is sent.
- If no `ACK` is received within the timeout period, the client checks a list of unacknowledged chunks and retransmits those that have timed out.

---

## **Differences from Actual TCP** 🚫

- While this implementation mimics `TCP`’s acknowledgment and retransmission mechanisms, it lacks some of the more sophisticated features of `TCP`, such as **flow control** and **congestion control**, which are essential for ensuring efficient data transmission and preventing network overload.
- In this simulation, sequence numbers are manually assigned to data chunks for ordering, whereas `TCP` dynamically handles sequence numbers for both reliability and in-order delivery.
- `TCP` offers additional robustness through features like `windowing`, `error detection`, and `management of network resources`, which are not included in this simplified implementation.

---

## **Code Structure** 📂

### **Server-Side (Receiver)** 🖥️
1. Create a `UDP socket`.
2. Receive the chunk size and total number of chunks from the client.
3. Process received chunks, storing them in an array based on their sequence numbers.
4. Send an acknowledgment (`ACK`) for each successfully received chunk.
5. Reassemble the complete data after receiving all chunks.

### **Client-Side (Sender)** 📱
1. Create a `UDP socket` in non-blocking mode.
2. Accept an input string and split it into chunks.
3. For each chunk:
   - Send it to the server.
   - Wait for an `ACK` for 0.05 seconds.
   - If an `ACK` is received, mark the chunk as sent and proceed to the next one.
   - If no `ACK` is received, check for timeouts and retransmit any unacknowledged chunks.
4. Repeat until all chunks are successfully acknowledged by the server.

---

### **Code Sections** 🧑‍💻

#### **1. Library Inclusions and Macros** 📚

```c
#define PORT 8080
#define _BUFFER_SIZE_ 1000000
#define _SEGMENT_SIZE_ 3
#define _TIMEOUT_ 100
```

#### **2. Data Structure Definitions** 🗂️

```c
typedef struct DataSegment {
    int id;
    char* data;
    struct timeval init_time;
} DataSegment;

typedef struct Node {
    DataSegment* segment;
    struct Node* next;
} Node;
```

#### **3. Functions for Data Segment Creation and Handling** ✂️

```c
DataSegment* createDataSegment(int id, char* data);
Node* createNode(int id, char* data);
void insertNode(SegmentList Head, DataSegment* segment);
```

#### **4. Segment Serialization and Resending** 🔄

```c
int generateSteralizedSize(DataSegment* currentSegment);
char* generateSteralizedData(DataSegment* currentSegment);
void resendSegment(int serverSocket, SegmentList Head, struct sockaddr_in serverAddress, int* packet);
```

#### **5. Main Function** 🏁

```c
int main() {
    // Create UDP socket, set up server address, send chunks, wait for ACK, and reassemble the final string.
}
```

---

## **Key Functionalities** 🛠️

1. **Data Segmentation** 🧩: Breaks a string into smaller chunks for transmission.
2. **UDP Transmission** 📡: Uses a `non-blocking UDP socket` to send each segment.
3. **Acknowledgment and Resend** 🔄: Waits for `ACK`, retransmits unacknowledged segments.
4. **Data Reassembly** 🔧: Reconstructs the original string from received chunks.

---

## **Conclusion** 🎉

This project simulates basic `TCP`-like behavior over `UDP` by using data sequencing and retransmission mechanisms. While it captures some of the reliability features of `TCP`, it omits critical components like flow and congestion control, making it a simplified version for educational purposes.

---

### **Usage** 🚀

- **Compile and run the server code** on one machine, then run the client code on another machine or the same machine with the appropriate IP address.
- ```shell
  make
  ```
- ```shell
  ./server
  ```
- ```shell
  ./client
  ```

---

### **References** 📚
- [UDP](https://www.javatpoint.com/udp-protocol)
- [TCP](https://www.javatpoint.com/tcp)
- [Socket Programming](https://www.geeksforgeeks.org/socket-programming-cc/)