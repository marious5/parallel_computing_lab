#!/bin/bash
#SBATCH -J cuda_transpose
#SBATCH -p gpu
#SBATCH -N 1
#SBATCH --gres=gpu:1

# 编译代码
nvcc -O3 matrix_transpose.cu -o matrix_transpose

# 运行代码
echo -e "\n--- CUDA Matrix Transpose ---"
./matrix_transpose