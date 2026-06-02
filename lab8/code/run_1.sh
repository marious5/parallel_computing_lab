#!/bin/bash
#SBATCH -J cuda_transpose
#SBATCH -p gpu
#SBATCH -N 1
#SBATCH --gres=gpu:1

# 编译代码
nvcc -O3 hello_world.cu -o hello_world

# 运行代码
echo "--- CUDA Hello World ---"
./hello_world