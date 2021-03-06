/*
 * Author: Daniel Walsh
 * Date: 5/12/21
 * Title: Lab 5 Server UDP
 * Purpose: Implement the server side for UDP connection
 */

//Lab5 Server
#include "lab5.h"

/*The simplest checksum algorithm is the so-called longitudinal parity check, 
which breaks the data into "bytes" with a fixed number 8 of bits, and then 
computes the exclusive or (XOR) of all those bytes. 
The result is appended to the message as an extra byte*/
//getChecksum()
int getChecksum(Packet packet) {
    packet.header.cksum = 0;
    int checksum = 0;
    char *ptr = (char *)&packet;
    char *end = ptr + sizeof(Header) + packet.header.len;
    while (ptr < end) {
        checksum ^= *ptr++;
    }

    return checksum;
}
// printPacket()
void printPacket(Packet packet) {
    printf("Packet{ header: { seq_ack: %d, len: %d, cksum: %d }, data: \"",
            packet.header.seq_ack,
            packet.header.len,
            packet.header.cksum);
    fwrite(packet.data, (size_t)packet.header.len, 1, stdout);
    printf("\" }\n");
}

//server sending ACK to the client
void serverSend(int sockfd, const struct sockaddr *address, socklen_t addrlen, int seqnum) {
    Packet packet;	
    packet.header.seq_ack=seqnum;
    packet.header.len=sizeof(packet.data);
	packet.header.cksum=getChecksum(packet);
    //send packet
    int bytes_sent = sendto(sockfd, (void*)&packet, sizeof(packet), 0, address, addrlen);
    printf("\t Server sending ACK %d, checksum %d\n", packet.header.seq_ack, packet.header.cksum);
}

//server receiving packet
Packet serverReceive(int sockfd, struct sockaddr *address, socklen_t addrlen, int seqnum) {
    Packet packet;
    while (1) {
        //your code to receive packets from the client
	    int bytes_recv = recvfrom(sockfd, (void*)&packet, sizeof(packet), 0, (struct sockaddr*)address, &addrlen);
        printf("recieved \n");
        
        // print received packet
        printPacket(packet);
	    
        // your code to validate the packet
        int e_cksum = getChecksum(packet);

        //random chance for bad checksum
        srand(time(0));
        int randNum = rand() % 3;
        printf("randNum: %d\n", randNum);
        
        if (packet.header.cksum!=e_cksum || randNum == 0) {
            printf("\t Server: Bad checksum, expected checksum was: %d\n", e_cksum);
            serverSend(sockfd, address, addrlen, !seqnum);
        } else if (packet.header.seq_ack != seqnum) {
            printf("\t Server: Bad seqnum, expected sequence number was:%d\n",seqnum);
            serverSend(sockfd, address, addrlen, !seqnum);
            
        } else {
            printf("\t Server: Good packet\n");
            serverSend(sockfd, address, addrlen, packet.header.seq_ack);

            break;
        }
    }
    return packet;
}


int main(int argc, char *argv[]) {
    // check arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <outfile>\n", argv[0]);
        exit(1);
    }

    // create to create and configure the socket
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
	    perror("failure to setup an endpoint socket\n");	
	    exit(1);
    }

    // your code to initialize the server address structure using argv[1]
   
    struct sockaddr_in address, client_address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(atoi(argv[1]));
    socklen_t addrlen = sizeof(address);
    
    //bind 
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(struct sockaddr)) < 0){
      perror("Failure to bind server address to the endpoint socket");
      exit(1);
    } 

    // your code to open file using argv[2]
    
    int fp;
    char buff[1024];
    if((fp = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0)) < 1){    
	    perror("File failed to open\n");
	    exit(1);
    }

    // your code to get file contents from client and write it to the file
    int seqnum = 0;
    Packet packet;

    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr);
    do {
        packet = serverReceive(sockfd, (struct sockaddr*)&clientaddr, addrlen, seqnum);
	    write(fp,packet.data,strlen(packet.data));
    	seqnum=(seqnum+1)%2;
    } while (packet.header.len != 0);
    

    close(fp);
    close(sockfd);
    return 0;
}
