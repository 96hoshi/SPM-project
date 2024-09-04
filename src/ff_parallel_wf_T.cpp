#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <iomanip>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;


using Matrix = std::vector<std::vector<double>>;


void computeUpperTriangleParallelForWithTranspose(Matrix& M, Matrix& MT, int NW) {
    int n = M.size();
    int max = n - 1;
    ParallelFor pf(NW);

    for (int d = 1; d < n; ++d) {  // Iterate over diagonals
        pf.parallel_for(0, n - d, 1, [&](const long i) {
                int j = d + i;

                double sum = 0.0;
                for (int k = i; k < j; ++k) {
                    // sum += M[h][k] * M[k + 1][j];
                    sum += M[i][k] * MT[max - j][max - (k + 1)];
                }
                sum = cbrt(sum);
                M[i][j] = sum;
                MT[max - j][max - i] = sum;
        });  // Number of elements in each block
    }
}


int main(int argc, char *argv[]) {
    uint64_t N = 516;    // default size of the matrix (NxN)
    int nw = 4;          // default number of workers

    if (argc != 2 && argc != 3) {
		std::printf("use: %s N nw\n", argv[0]);
		std::printf("     N size of the square matrix\n");
        std::printf("     nw number of workers\n");
		return -1;
	}
	if (argc > 2) {
		N = std::stol(argv[1]);
        nw = std::stoi(argv[2]);
	}
    
    // Allocate the matrix as a single vector
    Matrix M(N, std::vector<double>(N, 0.0));
    Matrix MT(N, std::vector<double>(N, 0.0));

    // Initialize major diagonal
    for(uint64_t i = 0; i < N; ++i) {
        M[i][i] = static_cast<double>(i + 1) / N;
        MT[(N - 1) - i][(N - 1) - i] = static_cast<double>(double(i+1) / N);
    }
    
    #ifdef BENCHMARK
        auto a = std::chrono::system_clock::now();
        computeUpperTriangleParallelForWithTranspose(M, MT, nw);
        auto b = std::chrono::system_clock::now();
        std::chrono::duration<double> delta = b-a;
        std::cout << std::fixed << std::setprecision(6) << delta.count() << std::endl;
    #else
        // parallelwavefront(M, N, nw);
        computeUpperTriangleParallelForWithTranspose(M, MT, nw);
        std::cout << M[0][N - 1] << std::endl;
    #endif

    return 0;
}
