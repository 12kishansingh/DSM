#include <infiniband/verbs.h>
#include <iostream>
#include <string.h>

void setup_rdma_buffer() {
    // 1. Get the list of RDMA devices (Soft-RoCE or hardware)
    int num_devices;
    struct ibv_device **dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list) {
        std::cerr << "Failed to get RDMA devices list. Is Soft-RoCE enabled?" << std::endl;
        return;
    }

    // 2. Open the first device (e.g., rxe0)
    struct ibv_context *context = ibv_open_device(dev_list[0]);
    
    // 3. Allocate a Protection Domain (PD) - Think of this as a "Sandbox"
    struct ibv_pd *pd = ibv_alloc_pd(context);

    // 4. Create the buffer we want to share over the network
    size_t size = 4096; // One page of memory
    void *buf = malloc(size);
    strcpy((char*)buf, "Hello from Neuro-DSM!");

    // 5. REGISTER MEMORY (The Magic Part)
    // We give the NIC permission to read/write this specific RAM area
    struct ibv_mr *mr = ibv_reg_mr(pd, buf, size, 
                                   IBV_ACCESS_LOCAL_WRITE | 
                                   IBV_ACCESS_REMOTE_READ | 
                                   IBV_ACCESS_REMOTE_WRITE);

    if (!mr) {
        std::cerr << "Memory Registration failed!" << std::endl;
    } else {
        std::cout << "Memory Registered! Remote Key (R_Key): " << mr->rkey << std::endl;
        std::cout << "Virtual Address: " << (uintptr_t)buf << std::endl;
    }

    // Clean up (In a real app, you'd keep these alive)
    ibv_dereg_mr(mr);
    ibv_dealloc_pd(pd);
    ibv_close_device(context);
    ibv_free_device_list(dev_list);
}

int main(){
    setup_rdma_buffer();
    return 0;
}