#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

int thread_count;
long long n;
long long *A;
long long global_sum = 0;
pthread_mutex_t mutex;

// 获取当前时间
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 线程运行函数：局部求和然后互斥累加到全局
void* Pth_sum(void* rank) {
    long my_rank = (long) rank;
    long long local_n = n / thread_count;
    long long my_first_i = my_rank * local_n;
    long long my_last_i = (my_rank + 1) * local_n - 1;
    
    // 最后一个线程处理剩余的元素
    if (my_rank == thread_count - 1) {
        my_last_i = n - 1;
    }

    // 使用局部变量避免大量的互斥锁开销
    long long my_sum = 0;
    for (long long i = my_first_i; i <= my_last_i; i++) {
        my_sum += A[i];
    }

    // 局部求和完成后，再安全地累加到全局变量
    pthread_mutex_lock(&mutex);
    global_sum += my_sum;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "用法: %s <thread_count> <n>\n", argv[0]);
        exit(0);
    }

    thread_count = strtol(argv[1], NULL, 10);
    n = strtoll(argv[2], NULL, 10);

    // 1. 分配并初始化数组
    A = (long long*)malloc(n * sizeof(long long));
    if (A == NULL) {
        fprintf(stderr, "内存分配失败！\n");
        exit(1);
    }
    
    for (long long i = 0; i < n; i++) {
        A[i] = rand() % 100; // 填入随机整数
    }

    pthread_t* thread_handles = malloc(thread_count * sizeof(pthread_t));
    pthread_mutex_init(&mutex, NULL);

    // 2. 开始计时并执行多线程
    double start_time = get_time();
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], NULL, Pth_sum, (void*)thread);
    }
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL);
    }
    double end_time = get_time();

    printf("线程数: %d, 数组规模: %lld\n", thread_count, n);
    printf("并行计算时间: %f 秒\n", end_time - start_time);
    printf("并行求和结果: %lld\n", global_sum);

    // 3. 验证结果 (串行计算验证) [cite: 41]
    printf("正在验证计算结果...\n");
    long long serial_sum = 0;
    for (long long i = 0; i < n; i++) {
        serial_sum += A[i];
    }
    if (serial_sum == global_sum) {
        printf("验证通过: 结果一致 (%lld)\n", serial_sum);
    } else {
        printf("验证失败: 串行结果 %lld, 并行结果 %lld\n", serial_sum, global_sum);
    }

    // 4. 清理
    free(A);
    free(thread_handles);
    pthread_mutex_destroy(&mutex);

    return 0;
}