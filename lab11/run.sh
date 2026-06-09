#!/bin/bash

# 退出遇到错误停止执行
set -e

# 定义源码和生成的可执行文件名字
SOURCE_FILE="conv.cu"
EXEC_FILE="conv_benchmark"

echo "======================================="
echo " 开始编译 CUDA 卷积程序..."
echo " 源文件: $SOURCE_FILE"
echo "======================================="

# 使用 nvcc 编译，需链接 cudnn 和 cublas
# 注意：如果是 Ampere 架构显卡（如 A100）可以用 -arch=sm_80
# 这里默认使用 -arch=sm_70 (Volta 架构如 V100 等兼容性较好的配置)
nvcc -O3 \
     -arch=sm_70 \
     $SOURCE_FILE \
     -lcudnn \
     -lcublas \
     -o $EXEC_FILE

echo "编译成功！开始执行测试..."
echo "======================================="

# 运行编译出的程序
./$EXEC_FILE

echo "======================================="
echo " 测试完成！"