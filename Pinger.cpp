#include "Pinger.h"
#include <atomic>
#include <iostream>
#include <thread>
#include <string.h>
#include <string>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>


static std::atomic<bool> exit_thread_flag{false};
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
    }else{
    	std::cout<<"\nTrying to connect to \n";
    	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    	if(sockfd<0){
            std::cout<<"\nSocket file descriptor not received!!\n";
    	}else{
            std::cout<<"\nSocket file descriptor"<<sockfd<<"received\n";
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

void Pinger::PacketFilling(){
    bzero(&pckt, sizeof(pckt));
    pckt.hdr.type = ICMP_ECHO;
    pckt.hdr.un.echo.id = getpid();
    for (long unsigned int i = 0; i < sizeof(pckt.msg)-1; i++)
        pckt.msg[i] = i+'0';
    pckt.msg[sizeof(pckt.msg) - 1] = 0;
    pckt.hdr.un.echo.sequence = msg_count++;
    pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
}

void Pinger::PacketSend(){
    time_start = std::chrono::steady_clock::now();
    if (sendto(sockfd, &pckt, sizeof(pckt), 0,(struct sockaddr*)(&addr_con),sizeof(addr_con)) <= 0){
    	mx.lock();
        std::cout<<"\nPacket Sending Failed!\n";
        mx.unlock();
        Send_flag=false;
    }
}

void Pinger::PacketReceive(){
    sockaddr_in r_addr;
    socklen_t addr_len;
    addr_len=sizeof(r_addr);
    if (recvfrom(sockfd, &pckt, sizeof(pckt), 0,(struct sockaddr*)&r_addr, &addr_len) <= 0 && msg_count>1){
    	mx.lock();
        std::cout<<"\nPacket receive failed!\n";
        mx.unlock();
    }else{
        std::chrono::time_point<std::chrono::steady_clock> time_end = std::chrono::steady_clock::now();
        double timeElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_start).count();
        if(Send_flag){
            if(!(pckt.hdr.type ==69 && pckt.hdr.code==0)){
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
    if(Pinger_ready){
    	signal(SIGINT,[](int signal){exit_thread_flag = true;});
    	while(!exit_thread_flag){
            PacketFilling();
            usleep(PING_SLEEP_RATE);
            PacketSend();
            PacketReceive();
    	}
        std::this_thread::sleep_for(std::chrono::seconds(1));

    	mx.lock();
    	std::cout<<"\n"<<hostname<<" sent packets: "<<msg_count<<" lost packets: "<<msg_count - msg_received_count<<std::endl;
    	mx.unlock();
    }
}
