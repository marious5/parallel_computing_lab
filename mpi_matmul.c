#include <mpi.h>

#include <stdio.h>

#include <stdlib.h>

#include <time.h>



// 辅助函数：随机初始化矩阵

void init_matrix(double *mat, int rows, int cols) {

    for (int i = 0; i < rows * cols; i++) {

        mat[i] = (double)(rand() % 100) / 10.0; // 生成 0.0 ~ 9.9 的随机浮点数

    }

}



// 辅助函数：打印矩阵（带有大小限制防刷屏）

void print_matrix(const char *name, double *mat, int rows, int cols) {

    if (rows > 16 || cols > 16) {

        printf("矩阵 %s 规模过大 (%dx%d)，省略打印内容。\n", name, rows, cols);

        return;

    }

    printf("--- 矩阵 %s ---\n", name);

    for (int i = 0; i < rows; i++) {

        for (int j = 0; j < cols; j++) {

            printf("%6.2f ", mat[i * cols + j]);

        }

        printf("\n");

    }

    printf("\n");

}



int main(int argc, char** argv) {

    int rank, size;

    int N; // 矩阵规模 N x N



    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_size(MPI_COMM_WORLD, &size);



    // 从命令行参数获取矩阵规模，默认设为 128

    if (argc > 1) {

        N = atoi(argv[1]);

    } else {

        N = 128;

    }



    // 假设 N 总是能被 size 整除 (实验里的 128~2048 和进程数 1~16 均满足)

    if (N % size != 0) {

        if (rank == 0) {

            printf("错误：矩阵规模 %d 无法被进程数 %d 整除！\n", N, size);

        }

        MPI_Finalize();

        return -1;

    }



    int local_rows = N / size; // 每个进程分担的行数



    double *A = NULL;

    double *B = (double*)malloc(N * N * sizeof(double)); // 所有进程都需要完整的矩阵 B

    double *C = NULL;



    double *local_A = (double*)malloc(local_rows * N * sizeof(double));

    double *local_C = (double*)malloc(local_rows * N * sizeof(double));



    // 主进程进行初始化工作

    if (rank == 0) {

        A = (double*)malloc(N * N * sizeof(double));

        C = (double*)malloc(N * N * sizeof(double));



        srand(time(NULL));

        init_matrix(A, N, N);

        init_matrix(B, N, N);

        

        printf("MPI 并行矩阵乘法 (N=%d, 进程数=%d) 开始计算...\n", N, size);

    }



    // ------------------ 计时开始 ------------------

    MPI_Barrier(MPI_COMM_WORLD); // 同步所有进程

    double start_time = MPI_Wtime();



    // 1. 广播完整的矩阵 B 到所有进程

    MPI_Bcast(B, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);



    // 2. 主进程将矩阵 A 均匀分发 (Scatter) 给所有进程的 local_A 中

    MPI_Scatter(A, local_rows * N, MPI_DOUBLE, 

                local_A, local_rows * N, MPI_DOUBLE, 

                0, MPI_COMM_WORLD);



    // 3. 核心计算：各个进程利用自己负责的 local_A 计算 local_C

    for (int i = 0; i < local_rows; i++) {

        for (int j = 0; j < N; j++) {

            local_C[i * N + j] = 0.0;

            for (int k = 0; k < N; k++) {

                local_C[i * N + j] += local_A[i * N + k] * B[k * N + j];

            }

        }

    }



    // 4. 主进程收集 (Gather) 各个进程计算完毕的 local_C，合并为完整的矩阵 C

    MPI_Gather(local_C, local_rows * N, MPI_DOUBLE, 

               C, local_rows * N, MPI_DOUBLE, 

               0, MPI_COMM_WORLD);



    // ------------------ 计时结束 ------------------

    MPI_Barrier(MPI_COMM_WORLD);

    double end_time = MPI_Wtime();



    // 主进程输出结果和耗时

    if (rank == 0) {

        double elapsed_time = end_time - start_time;

        

        // 打印矩阵（根据规模判断是否打印）

        print_matrix("A", A, N, N);

        print_matrix("B", B, N, N);

        print_matrix("C", C, N, N);



        printf("计算完成！耗时: %f 秒\n", elapsed_time);



        // 释放主进程独有的内存

        free(A);

        free(C);

    }



    // 释放公共内存

    free(B);

    free(local_A);

    free(local_C);



    MPI_Finalize();

    return 0;

}
