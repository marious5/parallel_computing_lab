#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

long long n;          // 总点数
int thread_count;     // 线程数
long long total_m = 0;// 落在圆内的总点数
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* monte_carlo_pi(void* rank) {
    long my_rank = (long)rank;
    long long local_n = n / thread_count;
    long long local_m = 0;
    
    // 使用线程安全的随机数生成器
    unsigned int seed = (unsigned int)my_rank + time(NULL);

    for (long long i = 0; i < local_n; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        if (x * x + y * y <= 1.0) {
            local_m++;
        }
    }

    // 汇总到全局变量
    pthread_mutex_lock(&mutex);
    total_m += local_m;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <total_points> <thread_count>\n", argv[0]);
        return -1;
    }
    n = atoll(argv[1]);
    thread_count = atoi(argv[2]);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    pthread_t* threads = malloc(thread_count * sizeof(pthread_t));
    for (long i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, monte_carlo_pi, (void*)i);
    }

    for (long i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    
    double pi_estimate = 4.0 * total_m / (double)(n);

    printf("Total Points (n) = %lld\n", n);
    printf("Points in Circle (m) = %lld\n", total_m);
    printf("Estimated Pi = %f\n", pi_estimate);
    printf("Time elapsed: %f seconds\n", time_spent);

    free(threads);
    return 0;
}#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

long long n;          // 总点数
int thread_count;     // 线程数
long long total_m = 0;// 落在圆内的总点数
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* monte_carlo_pi(void* rank) {
    long my_rank = (long)rank;
    long long local_n = n / thread_count;
    long long local_m = 0;
    
    // 使用线程安全的随机数生成器
    unsigned int seed = (unsigned int)my_rank + time(NULL);

    for (long long i = 0; i < local_n; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        if (x * x + y * y <= 1.0) {
            local_m++;
        }
    }

    // 汇总到全局变量
    pthread_mutex_lock(&mutex);
    total_m += local_m;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <total_points> <thread_count>\n", argv[0]);
        return -1;
    }
    n = atoll(argv[1]);
    thread_count = atoi(argv[2]);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    pthread_t* threads = malloc(thread_count * sizeof(pthread_t));
    for (long i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, monte_carlo_pi, (void*)i);
    }

    for (long i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    
    double pi_estimate = 4.0 * total_m / (double)(n);

    printf("Total Points (n) = %lld\n", n);
    printf("Points in Circle (m) = %lld\n", total_m);
    printf("Estimated Pi = %f\n", pi_estimate);
    printf("Time elapsed: %f seconds\n", time_spent);

    free(threads);
    return 0;
}