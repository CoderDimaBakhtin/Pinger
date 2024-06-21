#include"Application.h"
#include<iostream>

void App::FillingHosts(){
    std::string hostname;
    host.clear();
    while(true){
        // Improve user experience
        std::cout<<"\nEnter a host or END\n";
        std::cin>>hostname;
        if(hostname=="END"){
            break;
        }
        // If hostname was wrong we will anyway have Pinger instance and created thread
        // Pinger p;
        // if (p.initalize(hostname)) { // if succeded to initialize add to container
            host.push_back(std::make_shared<Pinger>(hostname));
        // }

    }
}

void App::RunPingers(){
    threads.clear();
    // too long type name
    // long unsigned int ~= size_t
    // You can use `for each`
    // for (const auto& host : hosts)
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

