#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>

double a, b, c;
double sqrt_delta, denominator;
double x1, x2;
int delta_ready = 0;
int deno_ready = 0;
int has_real_roots = 1;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* calc_delta(void* arg) {
    double delta = b * b - 4 * a * c;
    pthread_mutex_lock(&mutex);
    if (delta >= 0) {
        sqrt_delta = sqrt(delta);
    } else {
        has_real_roots = 0;
    }
    delta_ready = 1;
    pthread_cond_broadcast(&cond); // 通知等待的计算线程
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void* calc_deno(void* arg) {
    pthread_mutex_lock(&mutex);
    denominator = 2 * a;
    deno_ready = 1;
    pthread_cond_broadcast(&cond); // 通知等待的计算线程
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void* calc_x1(void* arg) {
    pthread_mutex_lock(&mutex);
    while (!delta_ready || !deno_ready) {
        pthread_cond_wait(&cond, &mutex); // 等待中间值计算完成
    }
    if (has_real_roots && denominator != 0) {
        x1 = (-b + sqrt_delta) / denominator;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void* calc_x2(void* arg) {
    pthread_mutex_lock(&mutex);
    while (!delta_ready || !deno_ready) {
        pthread_cond_wait(&cond, &mutex); // 等待中间值计算完成
    }
    if (has_real_roots && denominator != 0) {
        x2 = (-b - sqrt_delta) / denominator;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <a> <b> <c>\n", argv[0]);
        return -1;
    }
    a = atof(argv[1]); b = atof(argv[2]); c = atof(argv[3]);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    pthread_t t_delta, t_deno, t_x1, t_x2;
    pthread_create(&t_delta, NULL, calc_delta, NULL);
    pthread_create(&t_deno, NULL, calc_deno, NULL);
    pthread_create(&t_x1, NULL, calc_x1, NULL);
    pthread_create(&t_x2, NULL, calc_x2, NULL);

    pthread_join(t_delta, NULL);
    pthread_join(t_deno, NULL);
    pthread_join(t_x1, NULL);
    pthread_join(t_x2, NULL);

    gettimeofday(&end, NULL);
    double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    if (!has_real_roots) {
        printf("No real roots.\n");
    } else {
        printf("x1 = %.4f, x2 = %.4f\n", x1, x2);
    }
    printf("Time elapsed: %f seconds\n", time_spent);

    return 0;
}