
#!/bin/bash



EXECUTABLE="./mpi_matmul"

SOURCE_FILE="mpi_matmul.c"

RESULT_FILE="experiment_results.csv"



# 1. 检查并编译代码

if [ ! -f "$EXECUTABLE" ]; then

    echo "未找到可执行文件 $EXECUTABLE，正在自动编译..."

    mpicc -O3 $SOURCE_FILE -o $EXECUTABLE

    if [ $? -ne 0 ]; then

        echo "错误：编译失败，请检查 C 源码或 MPI 环境。"

        exit 1

    fi

    echo "编译成功！"

fi



# 2. 定义要测试的进程数和矩阵规模数组

PROCS=(1 2 4 8 16)

SIZES=(128 256 512 1024 2048)



# 3. 初始化 CSV 结果文件，写入表头

echo "进程数,矩阵规模,耗时(秒)" > $RESULT_FILE



echo "========================================"

echo "          MPI 矩阵乘法自动化测试         "

echo "========================================"

printf "%-10s | %-10s | %-15s\n" "进程数" "矩阵规模" "耗时(秒)"

echo "----------------------------------------"



# 4. 嵌套循环执行测试

for p in "${PROCS[@]}"; do

    for s in "${SIZES[@]}"; do

        

        # 运行 MPI 程序 (已加入 --oversubscribe 防报错)

        OUTPUT=$(mpirun --oversubscribe -np $p $EXECUTABLE $s 2>/dev/null)

        

        # 提取时间数值

        TIME_COST=$(echo "$OUTPUT" | grep "耗时" | awk -F "耗时: " '{print $2}' | awk '{print $1}')

        

        # 处理执行错误

        if [ -z "$TIME_COST" ]; then

            TIME_COST="Failed"

        fi

        

        # 格式化输出到终端

        printf "%-10s | %-10s | %-15s\n" "$p" "$s" "$TIME_COST"

        

        # 写入 CSV 文件

        echo "$p,$s,$TIME_COST" >> $RESULT_FILE

        

    done

done



echo "========================================"

echo "测试全部完成！数据已汇总至当前目录下的: $RESULT_FILE"

