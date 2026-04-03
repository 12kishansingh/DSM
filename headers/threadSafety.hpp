#ifndef THREAD_SAFETY_H
#define THREAD_SAFETY_H

#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>

struct SharedData
{
    std::mutex mtx; // The lock
    std::condition_variable cv; // The signaling mechanism (The "Doorbell")
    int flag = 0; // Your status flag
};

extern SharedData mesh_info;

struct commands_args
{
    SharedData *mesh_info;
};

void wait();

void resume();


#endif // THREAD_SAFETY_H