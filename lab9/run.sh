#!/bin/bash
# 编译CUDA程序
nvcc -O3 matrix_mul.cu -o matrix_mul

echo "开始执行CUDA矩阵乘法性能测试..."

# 1. 实验要求：规模在 [128, 2048] 之间
# 分析不同矩阵规模的影响 (固定BlockSize为16)
echo "--- 测试一：矩阵规模对性能的影响 ---"
for size in 128 512 1024 2048; do
    ./matrix_mul $size $size $size 16 0  # 运行Naive版本
    ./matrix_mul $size $size $size 16 1  # 运行Shared版本
done

# 2. 分析不同线程块大小对性能的影响 (固定规模为2048)
echo "--- 测试二：线程块大小的影响 ---"
for bsize in 4 8 16 32; do
    ./matrix_mul 2048 2048 2048 $bsize 0
    ./matrix_mul 2048 2048 2048 $bsize 1
done

echo "测试完成！"