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

//****************************************************************************80
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
        ctime1 = MPI_Wtime(); // 使用 MPI 时间函数
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
//****************