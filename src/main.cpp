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

#include <vector>
#include <fstream>
#include <algorithm>

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
    int ttlCount=0;
    int destPort = 33345;
    std::string message = "Hello from UDP";
    std::vector<std::pair<int ,std::string>> connections;
    std::vector<std::string> receivedIps;

    if (argc != 2) {
        std::cerr << "Usage: traceroute <hostname>" << std::endl;
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
    dest.sin_port = destPort++;
    memcpy(&dest.sin_addr, host->h_addr, host->h_length);

    std::string destIp(INET_ADDRSTRLEN, 0x00);
    inet_ntop(AF_INET, &dest.sin_addr, (char *)destIp.data(), INET_ADDRSTRLEN);
    std::cout << "dest: " << destIp << '\n';


    struct timeval timeout;
    timeout.tv_sec = 0; // 2 second timeout
    timeout.tv_usec = 10;
    if (setsockopt(sockIcmp, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
        std::cerr << "setsockopt error" << std::endl;
        return -1;
    }
    

    char recv_buf[1024];
    
    // Loop through each hop
    
    while (ttl < 70)
    {
        // Create a raw socket
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
            std::cerr << "setsockopt error" << std::endl;
            return -1;
        }
        if (sock < 0) {
            std::cerr << "Error: Cannot create socket" << std::endl;
            return 1;
        }

        dest.sin_port = destPort++;
        bzero(recv_buf, sizeof(recv_buf));
        // Increment the TTL
        ttl++;
        

        if(setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        {
            perror("Error: setsockopt");
            return 1;
        }

        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);
        if (sendto(sock, message.c_str(), message.size(), 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
            std::cerr << "sendto error " << strerror(errno) << std::endl;
            return -1;
        }

        printf("now receiving from the ttl: &d\n", ttl);
        int recv_len = recv(sockIcmp, recv_buf, sizeof(recv_buf), 0);
        if (recv_len < 0) {
            printf("ttl: %d return val: %d\n", ttl, recv_len);
            printf("%s\n",strerror(errno));
            //std::cerr << "recvfrom error" << std::endl;
            if(ttlCount < 8){
                ttl--;
                ttlCount++;
                close(sock);
                continue;
            }
            else{
                ttlCount = 0;
                close(sock);
                continue;
            }
            
            
        }
        gettimeofday(&end_time, NULL);  // measure end time
        // Calculate the RTT in milliseconds
        double rtt = (end_time.tv_sec - start_time.tv_sec) * 1000.0; // seconds to milliseconds
        rtt += (end_time.tv_usec - start_time.tv_usec) / 1000.0; // microseconds to milliseconds

        struct iphdr *ip_header = (struct iphdr *) recv_buf;
        struct icmphdr *icmp_header = (struct icmphdr *) (recv_buf + (ip_header->ihl * 4));

        // convert source IP address to string
        std::string srcIp(INET_ADDRSTRLEN, 0x00);
        inet_ntop(AF_INET, &ip_header->saddr, (char *)srcIp.data(), INET_ADDRSTRLEN);

        //look for the ip address in the receivedIps
        auto it = std::find(receivedIps.begin(), receivedIps.end(), srcIp);
        if(it == receivedIps.end()){
            //this ip not in receivedIps
            //add it to the receivedIPs
            receivedIps.push_back(srcIp);
            std::pair<int, std::string>connection = make_pair(ttl, srcIp);
            connections.push_back(connection);
            std::cout << ttl << " " << srcIp << " "  << rtt << " ms" << '\n';
        
        }
        else{
            //this ip in the receivedIps sp do nothing
        }
        
        close(sock);

        if(srcIp == destIp)
            break;
            
    
}


close(sockIcmp);

// Open a file for writing
  ofstream outFile("mygraph.gml");

  // Write the GML syntax to the file
  outFile << "graph [\n";
  outFile << "  directed 1\n";

  // Write the nodes to the file
  for (int i = 0; i < connections.size(); i++) {
    outFile << "  node [\n";
    outFile << "    id " << connections[i].first << "\n";
    outFile << "    label " << connections[i].second << "\n";
    outFile << "  ]\n";
  }

  // Write the edges to the file
  for (int i = 0; i < connections.size()-1; i++) {
      outFile << "  edge [\n";
      outFile << "    source " << connections[i].first << "\n";
      outFile << "    target " << connections[i + 1].first << "\n";
      outFile << "    label " << connections[i].second << "\n";
      outFile << "  ]\n";
    }

  outFile << "]\n";



  // Close the file
  outFile.close();

return 0;
}


