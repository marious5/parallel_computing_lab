#include <stdio.h>

__global__ void helloWorld() {
    // 每个线程输出线程块编号、二维块内线程编号 
    printf("Hello World from Thread (%d, %d) in Block %d!\n", threadIdx.x, threadIdx.y, blockIdx.x);
}

int main() {
    int n = 4; // 线程块数量，取值范围建议在[1, 32] 
    int m = 2, k = 2; // 线程块维度 m x k
    
    dim3 grid(n);
    dim3 block(m, k);
    
    // 主线程输出 
    printf("Hello World from the host!\n");
    
    // 启动内核 
    helloWorld<<<grid, block>>>();
    
    // 等待设备同步，确保printf能输出
    cudaDeviceSynchronize(); 
    return 0;
}