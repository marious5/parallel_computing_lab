#!/bin/bash
# 编译
gcc -O2 exp1_equation.c -o exp1_equation -lpthread -lm
gcc -O2 exp2_montecarlo.c -o exp2_montecarlo -lpthread

echo "--- 实验1：一元二次方程求解 ---"
./exp1_equation 1 -3 2
./exp1_equation 1 2 1

echo -e "\n--- 实验2：蒙特卡洛求Pi (对比不同线程和数据量) ---"
# 测试不同的点数量和线程数
for points in 1024 65536; do
    for threads in 1 2 4 8 16; do
        echo "Points: $points, Threads: $threads"
        ./exp2_montecarlo $points $threads
    done
done