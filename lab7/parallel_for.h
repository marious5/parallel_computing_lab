#ifndef PARALLEL_FOR_H
#define PARALLEL_FOR_H

#ifdef __cplusplus
extern "C" {
#endif

void parallel_for(int start, int end, int inc, 
                  void *(*functor)(int, void*), 
                  void *arg, int num_threads);

#ifdef __cplusplus
}
#endif

#endif