# include <mpi.h>
# include <cstdlib>
# include <iostream>
# include <iomanip>
# include <cmath>
# include <ctime>

using namespace std;

int main ( int argc, char *argv[] );
void ccopy ( int n, double x[], double y[] );
void cfft2 ( int n, double x[], double y[], double w[], double sgn, int rank, int numprocs );
void cffti ( int n, double w[] );
double ggl ( double *seed );
void step ( int n, int mj, double a[], double b[], double c[], double d[], 
  double w[], double sgn, int rank, int numprocs );
void timestamp ( );

//****************************************************************************80

int main ( int argc, char *argv[] )
{
  int rank, numprocs;
  
  // 初始化 MPI 环境
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

  double ctime;
  double ctime1;
  double ctime2;
  double error;
  int first;
  double flops;
  double fnm1;
  int i;
  int icase;
  int it;
  int ln2;
  double mflops;
  int n;
  int nits = 10000;
  static double seed;
  double sgn;
  double *w;
  double *x;
  double *y;
  double *z;
  double z0;
  double z1;

  if (rank == 0) {
    timestamp ( );
    cout << "\n";
    cout << "FFT_MPI\n";
    cout << "  C++ version with MPI\n";
    cout << "\n";
    cout << "  Demonstrate an implementation of the Fast Fourier Transform\n";
    cout << "  of a complex data vector using MPI_Pack/Unpack.\n";
    cout << "\n";
    cout << "  Accuracy check:\n";
    cout << "\n";
    cout << "    FFT ( FFT ( X(1:N) ) ) == N * X(1:N)\n";
    cout << "\n";
    cout << "             N      NITS    Error         Time          Time/Call     MFLOPS\n";
    cout << "\n";
  }

  seed  = 331.0;
  n = 1;

  for ( ln2 = 1; ln2 <= 20; ln2++ )
  {
    n = 2 * n;

    w = new double[  n];
    x = new double[2*n];
    y = new double[2*n];
    z = new double[2*n];

    first = 1;

    for ( icase = 0; icase < 2; icase++ )
    {
      if ( first )
      {
        for ( i = 0; i < 2 * n; i = i + 2 )
        {
          z0 = ggl ( &seed );
          z1 = ggl ( &seed );
          x[i] = z0;
          z[i] = z0;
          x[i+1] = z1;
          z[i+1] = z1;
        }
      } 
      else
      {
        for ( i = 0; i < 2 * n; i = i + 2 )
        {
          z0 = 0.0;
          z1 = 0.0;
          x[i] = z0;
          z[i] = z0;
          x[i+1] = z1;
          z[i+1] = z1;
        }
      }

      cffti ( n, w );

      if ( first )
      {
        sgn = + 1.0;
        cfft2 ( n, x, y, w, sgn, rank, numprocs );
        sgn = - 1.0;
        cfft2 ( n, y, x, w, sgn, rank, numprocs );

        if (rank == 0) {
            fnm1 = 1.0 / ( double ) n;
            error = 0.0;
            for ( i = 0; i < 2 * n; i = i + 2 )
            {
              error = error 
              + pow ( z[i]   - fnm1 * x[i], 2 )
              + pow ( z[i+1] - fnm1 * x[i+1], 2 );
            }
            error = sqrt ( fnm1 * error );
            cout << "  " << setw(12) << n
                 << "  " << setw(8) << nits
                 << "  " << setw(12) << error;
        }
        first = 0;
      }
      else
      {
        MPI_Barrier(MPI_COMM_WORLD);
        ctime1 = MPI_Wtime(); 
        for ( it = 0; it < nits; it++ )
        {
          sgn = + 1.0;
          cfft2 ( n, x, y, w, sgn, rank, numprocs );
          sgn = - 1.0;
          cfft2 ( n, y, x, w, sgn, rank, numprocs );
        }
        MPI_Barrier(MPI_COMM_WORLD);
        ctime2 = MPI_Wtime();
        ctime = ctime2 - ctime1;

        if (rank == 0) {
            flops = 2.0 * ( double ) nits * ( 5.0 * ( double ) n * ( double ) ln2 );
            mflops = flops / 1.0E+06 / ctime;
            cout << "  " << setw(12) << ctime
                 << "  " << setw(12) << ctime / ( double ) ( 2 * nits )
                 << "  " << setw(12) << mflops << "\n";
        }
      }
    }
    if ( ( ln2 % 4 ) == 0 ) 
    {
      nits = nits / 10;
    }
    if ( nits < 1 ) 
    {
      nits = 1;
    }
    delete [] w;
    delete [] x;
    delete [] y;
    delete [] z;
  }

  if (rank == 0) {
      cout << "\n";
      cout << "FFT_MPI:\n";
      cout << "  Normal end of execution.\n";
      cout << "\n";
      timestamp ( );
  }

  MPI_Finalize();
  return 0;
}
//****************************************************************************80

void ccopy ( int n, double x[], double y[] )
{
  int i;
  for ( i = 0; i < n; i++ )
  {
    y[i*2+0] = x[i*2+0];
    y[i*2+1] = x[i*2+1];
  }
  return;
}
//****************************************************************************80

void cfft2 ( int n, double x[], double y[], double w[], double sgn, int rank, int numprocs )
{
  int j;
  int m;
  int mj;
  int tgle;

  m = ( int ) ( log ( ( double ) n ) / log ( 1.99 ) );
  mj   = 1;
  tgle = 1;
  step ( n, mj, &x[0*2+0], &x[(n/2)*2+0], &y[0*2+0], &y[mj*2+0], w, sgn, rank, numprocs );

  if ( n == 2 )
  {
    return;
  }

  for ( j = 0; j < m - 2; j++ )
  {
    mj = mj * 2;
    if ( tgle )
    {
      step ( n, mj, &y[0*2+0], &y[(n/2)*2+0], &x[0*2+0], &x[mj*2+0], w, sgn, rank, numprocs );
      tgle = 0;
    }
    else
    {
      step ( n, mj, &x[0*2+0], &x[(n/2)*2+0], &y[0*2+0], &y[mj*2+0], w, sgn, rank, numprocs );
      tgle = 1;
    }
  }

  if ( tgle ) 
  {
    ccopy ( n, y, x );
  }

  mj = n / 2;
  step ( n, mj, &x[0*2+0], &x[(n/2)*2+0], &y[0*2+0], &y[mj*2+0], w, sgn, rank, numprocs );

  return;
}
//****************************************************************************80

void cffti ( int n, double w[] )
{
  double arg;
  double aw;
  int i;
  int n2;
  const double pi = 3.141592653589793;

  n2 = n / 2;
  aw = 2.0 * pi / ( ( double ) n );

  for ( i = 0; i < n2; i++ )
  {
    arg = aw * ( ( double ) i );
    w[i*2+0] = cos ( arg );
    w[i*2+1] = sin ( arg );
  }
  return;
}
//****************************************************************************80

double ggl ( double *seed )
{
  double d2 = 0.2147483647e10;
  double t;
  double value;

  t = *seed;
  t = fmod ( 16807.0 * t, d2 );
  *seed = t;
  value = ( t - 1.0 ) / ( d2 - 1.0 );

  return value;
}
//****************************************************************************80

void step ( int n, int mj, double a[], double b[], double c[],
  double d[], double w[], double sgn, int rank, int numprocs )
{
  double ambr;
  double ambu;
  int mj2;
  double wjw[2];

  mj2 = 2 * mj;
  int lj = n / mj2;

  // 1. 根据进程数划分外层循环任务
  int local_lj = lj / numprocs;
  int remainder = lj % numprocs;
  int start_j = rank * local_lj + (rank < remainder ? rank : remainder);
  int end_j = start_j + local_lj + (rank < remainder ? 1 : 0);
  int actual_lj = end_j - start_j;

  // 2. 本地计算分配到的块
  for ( int j = start_j; j < end_j; j++ )
  {
    int jw = j * mj;
    int ja  = jw;
    int jb  = ja;
    int jc  = j * mj2;
    int jd  = jc;

    wjw[0] = w[jw*2+0]; 
    wjw[1] = w[jw*2+1];

    if ( sgn < 0.0 ) 
    {
      wjw[1] = - wjw[1];
    }

    for ( int k = 0; k < mj; k++ )
    {
      c[(jc+k)*2+0] = a[(ja+k)*2+0] + b[(jb+k)*2+0];
      c[(jc+k)*2+1] = a[(ja+k)*2+1] + b[(jb+k)*2+1];

      ambr = a[(ja+k)*2+0] - b[(jb+k)*2+0];
      ambu = a[(ja+k)*2+1] - b[(jb+k)*2+1];

      d[(jd+k)*2+0] = wjw[0] * ambr - wjw[1] * ambu;
      d[(jd+k)*2+1] = wjw[1] * ambr + wjw[0] * ambu;
    }
  }

  // 3. 准备 MPI_Pack
  int pack_size_per_double;
  MPI_Pack_size(1, MPI_DOUBLE, MPI_COMM_WORLD, &pack_size_per_double);
  
  // 计算当前进程需要打包的最大空间：实际处理的循环次数 * 每次内层循环产生的双精度浮点数数量 (mj*4) 
  int local_pack_size = actual_lj * mj * 4 * pack_size_per_double;
  
  // 如果分配到的任务为 0，依然需要分配极小空间以防越界
  char* pack_buffer = new char[local_pack_size > 0 ? local_pack_size : 1];
  int position = 0;

  for ( int j = start_j; j < end_j; j++ ) {
      int jc = j * mj2;
      int jd = jc; 
      // 打包 c 数组的更新块
      MPI_Pack(&c[jc*2], mj*2, MPI_DOUBLE, pack_buffer, local_pack_size, &position, MPI_COMM_WORLD);
      // 打包 d 数组的更新块
      MPI_Pack(&d[jd*2], mj*2, MPI_DOUBLE, pack_buffer, local_pack_size, &position, MPI_COMM_WORLD); 
  }

  // 4. 同步各进程打包后的字节数
  int* recvcounts = new int[numprocs];
  int* displs = new int[numprocs];
  MPI_Allgather(&position, 1, MPI_INT, recvcounts, 1, MPI_INT, MPI_COMM_WORLD);
  
  int total_recv_size = 0;
  for ( int i = 0; i < numprocs; i++ ) {
      displs[i] = total_recv_size;
      total_recv_size += recvcounts[i];
  }
  
  char* recv_buffer = new char[total_recv_size > 0 ? total_recv_size : 1];
  
  // 5. 将所有进程的 packed 数据收集到所有进程的 recv_buffer 中
  MPI_Allgatherv(pack_buffer, position, MPI_PACKED, recv_buffer, recvcounts, displs, MPI_PACKED, MPI_COMM_WORLD);

  // 6. MPI_Unpack: 遍历所有进程的任务范围，按照同样的顺序解包，将数据写回 c 和 d
  for ( int r = 0; r < numprocs; r++ ) {
      int r_start_j = r * local_lj + (r < remainder ? r : remainder);
      int r_end_j = r_start_j + local_lj + (r < remainder ? 1 : 0);
      int unpack_pos = displs[r];
      
      for ( int j = r_start_j; j < r_end_j; j++ ) {
          int jc = j * mj2;
          int jd = jc;
          MPI_Unpack(recv_buffer, total_recv_size, &unpack_pos, &c[jc*2], mj*2, MPI_DOUBLE, MPI_COMM_WORLD);
          MPI_Unpack(recv_buffer, total_recv_size, &unpack_pos, &d[jd*2], mj*2, MPI_DOUBLE, MPI_COMM_WORLD);
      }
  }

  // 清理内存
  delete[] pack_buffer;
  delete[] recv_buffer;
  delete[] recvcounts;
  delete[] displs;
}
//****************************************************************************80

void timestamp ( )
{
# define TIME_SIZE 40

  static char time_buffer[TIME_SIZE];
  const struct tm *tm;
  time_t now;

  now = time ( NULL );
  tm = localtime ( &now );

  strftime ( time_buffer, TIME_SIZE, "%d %B %Y %I:%M:%S %p", tm );

  cout << time_buffer << "\n";

  return;
# undef TIME_SIZE
}