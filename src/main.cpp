#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>
#include <errno.h>

using namespace std;

#define IP_HEADER_LEN sizeof(iphdr)
#define ICMP_HEADER_LEN sizeof(icmphdr)
#define IP_PACKET_LEN IP_HEADER_LEN + ICMP_HEADER_LEN


const int BUF_SIZE = 1024;
const int TIMEOUT = 2;

// Calculates the checksum of an ICMP header
unsigned short calc_checksum(unsigned short *buf, int nwords)
{
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}



struct icmp_echo {
    // header
    uint8_t type;
    uint8_t code;
    uint16_t checksum;

    uint16_t ident;
    uint16_t seq;

};


int main(int argc, char *argv[])
{
    int ttl = 0;
    std::string message = "Hello from UDP";

    if (argc != 2) {
        std::cerr << "Usage: traceroute <hostname>" << std::endl;
        return 1;
    }

    // Create a raw socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Error: Cannot create socket" << std::endl;
        return 1;
    }
    int sockIcmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sockIcmp < 0){
        std::cerr << "Error: Cannot create raw socket\n";
        return 1;
    }

    // hostname
    struct hostent *host = gethostbyname(argv[1]);
    if (!host) {
        std::cerr << "Error: Cannot resolve hostname" << std::endl;
        return 1;
    }

    // destination
    struct sockaddr_in dest;
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = 12345;
    memcpy(&dest.sin_addr, host->h_addr, host->h_length);


    struct timeval timeout;
    timeout.tv_sec = 1; // 2 second timeout
    timeout.tv_usec = 0;
    if (setsockopt(sockIcmp, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
        std::cerr << "setsockopt error" << std::endl;
        return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
        std::cerr << "setsockopt error" << std::endl;
        return -1;
    }

    char recv_buf[1024];
    
    // Loop through each hop
    
    while (ttl < 64)
    {
        bzero(recv_buf, sizeof(recv_buf));
        // Increment the TTL
        ttl++;

        if(setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        {
            perror("Error: setsockopt");
        return 1;
        }


        if (sendto(sock, message.c_str(), message.size(), 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
            std::cerr << "sendto error " << strerror(errno) << std::endl;
            return -1;
        }



        bzero(recv_buf, sizeof(recv_buf));
        int recv_len = recv(sockIcmp, recv_buf, sizeof(recv_buf), 0);
        if (recv_len < 0) {
            std::cerr << "recvfrom error" << std::endl;
            continue;
        }
        std::cout << "received package\n";
        

        struct iphdr *ip_header = (struct iphdr *) recv_buf;
        struct icmphdr *icmp_header = (struct icmphdr *) (recv_buf + (ip_header->ihl * 4));

        // convert source IP address to string
        char src_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip_header->saddr, src_ip, INET_ADDRSTRLEN);
        std::cout << "Received ICMP packet from IP address: " << src_ip << std::endl;
       
            
    
}

close(sock);

return 0;
}


