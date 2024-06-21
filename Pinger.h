#ifndef PINGER_H
#define PINGER_H

#include <netinet/ip_icmp.h>
#include <mutex>
#include <chrono>

// use static constexpr auto instead of defines
#define PING_PKT_S 64
#define PORT_NO 0
#define PING_SLEEP_RATE 1000000
#define RECV_TIMEOUT 1

// ping packet structure
struct ping_pkt{
    struct icmphdr hdr;
    char msg[PING_PKT_S-sizeof(struct icmphdr)];
};

// class RawSocket

class Pinger{
private:
    int sockfd;
    std::string ip_addr;
    sockaddr_in addr_con;
    ping_pkt pckt;
    std::string hostname;
    bool Send_flag = true;
    int msg_count=0,msg_received_count=0;
    std::chrono::time_point<std::chrono::steady_clock> time_start;
    bool Pinger_ready = true;
private:
    std::string dns_lookup(const std::string& addr_host);
    unsigned short checksum(ping_pkt* b, int len);
    void PacketFilling();
    void PacketSend();
    void PacketReceive();
    // Ping();
    static std::mutex mx; // give better naming to understand what mutex covers
public:
    Pinger(const std::string& hostname);
    void Run();
};

#endif
