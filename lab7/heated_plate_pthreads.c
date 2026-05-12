#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include "parallel_for.h"

#define M 500
#define N_COLS 500

double u[M][N_COLS];
double w[M][N_COLS];
double row_diffs[M]; // 用于替代 OpenMP reduction 的辅助数组

// --- 参数结构体定义 ---
struct init_args { double (*w)[N_COLS]; double mean; };
struct compute_args { double (*u)[N_COLS]; double (*w)[N_COLS]; };

// --- Functors ---

// 1. 内部网格初始化
void* init_interior_functor(int idx, void* arg) {
    struct init_args* args = (struct init_args*)arg;
    int i = idx;
    for (int j = 1; j < N_COLS - 1; j++) {
        args->w[i][j] = args->mean;
    }
    return NULL;
}

// 2. 复制状态 U = W
void* copy_state_functor(int idx, void* arg) {
    struct compute_args* args = (struct compute_args*)arg;
    int i = idx;
    for (int j = 0; j < N_COLS; j++) {
        args->u[i][j] = args->w[i][j];
    }
    return NULL;
}

// 3. 计算新状态 W (核心热传导方程)
void* compute_w_functor(int idx, void* arg) {
    struct compute_args* args = (struct compute_args*)arg;
    int i = idx;
    for (int j = 1; j < N_COLS - 1; j++) {
        args->w[i][j] = (args->u[i-1][j] + args->u[i+1][j] + 
                         args->u[i][j-1] + args->u[i][j+1]) / 4.0;
    }
    return NULL;
}

// 4. 计算差异 Diff
void* compute_diff_functor(int idx, void* arg) {
    struct compute_args* args = (struct compute_args*)arg;
    int i = idx;
    double my_diff = 0.0;
    for (int j = 1; j < N_COLS - 1; j++) {
        double temp = fabs(args->w[i][j] - args->u[i][j]);
        if (my_diff < temp) {
            my_diff = temp;
        }
    }
    row_diffs[i] = my_diff; // 写入属于当前行的位置，无锁操作
    return NULL;
}

// 获取高精度时间
double get_wtime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int main(int argc, char *argv[]) {
    int num_threads = 4; // 默认线程数，可考虑从 argv 传入进行不同规模测试
    if (argc > 1) {
        num_threads = atoi(argv[1]);
    }

    double diff;
    double epsilon = 0.001;
    int iterations = 0;
    int iterations_print = 1;
    double mean = 0.0;
    double wtime;

    printf("\nHEATED_PLATE_PTHREADS\n");
    printf("  Spatial grid of %d by %d points.\n", M, N_COLS);
    printf("  Number of threads = %d\n", num_threads);

    // 边界初始化 (由于边界极小，直接串行初始化避免线程创建开销)
    for (int i = 1; i < M - 1; i++) w[i][0] = 100.0;
    for (int i = 1; i < M - 1; i++) w[i][N_COLS-1] = 100.0;
    for (int j = 0; j < N_COLS; j++) w[M-1][j] = 100.0;
    for (int j = 0; j < N_COLS; j++) w[0][j] = 0.0;

    // 计算均值
    for (int i = 1; i < M - 1; i++) mean += (w[i][0] + w[i][N_COLS-1]);
    for (int j = 0; j < N_COLS; j++) mean += (w[M-1][j] + w[0][j]);
    mean = mean / (double)(2 * M + 2 * N_COLS - 4);
    printf("\n  MEAN = %f\n", mean);

    // 并行初始化内部
    struct init_args i_args = {w, mean};
    parallel_for(1, M - 1, 1, init_interior_functor, (void*)&i_args, num_threads);

    printf("\n Iteration  Change\n\n");
    wtime = get_wtime();
    diff = epsilon;

    struct compute_args c_args = {u, w};

    while (epsilon <= diff) {
        // 1. 并行拷贝旧状态
        parallel_for(0, M, 1, copy_state_functor, (void*)&c_args, num_threads);
        
        // 2. 并行计算新状态
        parallel_for(1, M - 1, 1, compute_w_functor, (void*)&c_args, num_threads);
        
        // 3. 并行计算局部差异
        parallel_for(1, M - 1, 1, compute_diff_functor, (void*)&c_args, num_threads);

        // 4. 串行归约全局最大 diff
        diff = 0.0;
        for (int i = 1; i < M - 1; i++) {
            if (diff < row_diffs[i]) {
                diff = row_diffs[i];
            }
        }

        iterations++;
        if (iterations == iterations_print) {
            printf("  %8d  %f\n", iterations, diff);
            iterations_print = 2 * iterations_print;
        }
    } 
    wtime = get_wtime() - wtime;

    printf("\n  %8d  %f\n", iterations, diff);
    printf("  Error tolerance achieved.\n");
    printf("  Wallclock time = %f\n", wtime);
    return 0;
}