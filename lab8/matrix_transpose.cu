#include <stdio.h>
#include <stdlib.h>

#define BDIM 16

// 优化版：使用共享内存并填充(Padding)避免存储体冲突
__global__ void transposeOptimized(float *out, float *in, int n) {
    // 给共享内存分配一列空白数据以解决冲突，Stride=17 
    __shared__ float smem[BDIM][BDIM + 1]; 
    
    int bx = blockIdx.x * blockDim.x;
    int by = blockIdx.y * blockDim.y;
    int tx = threadIdx.x;
    int ty = threadIdx.y;
    
    // 读入第(i, j)个数据块至共享内存
    if (bx + tx < n && by + ty < n) {
        smem[ty][tx] = in[(by + ty) * n + bx + tx];
    }
    __syncthreads();
    
    // 计算转置后的全局内存坐标：元素(i, j) -> (j, i) 
    int bx_out = blockIdx.y * blockDim.y;
    int by_out = blockIdx.x * blockDim.x;
    
    if (bx_out + tx < n && by_out + ty < n) {
        // 从共享内存中写出至第(j, i)个数据块 
        out[(by_out + ty) * n + bx_out + tx] = smem[tx][ty]; 
    }
}

int main() {
    int n = 1024; // 矩阵规模，取值范围为[512, 2048] 
    size_t size = n * n * sizeof(float);
    
    float *h_A = (float*)malloc(size);
    float *h_AT = (float*)malloc(size);
    for(int i = 0; i < n * n; i++) h_A[i] = (float)rand() / RAND_MAX;
    
    float *d_A, *d_AT;
    cudaMalloc(&d_A, size);
    cudaMalloc(&d_AT, size);
    cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
    
    dim3 block(BDIM, BDIM); // 线程块宽度为16倍数 
    dim3 grid((n + block.x - 1) / block.x, (n + block.y - 1) / block.y);
    
    cudaEvent_t start, stop;
    cudaEventCreate(&start); cudaEventCreate(&stop);
    
    cudaEventRecord(start);
    transposeOptimized<<<grid, block>>>(d_AT, d_A, n);
    cudaEventRecord(stop);
    
    cudaMemcpy(h_AT, d_AT, size, cudaDeviceToHost);
    cudaEventSynchronize(stop);
    
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    
    printf("Matrix Size: %dx%d\n", n, n);
    printf("Time consumed (ms): %f\n", milliseconds); // 计算所消耗的时间 
    
    cudaFree(d_A); cudaFree(d_AT);
    free(h_A); free(h_AT);
    return 0;
}