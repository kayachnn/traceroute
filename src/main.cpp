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


int main(int argc, char *argv[])
{
    int sock_raw;
    int ttl = 1;
    int addr_len = sizeof(sockaddr_in);
    int seq = 1;
    int count = 1;
    struct sockaddr_in addr_dest;
    struct sockaddr_in addr_from;
    char buf[BUF_SIZE];
    char ip_str[BUF_SIZE];
    struct iphdr *ip;
    struct icmphdr *icmp;
    int IP_HEADER_LEN = sizeof(ip);
    int ICMP_HEADER_LEN = sizeof(icmp);
    int IP_PACKET_LEN = IP_HEADER_LEN + ICMP_HEADER_LEN;

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
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = 0;
    memcpy(&dest_addr.sin_addr, host->h_addr, host->h_length);
    socklen_t dest_add_len = sizeof(dest_addr);


    int ttl = 1;
    
    

    


    // Loop through each hop
    
    while (ttl < 64)
    {
        // Set the TTL for the packet
        if(setsockopt(sock_raw, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        {
        perror("Error: setsockopt");
        return 1;
        }

        memset(buf, 0, BUF_SIZE);

        ip.version = 4;
        ip->ihl = 5;
        iph->tos = 0;
        iph->tot_len = sizeof(struct ip) + sizeof(struct icmphdr);
        iph->id = htons(getpid());
        iph->frag_off = 0;
        iph->ttl = ttl;
        iph->protocol = IPPROTO_ICMP;
        iph->check = 0;
        iph->saddr = inet_addr("0.0.0.0");
        iph->daddr = inet_addr(argv[1]);

        icmph->type = ICMP_ECHO;
        icmph->code = 0;
        icmph->un.echo.id = htons(getpid());

        
        // Send the ICMP echo request
        int bytes_sent = sendto(sock, buffer, sizeof(icmp), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (bytes_sent < 0) {
            std::cerr << "Error: Cannot send packet" << std::endl;
            return 1;
        }

        // Wait for a response with timeout
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);


        int result = select(sock + 1, &read_fds, nullptr, nullptr, &tv);
        if (result == -1) {
            std::cerr << "Error: Cannot receive response" << std::endl;
            return 1;
        } else if (result == 0) {
            std::cout << "Timeout exceeded" << std::endl;
            continue;;
        }

        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock, buffer, BUF_SIZE, 0);
        if (bytes_received < 0) {
            std::cerr << "Error: Cannot receive response" << std::endl;
            return 1;
        }


        

    // Increment the TTL
    ttl++;
}

close(sock);

return 0;
}


