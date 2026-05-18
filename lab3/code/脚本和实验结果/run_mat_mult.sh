#!/bin/bash

# 1. 编译代码 (如果尚未编译)
echo "正在编译 mat_mult.c ..."
gcc -g -Wall -O2 -o mat_mult mat_mult.c -lpthread
if [ $? -ne 0 ]; then
    echo "编译失败，请检查代码！"
    exit 1
fi

# 2. 定义测试参数
threads=(1 2 4 8 16)
sizes=(128 256 512 1024 2048)
output_file="mat_mult_results.csv"

# 3. 初始化 CSV 文件表头
echo "线程数,矩阵规模,耗时(秒)" > $output_file

echo "开始执行矩阵乘法批量测试..."
echo "----------------------------------------"

# 4. 双重循环遍历所有组合
for t in "${threads[@]}"; do
    for s in "${sizes[@]}"; do
        echo "正在运行: 线程数 = $t, 矩阵规模 = ${s}x${s}"
        
        # 运行程序，抓取包含“并行计算时间”的那一行，并提取出具体的时间数字
        # 假设你的C语言代码输出格式为 "并行计算时间: 0.123456 秒"
        run_output=$(./mat_mult $t $s $s $s | grep "并行计算时间")
        
        # 使用 awk 提取第二个字段（即时间数字）
        time_cost=$(echo $run_output | awk '{print $2}')
        
        # 将结果追加到 CSV 文件中
        echo "$t,$s,$time_cost" >> $output_file
    done
done

echo "----------------------------------------"
echo "测试完成！结果已保存至: $output_file"