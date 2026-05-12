#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "parallel_for.h"

// 传递给底层的线程参数结构体
typedef struct {
    int start;
    int end;
    int inc;
    void *(*functor)(int, void*);
    void *arg;
} thread_data_t;

// 线程实际执行的包裹函数
void* thread_wrapper(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    for (int i = data->start; i < data->end; i += data->inc) {
        data->functor(i, data->arg);
    }
    return NULL;
}

void parallel_for(int start, int end, int inc, 
                  void *(*functor)(int, void*), 
                  void *arg, int num_threads) {
    if (num_threads <= 0) num_threads = 1;
    if (inc <= 0) inc = 1; // 简单的容错处理

    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    thread_data_t *t_data = (thread_data_t*)malloc(num_threads * sizeof(thread_data_t));

    // 计算总迭代次数
    int total_iters = (end - start + inc - 1) / inc; 
    
    // 如果任务数少于线程数，减少实际派生的线程数
    int actual_threads = (total_iters < num_threads) ? total_iters : num_threads;
    
    int chunk = total_iters / actual_threads;
    int remainder = total_iters % actual_threads;

    int current_start = start;
    for (int i = 0; i < actual_threads; i++) {
        t_data[i].start = current_start;
        // 处理不能整除的情况，将余数分配给前几个线程
        int current_chunk = chunk + (i < remainder ? 1 : 0);
        t_data[i].end = current_start + current_chunk * inc;
        t_data[i].inc = inc;
        t_data[i].functor = functor;
        t_data[i].arg = arg;

        pthread_create(&threads[i], NULL, thread_wrapper, (void*)&t_data[i]);
        current_start = t_data[i].end;
    }

    // 同步等待所有线程完成
    for (int i = 0; i < actual_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(t_data);
}