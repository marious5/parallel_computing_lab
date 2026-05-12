#!/bin/bash

# 遇到错误即停止
set -e 

echo "=== 1. 编译 parallel_for 动态链接库 ==="
gcc -shared -fPIC -Wall -O3 -o libparallel_for.so parallel_for.c -lpthread
echo "动态链接库 libparallel_for.so 生成成功！"

echo "=== 2. 编译测试程序 ==="
# 编译矩阵乘法验证
gcc -Wall -O3 -o test_matmul test_matmul.c -L. -lparallel_for -lpthread

# 编译 Pthreads 版本的 heated_plate
gcc -Wall -O3 -o heated_plate_pthreads heated_plate_pthreads.c -L. -lparallel_for -lpthread -lm

# 编译原版 OpenMP 版本的 heated_plate
gcc -Wall -fopenmp -O3 -o heated_plate_openmp heated_plate_openmp.c -lm

echo "编译成功！"

# 设置环境变量，确保能找到动态链接库
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

echo "=== 3. 运行矩阵乘法正确性验证 ==="
./test_matmul

echo "=== 4. 运行 Heated Plate 对比测试 ==="

echo "--- 运行 OpenMP 版本 ---"
./heated_plate_openmp

# 循环测试不同线程数量下的 Pthreads 版本
for threads in 1 2 4 8; do
    echo "--- 运行 Pthreads 版本 (Threads: $threads) ---"
    ./heated_plate_pthreads $threads
done

echo "全部执行完毕！"