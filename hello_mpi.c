#include <mpi.h>

#include <stdio.h>



int main(int argc, char** argv) {

    // 初始化 MPI 环境

    MPI_Init(&argc, &argv);



    // 获取当前进程的总数

    int world_size;

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);



    // 获取当前进程的编号 (Rank)

    int world_rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);



    // 打印带有进程编号的信息

    printf("Hello World from processor rank %d out of %d processors\n", world_rank, world_size);



    // 结束 MPI 环境

    MPI_Finalize();

    return 0;

}
