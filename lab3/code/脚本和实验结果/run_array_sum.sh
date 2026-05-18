#!/bin/bash

# 1. 编译代码
echo "正在编译 array_sum.c ..."
gcc -g -Wall -O2 -o array_sum array_sum.c -lpthread
if [ $? -ne 0 ]; then
    echo "编译失败，请检查代码！"
    exit 1
fi

# 2. 定义测试参数 (1M, 16M, 64M, 128M)
threads=(1 2 4 8 16)
sizes=(1048576 16777216 67108864 134217728)
output_file="array_sum_results.csv"

# 3. 初始化 CSV 文件表头
echo "线程数,数组规模,耗时(秒)" > $output_file

echo "开始执行数组求和批量测试..."
echo "----------------------------------------"

for t in "${threads[@]}"; do
    for s in "${sizes[@]}"; do
        # 将数字转换为更易读的 M 单位用于终端显示
        size_in_m=$((s / 1048576))
        echo "正在运行: 线程数 = $t, 数组规模 = ${size_in_m}M"
        
        run_output=$(./array_sum $t $s | grep "并行计算时间")
        time_cost=$(echo $run_output | awk '{print $2}')
        
        echo "$t,$size_in_m M,$time_cost" >> $output_file
    done
done

echo "----------------------------------------"
echo "测试完成！结果已保存至: $output_file"