#ifndef APPLICATION_H
#define APPLICATION_H

#include"Pinger.h"
#include<vector>
#include<memory>
#include<thread>

class App{
private:
    // use unique_ptr?
    std::vector<std::shared_ptr<Pinger>> host; // hosts
    std::vector<std::thread> threads;
private:
    void FillingHosts();
    void RunPingers();
public:
    void Run();
};

#endif