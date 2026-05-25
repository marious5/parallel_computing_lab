#!/bin/bash
# 编译程序
mpicxx -O2 -o fft_mpi fft_mpi.cpp
mpicxx -O2 -o fft_serial fft_serial.cpp

# 1. 并行性能分析 (测试不同进程数)
echo "=== Running Serial ==="
./fft_serial > output_serial.log

for p in 2 4 8 16; do
    echo "=== Running MPI with $p processes ==="
    mpirun -np $p ./fft_mpi > output_mpi_$p.log
done

# 2. 内存消耗分析 (Valgrind Massif) 按照实验文档要求增加 --stacks=yes 参数
echo "=== Profiling Memory with Valgrind Massif ==="
# 这里取 -np 4 进行内存分析，运行完会生成 massif.out.<pid> 文件
valgrind --tool=massif --stacks=yes mpirun -np 4 ./fft_mpi

# 找到生成的 massif.out 文件并通过 ms_print 打印
MASSIF_FILE=$(ls massif.out.* | head -n 1)
ms_print $MASSIF_FILE > memory_analysis.txt
echo "Memory profile saved to memory_analysis.txt"