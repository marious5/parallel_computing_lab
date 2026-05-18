#!/bin/bash

# 编译 C++ 代码
g++ -O3 -fopenmp omp_matmul.cpp -o omp_matmul

# 定义测试参数
SIZES=(128 512 1024 2048)
THREADS=(1 2 4 8 16)
SCHEDULES=("static" "dynamic" "auto") # auto 代表由编译器/运行时决定的默认调度

echo "开始并行矩阵乘法性能测试..."

for size in "${SIZES[@]}"; do
    echo "====================================="
    echo "测试矩阵规模: ${size} x ${size} x ${size}"
    
    for schedule_type in "${SCHEDULES[@]}"; do
        export OMP_SCHEDULE="$schedule_type"
        echo "--> 调度策略: ${schedule_type}"
        
        for thread_num in "${THREADS[@]}"; do
            export OMP_NUM_THREADS=$thread_num
            ./omp_matmul $size $size $size
        done
        echo "-------------------------------------"
    done
done

echo "测试完成！"