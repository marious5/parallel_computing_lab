#include <iostream>
#include <vector>
#include <cstdlib>
#include <cuda_runtime.h>
#include <cudnn.h>
#include <cublas_v2.h>

// 宏定义：CUDA 和 cuDNN 错误检查
#define CHECK_CUDA(call) { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        std::cerr << "CUDA Error: " << cudaGetErrorString(err) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(EXIT_FAILURE); \
    } \
}

#define CHECK_CUDNN(call) { \
    cudnnStatus_t status = call; \
    if (status != CUDNN_STATUS_SUCCESS) { \
        std::cerr << "cuDNN Error: " << cudnnGetErrorString(status) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(EXIT_FAILURE); \
    } \
}

#define CHECK_CUBLAS(call) { \
    cublasStatus_t status = call; \
    if (status != CUBLAS_STATUS_SUCCESS) { \
        std::cerr << "cuBLAS Error at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(EXIT_FAILURE); \
    } \
}

// ---------------------------------------------------------
// 1. 滑窗法 (Direct Convolution) Kernel
// ---------------------------------------------------------
__global__ void conv2d_sliding_window_kernel(
    const float* __restrict__ input, const float* __restrict__ weight, float* __restrict__ output,
    int in_c, int out_c, int in_h, int in_w, 
    int k_size, int stride, int pad, int out_h, int out_w) 
{
    int w_out = blockIdx.x * blockDim.x + threadIdx.x;
    int h_out = blockIdx.y * blockDim.y + threadIdx.y;
    int c_out = blockIdx.z * blockDim.z + threadIdx.z;

    if (h_out < out_h && w_out < out_w && c_out < out_c) {
        float val = 0.0f;
        for (int c_in = 0; c_in < in_c; ++c_in) {
            for (int kr = 0; kr < k_size; ++kr) {
                for (int kc = 0; kc < k_size; ++kc) {
                    int h_in = h_out * stride - pad + kr;
                    int w_in = w_out * stride - pad + kc;
                    if (h_in >= 0 && h_in < in_h && w_in >= 0 && w_in < in_w) {
                        int in_idx = (c_in * in_h + h_in) * in_w + w_in;
                        int w_idx = ((c_out * in_c + c_in) * k_size + kr) * k_size + kc;
                        val += input[in_idx] * weight[w_idx];
                    }
                }
            }
        }
        int out_idx = (c_out * out_h + h_out) * out_w + w_out;
        output[out_idx] = val;
    }
}

// ---------------------------------------------------------
// 2. im2col Kernel
// ---------------------------------------------------------
__global__ void im2col_kernel(
    const float* data_im, float* data_col,
    int in_c, int in_h, int in_w,
    int k_size, int stride, int pad, int out_h, int out_w) 
{
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int total_elements = in_c * out_h * out_w;

    if (index < total_elements) {
        int w_out = index % out_w;
        int h_out = (index / out_w) % out_h;
        int c_in = index / (out_w * out_h);

        int c_out = c_in * k_size * k_size;
        int h_in = h_out * stride - pad;
        int w_in = w_out * stride - pad;

        data_col += (c_out * out_h + h_out) * out_w + w_out;
        data_im += (c_in * in_h + h_in) * in_w + w_in;

        for (int i = 0; i < k_size; ++i) {
            for (int j = 0; j < k_size; ++j) {
                int h = h_in + i;
                int w = w_in + j;
                *data_col = (h >= 0 && w >= 0 && h < in_h && w < in_w) ? data_im[i * in_w + j] : 0.0f;
                data_col += out_h * out_w;
            }
        }
    }
}

// ---------------------------------------------------------
// 主函数
// ---------------------------------------------------------
int main() {
    // 实验参数设置 
    int N = 1;          // Batch size
    int in_c = 3;       // Input channels 
    int out_c = 3;      // Output channels (Filters)
    int in_h = 32;      // Image height
    int in_w = 32;      // Image width
    int k_size = 3;     // Kernel size (3x3)
    int stride = 1;     // Stride 1, 2, or 3
    int pad = 1;        // Padding 

    // 计算输出尺寸
    int out_h = (in_h - k_size + 2 * pad) / stride + 1; 
    int out_w = (in_w - k_size + 2 * pad) / stride + 1; 

    size_t size_input = N * in_c * in_h * in_w * sizeof(float);
    size_t size_weight = out_c * in_c * k_size * k_size * sizeof(float);
    size_t size_output = N * out_c * out_h * out_w * sizeof(float);

    // Host 内存分配与初始化
    std::vector<float> h_input(size_input / sizeof(float), 1.0f);
    std::vector<float> h_weight(size_weight / sizeof(float), 0.5f);
    std::vector<float> h_output(size_output / sizeof(float), 0.0f);

    // Device 内存分配
    float *d_input, *d_weight, *d_output;
    CHECK_CUDA(cudaMalloc(&d_input, size_input));
    CHECK_CUDA(cudaMalloc(&d_weight, size_weight));
    CHECK_CUDA(cudaMalloc(&d_output, size_output));

    CHECK_CUDA(cudaMemcpy(d_input, h_input.data(), size_input, cudaMemcpyHostToDevice));
    CHECK_CUDA(cudaMemcpy(d_weight, h_weight.data(), size_weight, cudaMemcpyHostToDevice));

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    float ms = 0.0f;

    std::cout << "--- 卷积参数 ---" << std::endl;
    std::cout << "输入: " << in_h << "x" << in_w << "x" << in_c << std::endl;
    std::cout << "卷积核: " << k_size << "x" << k_size << ", 数量: " << out_c << ", 步长: " << stride << ", 填充: " << pad << std::endl;
    std::cout << "输出: " << out_h << "x" << out_w << "x" << out_c << std::endl;
    std::cout << "----------------\n" << std::endl;

    // ==========================================
    // 1. 测试滑窗法 (Direct Convolution) 
    // ==========================================
    dim3 block_dim(16, 16, 1);
    dim3 grid_dim((out_w + block_dim.x - 1) / block_dim.x, 
                  (out_h + block_dim.y - 1) / block_dim.y, 
                  (out_c + block_dim.z - 1) / block_dim.z);

    cudaEventRecord(start);
    conv2d_sliding_window_kernel<<<grid_dim, block_dim>>>(d_input, d_weight, d_output, in_c, out_c, in_h, in_w, k_size, stride, pad, out_h, out_w);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&ms, start, stop);
    std::cout << "[方法 1] 滑窗法执行时间: " << ms << " ms" << std::endl;

    // ==========================================
    // 2. 测试 im2col + cuBLAS 
    // ==========================================
    float *d_col;
    int col_rows = in_c * k_size * k_size; // 列向量的高度 
    int col_cols = out_h * out_w;          // 窗口数量 

    CHECK_CUDA(cudaMalloc(&d_col, col_rows * col_cols * sizeof(float)));

    int total_elements = in_c * out_h * out_w;
    int threads_per_block = 256;
    int blocks_per_grid = (total_elements + threads_per_block - 1) / threads_per_block;

    cublasHandle_t cublas_handle;
    CHECK_CUBLAS(cublasCreate(&cublas_handle));
    float alpha = 1.0f, beta = 0.0f;

    cudaEventRecord(start);
    // 步骤 a: im2col 重排
    im2col_kernel<<<blocks_per_grid, threads_per_block>>>(d_input, d_col, in_c, in_h, in_w, k_size, stride, pad, out_h, out_w);
    // 步骤 b: SGEMM 矩阵乘法 W * X 
    // cuBLAS 是列优先格式，我们通过调换操作数并转置计算 C^T = B^T * A^T
    CHECK_CUBLAS(cublasSgemm(cublas_handle, CUBLAS_OP_N, CUBLAS_OP_N, 
                             col_cols, out_c, col_rows,
                             &alpha, 
                             d_col, col_cols, 
                             d_weight, col_rows, 
                             &beta, 
                             d_output, col_cols));
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&ms, start, stop);
    std::cout << "[方法 2] im2col+cuBLAS 执行时间: " << ms << " ms" << std::endl;

    CHECK_CUDA(cudaFree(d_col));
    cublasDestroy(cublas_handle);

    // ==========================================
    // 3. 测试 cuDNN API 
    // ==========================================
    cudnnHandle_t cudnn;
    CHECK_CUDNN(cudnnCreate(&cudnn));

    cudnnTensorDescriptor_t in_desc, out_desc;
    cudnnFilterDescriptor_t filt_desc;
    cudnnConvolutionDescriptor_t conv_desc;

    CHECK_CUDNN(cudnnCreateTensorDescriptor(&in_desc));
    CHECK_CUDNN(cudnnCreateTensorDescriptor(&out_desc));
    CHECK_CUDNN(cudnnCreateFilterDescriptor(&filt_desc));
    CHECK_CUDNN(cudnnCreateConvolutionDescriptor(&conv_desc));

    CHECK_CUDNN(cudnnSetTensor4dDescriptor(in_desc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT, N, in_c, in_h, in_w));
    CHECK_CUDNN(cudnnSetFilter4dDescriptor(filt_desc, CUDNN_DATA_FLOAT, CUDNN_TENSOR_NCHW, out_c, in_c, k_size, k_size));
    CHECK_CUDNN(cudnnSetConvolution2dDescriptor(conv_desc, pad, pad, stride, stride, 1, 1, CUDNN_CROSS_CORRELATION, CUDNN_DATA_FLOAT));
    CHECK_CUDNN(cudnnSetTensor4dDescriptor(out_desc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT, N, out_c, out_h, out_w));

    // 获取前向卷积算法并分配工作空间
    cudnnConvolutionFwdAlgo_t algo;
    int returnedAlgoCount;
    cudnnConvolutionFwdAlgoPerf_t perfResults;
    CHECK_CUDNN(cudnnGetConvolutionForwardAlgorithm_v7(cudnn, in_desc, filt_desc, conv_desc, out_desc, 1, &returnedAlgoCount, &perfResults));
    algo = perfResults.algo;

    size_t workspace_size = 0;
    CHECK_CUDNN(cudnnGetConvolutionForwardWorkspaceSize(cudnn, in_desc, filt_desc, conv_desc, out_desc, algo, &workspace_size));
    void* d_workspace{nullptr};
    if (workspace_size > 0) {
        CHECK_CUDA(cudaMalloc(&d_workspace, workspace_size));
    }

    cudaEventRecord(start);
    CHECK_CUDNN(cudnnConvolutionForward(cudnn, &alpha, in_desc, d_input, filt_desc, d_weight, conv_desc, algo, d_workspace, workspace_size, &beta, out_desc, d_output));
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&ms, start, stop);
    std::cout << "[方法 3] cuDNN 执行时间: " << ms << " ms" << std::endl;

    // 清理资源
    if (d_workspace) cudaFree(d_workspace);
    cudnnDestroyTensorDescriptor(in_desc);
    cudnnDestroyTensorDescriptor(out_desc);
    cudnnDestroyFilterDescriptor(filt_desc);
    cudnnDestroyConvolutionDescriptor(conv_desc);
    cudnnDestroy(cudnn);

    CHECK_CUDA(cudaFree(d_input));
    CHECK_CUDA(cudaFree(d_weight));
    CHECK_CUDA(cudaFree(d_output));

    return 0;
}