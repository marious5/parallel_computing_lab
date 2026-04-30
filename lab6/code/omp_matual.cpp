#include <iostream>
#include <vector>
#include <random>
#include <omp.h>
#include <cstdlib>

using namespace std;

void init_matrix(vector<float>& mat, int rows, int cols) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> dis(0.0, 1.0);
    for (int i = 0; i < rows * cols; ++i) {
        mat[i] = dis(gen);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <m> <n> <k>" << endl;
        return 1;
    }

    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);

    vector<float> A(m * n);
    vector<float> B(n * k);
    vector<float> C(m * k, 0.0f);

    init_matrix(A, m, n);
    init_matrix(B, n, k);

    double start_time = omp_get_wtime();

    // OpenMP 并行化，schedule 子句将通过环境变量 OMP_SCHEDULE 控制
    #pragma omp parallel for default(none) shared(A, B, C, m, n, k)
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < k; ++j) {
            float sum = 0.0f;
            for (int l = 0; l < n; ++l) {
                sum += A[i * n + l] * B[l * k + j];
            }
            C[i * k + j] = sum;
        }
    }

    double end_time = omp_get_wtime();
    cout << "Size: " << m << "x" << n << "x" << k 
         << " | Threads: " << omp_get_max_threads() 
         << " | Time: " << (end_time - start_time) << " s" << endl;

    return 0;
}