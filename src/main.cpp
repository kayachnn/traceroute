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

using namespace std;

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
    char buf[BUF_SIZE]; 

    if (argc != 2) {
        std::cerr << "Usage: traceroute <hostname>" << std::endl;
        return 1;
    }

    // Create a raw socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        std::cerr << "Error: Cannot create socket" << std::endl;
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
    dest.sin_port = 0;
    // dest_addr.sin_family = AF_INET;
    // dest_addr.sin_port = 0;
    // memcpy(&dest_addr.sin_addr, host->h_addr, host->h_length);
    // socklen_t dest_add_len = sizeof(dest_addr);

    // set socket timeout option
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = TIMEOUT;
    int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (ret == -1) {
        return -1;
    }

    // Loop through each hop
    
    while (ttl < 64)
    {
        // Increment the TTL
        ttl++;

        struct icmp_echo icmp;
        bzero(&icmp, sizeof(icmp));

        // fill header files
        icmp.type = 8;
        icmp.code = 0;
        icmp.ident = htons(ttl);
        icmp.seq = htons(ttl);


        // Set the TTL for the packet
        if(setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        {
        perror("Error: setsockopt");
        return 1;
        }
        std:cout << "ttl: " << ttl << '\n';


        // Send the ICMP echo request
        int bytes_sent = sendto(sock, &icmp, sizeof(icmp), 0, (struct sockaddr *)&dest, sizeof(dest));
        if (bytes_sent < 0) {
            std::cerr << "Error: Cannot send packet" << std::endl;
            return 1;
        }

        // allocate buffer
        char buffer[BUF_SIZE];
        struct sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        bzero(&buffer, sizeof(buffer));

        int bytes_received = recvfrom(sock, buffer, sizeof(buffer), 0,
        (struct sockaddr*)&peer_addr, &addr_len);


        if (bytes_received < 0) {
            std::cerr << "Error: Cannot receive response" << std::endl;
            continue;;
        }
        // std::cout << "check if timeout\n";
        // if(bytes_received == 0){
        //     std::cout << "timeout\n";
        //     continue;
        // }

        // find icmp packet in ip packet
        struct icmphdr* icmpRecv = (struct icmphdr*)(buffer + 20);

        

        printf("dest: %s\n",inet_ntoa(dest.sin_addr));
    
    
}

close(sock);

return 0;
}


