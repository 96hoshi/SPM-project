#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <iomanip>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;


// Print the matrix
void printMatrix(const std::vector<double> &M, const uint64_t &N) {
	for(uint64_t i = 0; i < N; ++i) {
		for(uint64_t j = 0; j < N; ++j) {
			std::printf("%f ", M[ i * N + j]);
		}
		std::printf("\n");
	}
}

void parallelwavefront(std::vector<double> &M, const uint64_t &N, int nw) {
    ParallelFor pf(nw);

    // For each upper diagonal
    for (uint64_t d = 1; d < N; ++d) {
        // Diagonal level parallelism
        pf.parallel_for(0, N - d, 1, [&](const long i) {
            uint64_t j = d + i;
            double sum = 0.0;
            uint64_t row = i * N;
            uint64_t row_t = j * N;

            for (uint64_t k = i; k < j; ++k) {
                sum += M[row + k] *  M[(k + 1) + row_t];
            }
            sum = std::cbrt(sum);
            M[row + j] = sum;
            M[row_t + i] = sum;
        });
    }
     
 // clear intermedaite results in the lower triangular part of the matrix
    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = 0; j < i; ++j) {
            M[i * N + j] = 0.0;
        }
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
	std::vector<double> M(N * N, 0.0);

    // Initialize major diagonal
    for(uint64_t i = 0; i < N; ++i) {
        M[i * N + i] = static_cast<double>(i + 1) / N;
    }
    
    #ifdef BENCHMARK
        auto a = std::chrono::system_clock::now();
        parallelwavefront(M, N, nw);
        auto b = std::chrono::system_clock::now();
        std::chrono::duration<double> delta = b-a;
        std::cout << std::fixed << std::setprecision(6) << delta.count() << std::endl;
    #else
        parallelwavefront(M, N, nw);
        //printMatrix(M, N);
        std::cout << M[N - 1] << std::endl;
    #endif

    return 0;
}
