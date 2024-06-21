#include "Pinger.h"
#include <atomic>
#include <iostream>
#include <thread>
#include <string.h>
#include <string>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

static std::atomic<bool> exit_thread_flag{false}; // should be member of Pinger object or Application
std::mutex Pinger::mx;

std::string Pinger::dns_lookup(const std::string& addr_host){
    hostent *host_entity;
    if ((host_entity = gethostbyname(addr_host.c_str())) == NULL){
        return "";
    }
    std::string ip(inet_ntoa(*(in_addr*)host_entity->h_addr));
    addr_con.sin_family = host_entity->h_addrtype;
    addr_con.sin_port = htons(PORT_NO);
    addr_con.sin_addr.s_addr = *(long*)host_entity->h_addr;
    return ip;
}

// mostly it requires void* and size_t
// if you want function specification for ping_pkt
// you can skip length field and use sizeof(ping_pkt) inside
// event better would be 2 functions
// unsigned short Pinger::checksum(ping_pkt& packet) {return checksum(&ping_pkt, sizeof(ping_pkt));}
// unsigned short Pinger::checksum(void* data, int len)
unsigned short Pinger::checksum(ping_pkt* packet, int len){
    unsigned short* buf = reinterpret_cast<unsigned short*>(packet);
    unsigned int sum=0;
    unsigned short result;
    for (sum = 0;len > 1;len -= 2){
        sum += *buf++;
    }
    if (len == 1){
        sum += *buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

Pinger::Pinger(const std::string& hostname){
    this->hostname = hostname;
    ip_addr = dns_lookup(hostname);
    if(ip_addr.empty()){
        std::cout<<"\nDNS lookup failed! Could not resolve hostname!\n";
        Pinger_ready = false;
        // return;
        // return and drop next else statement
    }else{
    	std::cout<<"\nTrying to connect to \n"; // to what?
    	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    	if(sockfd<0){
            std::cout<<"\nSocket file descriptor not received!!\n";
            // return; Pinger_ready = false
    	}else{
            // For debugging Ok, for production nope
            std::cout<<"\nSocket file descriptor"<<sockfd<<"received\n";// this log is exces
    	}
    	int time_to_live_val = 3;
    	timeval tv_out;
    	tv_out.tv_sec = RECV_TIMEOUT;
    	tv_out.tv_usec = 0;
    	if (setsockopt(sockfd, SOL_IP, IP_TTL,&time_to_live_val, sizeof(time_to_live_val)) != 0){
       	    std::cout<<"\nSetting socket options to TTL failed!\n";
       	    Pinger_ready = false;
    	}else{
            std::cout<<"\nSocket set to TTL..\n";
    	}
    	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(const char*)&tv_out, sizeof(tv_out));
    }
}

// this function can be implemented inside ping_pkt
void Pinger::PacketFilling(){
    // pckt is a c++ struct you can define its own ctor or default value for all members
    bzero(&pckt, sizeof(pckt)); // copy pasted? use memset instead
    pckt.hdr.type = ICMP_ECHO;
    pckt.hdr.un.echo.id = getpid();
    for (long unsigned int i = 0; i < sizeof(pckt.msg)-1; i++)
        pckt.msg[i] = i+'0';
    pckt.msg[sizeof(pckt.msg) - 1] = 0;
    pckt.hdr.un.echo.sequence = msg_count++;
    pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
}


// packetSend and packed receive should be wrapped in some function
// time_start
// PakcetSend
// PacketReceive
// time_end
void Pinger::PacketSend(){ // should return bool
    time_start = std::chrono::steady_clock::now();
    if (sendto(sockfd, &pckt, sizeof(pckt), 0,(struct sockaddr*)(&addr_con),sizeof(addr_con)) <= 0){
    	mx.lock(); // lock_guard or scoped_lock
        {
            // lock_guard
            std::cout<<"\nPacket Sending Failed!\n";
        }
        mx.unlock();
        Send_flag=false;
    }
}

void Pinger::PacketReceive(){
    sockaddr_in r_addr;
    socklen_t addr_len = sizeof(r_addr);
    // we will try to received reply but we may fail send
    if (recvfrom(sockfd, &pckt, sizeof(pckt), 0,(struct sockaddr*)&r_addr, &addr_len) <= 0 && msg_count>1){
    	mx.lock();
        std::cout<<"\nPacket receive failed!\n";
        mx.unlock();
        // return
    }else{
        std::chrono::time_point<std::chrono::steady_clock> time_end = std::chrono::steady_clock::now();
        double timeElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_start).count();
        if(Send_flag){
            if(!(pckt.hdr.type ==69 && pckt.hdr.code==0)){ // Magic numbers
            	mx.lock();
            	std::cout << "Error..Packet received with ICMP type " << pckt.hdr.type << " code " << pckt.hdr.code << std::endl;
            	mx.unlock();
            }else{
                mx.lock();
                std::cout << PING_PKT_S << " bytes from "<< hostname << " (" << ip_addr << ") msg_seq=";
                std::cout<< msg_count << " timeElapsed = " << timeElapsed << " ns."<< std::endl;
                mx.unlock();
                msg_received_count++;
            }
        }
    }
}

void Pinger::Run(){
    // if (!Pinger_ready) return;
    if(Pinger_ready){
    	signal(SIGINT,[](int signal){exit_thread_flag = true;}); // should be done only once
    	while(!exit_thread_flag){
            PacketFilling();
            // why sleep is needed?
            usleep(PING_SLEEP_RATE); // use std::this_thread::sleep_for()
            PacketSend();
            PacketReceive();
    	}
        std::this_thread::sleep_for(std::chrono::seconds(1));

    	mx.lock();
        // Should be logic of Application class
    	std::cout<<"\n"<<hostname<<" sent packets: "<<msg_count<<" lost packets: "<<msg_count - msg_received_count<<std::endl;
    	mx.unlock();
    }
}
