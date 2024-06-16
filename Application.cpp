#include"Application.h"
#include<iostream>

void App::FillingHosts(){
    std::string hostname;
    host.clear();
    while(true){
        std::cout<<"\nEnter a host or END\n";
        std::cin>>hostname;
        if(hostname=="END"){
            break;
        }
        host.push_back(std::make_shared<Pinger>(hostname));
    }
}

void App::RunPingers(){
    threads.clear();
    for(long unsigned int i = 0;i<host.size();i++){
        threads.emplace_back(&Pinger::Run,host[i]);
    }
    for(long unsigned int i = 0;i<host.size();i++){
        threads[i].join();
    }
}

void App::Run(){
    FillingHosts();
    RunPingers(); 
}

