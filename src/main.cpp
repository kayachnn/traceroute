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
    memcpy(&dest.sin_addr, host->h_addr, host->h_length);
    socklen_t dest_add_len = sizeof(dest);


    struct timeval timeout;
    timeout.tv_sec = 2; // 2 second timeout
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
    std::cerr << "setsockopt error" << std::endl;
    return -1;
    }
    // Loop through each hop
    
    while (ttl < 64)
    {
        // Increment the TTL
        ttl++;

        if(setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        {
            perror("Error: setsockopt");
        return 1;
        }


         // build the ICMP header
        // build the ICMP header
        char send_buf[IP_PACKET_LEN];
        icmphdr *icmp = (icmphdr *)(send_buf);
        icmp->type = ICMP_ECHO;
        icmp->code = 0;
        icmp->un.echo.id = getpid();
        icmp->un.echo.sequence = ttl;
        icmp->checksum = calc_checksum((unsigned short *) icmp, ICMP_HEADER_LEN);



        // Send the ICMP echo request
        int bytes_sent = sendto(sock, send_buf, IP_PACKET_LEN, 0, (struct sockaddr *)&dest, sizeof(dest));
        if (bytes_sent < 0) {
            std::cerr << "Error: Cannot send packet" << std::endl;
            return 1;
        }

        // wait for the reply
        char recv_buf[IP_PACKET_LEN];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        int n = recvfrom(sock, recv_buf, IP_PACKET_LEN, 0, (struct sockaddr *) &from_addr, &from_len);
        if (n < 0) {
        std::cerr << "recvfrom error" << std::endl;
            continue;;
        }

        // parse the reply
        iphdr *recv_ip = (iphdr *) recv_buf;
        icmphdr *recv_icmp = (icmphdr *) (recv_buf + IP_HEADER_LEN);
        if (recv_icmp->type == ICMP_ECHOREPLY) {
            std::cout << "Received reply from " << inet_ntoa(*(struct in_addr *) &from_addr.sin_addr) << ", ttl = " << ttl << std::endl;
        break;
        }
    
    
}

close(sock);

return 0;
}


