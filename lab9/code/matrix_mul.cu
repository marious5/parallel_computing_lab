#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <sys/time.h>

// 检查CUDA运行时的错误
#define CHECK(call)                                                            \
{                                                                              \
    const cudaError_t error = call;                                            \
    if (error != cudaSuccess)                                                  \
    {                                                                          \
        fprintf(stderr, "Error: %s:%d, ", __FILE__, __LINE__);                 \
        fprintf(stderr, "code: %d, reason: %s\n", error,                       \
                cudaGetErrorString(error));                                    \
        exit(1);                                                               \
    }                                                                          \
}

// ---------------------------------------------------------
// Kernel 1: 基础版本 (Global Memory)
// ---------------------------------------------------------
__global__ void matrixMulNaive(float *A, float *B, float *C, int m, int n, int k) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < m && col < k) {
        float sum = 0.0f;
        for (int i = 0; i < n; ++i) {
            sum += A[row * n + i] * B[i * k + col];
        }
        C[row * k + col] = sum;
    }
}

// ---------------------------------------------------------
// Kernel 2: 共享内存分块版本 (Shared Memory Tiling)
// ---------------------------------------------------------
__global__ void matrixMulShared(float *A, float *B, float *C, int m, int n, int k, int TILE_SIZE) {
    // 动态分配共享内存
    extern __shared__ float sharedMem[];
    float* s_A = sharedMem;
    float* s_B = (float*)&sharedMem[TILE_SIZE * TILE_SIZE];

    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    int tx = threadIdx.x;
    int ty = threadIdx.y;

    float sum = 0.0f;
    int numTiles = (n + TILE_SIZE - 1) / TILE_SIZE;

    for (int t = 0; t < numTiles; ++t) {
        // 协同加载A到共享内存
        if (row < m && t * TILE_SIZE + tx < n)
            s_A[ty * TILE_SIZE + tx] = A[row * n + t * TILE_SIZE + tx];
        else
            s_A[ty * TILE_SIZE + tx] = 0.0f;

        // 协同加载B到共享内存
        if (t * TILE_SIZE + ty < n && col < k)
            s_B[ty * TILE_SIZE + tx] = B[(t * TILE_SIZE + ty) * k + col];
        else
            s_B[ty * TILE_SIZE + tx] = 0.0f;

        __syncthreads();

        // 计算当前块的乘积
        for (int i = 0; i < TILE_SIZE; ++i) {
            sum += s_A[ty * TILE_SIZE + i] * s_B[i * TILE_SIZE + tx];
        }
        __syncthreads();
    }

    if (row < m && col < k) {
        C[row * k + col] = sum;
    }
}

double cpuSecond() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double)tp.tv_sec + (double)tp.tv_usec * 1.e-6);
}

int main(int argc, char **argv) {
    if (argc != 6) {
        printf("Usage: ./matrix_mul <m> <n> <k> <block_size> <kernel_type(0:Naive, 1:Shared)>\n");
        return -1;
    }

    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);
    int blockSize = atoi(argv[4]);
    int kernelType = atoi(argv[5]);

    size_t sizeA = m * n * sizeof(float);
    size_t sizeB = n * k * sizeof(float);
    size_t sizeC = m * k * sizeof(float);

    // 分配并初始化Host内存
    float *h_A = (float *)malloc(sizeA);
    float *h_B = (float *)malloc(sizeB);
    float *h_C = (float *)malloc(sizeC);

    for (int i = 0; i < m * n; i++) h_A[i] = (float)(rand() % 100) / 10.0f;
    for (int i = 0; i < n * k; i++) h_B[i] = (float)(rand() % 100) / 10.0f;

    // 分配Device内存
    float *d_A, *d_B, *d_C;
    CHECK(cudaMalloc((void **)&d_A, sizeA));
    CHECK(cudaMalloc((void **)&d_B, sizeB));
    CHECK(cudaMalloc((void **)&d_C, sizeC));

    // 数据传输 H2D
    CHECK(cudaMemcpy(d_A, h_A, sizeA, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(d_B, h_B, sizeB, cudaMemcpyHostToDevice));

    // 配置Grid和Block
    dim3 block(blockSize, blockSize);
    dim3 grid((k + block.x - 1) / block.x, (m + block.y - 1) / block.y);

    // 预热与计时
    double start, time_t;
    if (kernelType == 0) {
        matrixMulNaive<<<grid, block>>>(d_A, d_B, d_C, m, n, k);
        CHECK(cudaDeviceSynchronize()); // 预热
        
        start = cpuSecond();
        matrixMulNaive<<<grid, block>>>(d_A, d_B, d_C, m, n, k);
        CHECK(cudaDeviceSynchronize());
        time_t = cpuSecond() - start;
    } else {
        size_t sharedMemSize = 2 * blockSize * blockSize * sizeof(float);
        matrixMulShared<<<grid, block, sharedMemSize>>>(d_A, d_B, d_C, m, n, k, blockSize);
        CHECK(cudaDeviceSynchronize()); // 预热
        
        start = cpuSecond();
        matrixMulShared<<<grid, block, sharedMemSize>>>(d_A, d_B, d_C, m, n, k, blockSize);
        CHECK(cudaDeviceSynchronize());
        time_t = cpuSecond() - start;
    }

    CHECK(cudaMemcpy(h_C, d_C, sizeC, cudaMemcpyDeviceToHost));

    printf("Kernel: %s, M: %d, N: %d, K: %d, BlockSize: %dx%d, Time(t): %f ms\n", 
            kernelType == 0 ? "Naive" : "Shared", m, n, k, blockSize, blockSize, time_t * 1000);

    free(h_A); free(h_B); free(h_C);
    cudaFree(d_A); cudaFree(d_B); cudaFree(d_C);

    return 0;
}