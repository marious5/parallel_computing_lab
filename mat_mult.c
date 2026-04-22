#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>

int thread_count;
int m, n, k;
double **A, **B, **C;

// 获取当前时间的函数，用于计算耗时
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 动态分配二维数组
double** alloc_2d_array(int rows, int cols) {
    double **array = (double**)malloc(rows * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        array[i] = (double*)malloc(cols * sizeof(double));
    }
    return array;
}

// 释放二维数组
void free_2d_array(double **array, int rows) {
    for (int i = 0; i < rows; i++) free(array[i]);
    free(array);
}

// 线程运行函数：计算矩阵乘法的一部分
void* Pth_mat_mult(void* rank) {
    long my_rank = (long) rank;
    int local_m = m / thread_count; 
    int my_first_row = my_rank * local_m;
    int my_last_row = (my_rank + 1) * local_m - 1;
    
    // 处理不能整除的剩余行，全部分配给最后一个线程
    if (my_rank == thread_count - 1) {
        my_last_row = m - 1;
    }

    for (int i = my_first_row; i <= my_last_row; i++) {
        for (int j = 0; j < k; j++) {
            C[i][j] = 0.0;
            for (int x = 0; x < n; x++) {
                C[i][j] += A[i][x] * B[x][j];
            }
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "用法: %s <thread_count> <m> <n> <k>\n", argv[0]);
        exit(0);
    }

    thread_count = strtol(argv[1], NULL, 10);
    m = strtol(argv[2], NULL, 10);
    n = strtol(argv[3], NULL, 10);
    k = strtol(argv[4], NULL, 10);

    // 1. 分配内存
    A = alloc_2d_array(m, n);
    B = alloc_2d_array(n, k);
    C = alloc_2d_array(m, k);

    // 2. 随机初始化矩阵 A 和 B
    for(int i=0; i<m; i++)
        for(int j=0; j<n; j++)
            A[i][j] = (rand() % 100) / 10.0;
            
    for(int i=0; i<n; i++)
        for(int j=0; j<k; j++)
            B[i][j] = (rand() % 100) / 10.0;

    pthread_t* thread_handles = malloc(thread_count * sizeof(pthread_t));

    // 3. 开始计时并执行多线程
    double start_time = get_time();
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], NULL, Pth_mat_mult, (void*)thread);
    }
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL);
    }
    double end_time = get_time();
    
    printf("线程数: %d, 矩阵规模: %dx%d * %dx%d\n", thread_count, m, n, n, k);
    printf("并行计算时间: %f 秒\n", end_time - start_time);

    // 4. 验证结果 (串行计算比对)
    printf("正在验证计算结果...\n");
    int correct = 1;
    for(int i=0; i<m; i++) {
        for(int j=0; j<k; j++) {
            double temp = 0.0;
            for(int x=0; x<n; x++) temp += A[i][x] * B[x][j];
            if(fabs(temp - C[i][j]) > 1e-5) {
                correct = 0;
                break;
            }
        }
        if(!correct) break;
    }
    if (correct) printf("验证通过: 并行计算结果正确！\n");
    else printf("验证失败: 并行计算结果错误！\n");

    // 5. 释放资源
    free_2d_array(A, m); free_2d_array(B, n); free_2d_array(C, m);
    free(thread_handles);

    return 0;
}