#include "../headers/threadSafety.hpp"


SharedData mesh_info;

void wait()
{
    std::unique_lock<std::mutex> lock(mesh_info.mtx);

    mesh_info.cv.wait(lock, [] {
        return mesh_info.flag == 1;
    });

    mesh_info.flag = 0; // reset
}

void resume()
{
    {
        std::lock_guard<std::mutex> lock(mesh_info.mtx);
        mesh_info.flag = 1;
    }
    mesh_info.cv.notify_one();
}