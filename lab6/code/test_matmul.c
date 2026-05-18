#include <stdio.h>
#include <stdlib.h>
#include "parallel_for.h"

#define N 500

struct matmul_args {
    double (*A)[N];
    double (*B)[N];
    double (*C)[N];
};

// Functor：执行矩阵乘法的单行计算
void* matmul_functor(int idx, void* args) {
    struct matmul_args *data = (struct matmul_args*)args;
    int i = idx; // idx 代表行号
    for (int j = 0; j < N; j++) {
        data->C[i][j] = 0;
        for (int k = 0; k < N; k++) {
            data->C[i][j] += data->A[i][k] * data->B[k][j];
        }
    }
    return NULL;
}

int main() {
    struct matmul_args args;
    args.A = malloc(sizeof(double[N][N]));
    args.B = malloc(sizeof(double[N][N]));
    args.C = malloc(sizeof(double[N][N]));

    // 初始化数据
    for(int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            args.A[i][j] = 1.0;
            args.B[i][j] = 2.0;
        }
    }

    printf("Starting Parallel Matrix Multiplication (%dx%d)...\n", N, N);
    
    // 使用 4 个线程进行并行计算
    parallel_for(0, N, 1, matmul_functor, (void*)&args, 4);

    // 简单验证正确性
    printf("Result C[0][0] = %f (Expected: %f)\n", args.C[0][0], N * 2.0);
    
    free(args.A); free(args.B); free(args.C);
    return 0;
}
