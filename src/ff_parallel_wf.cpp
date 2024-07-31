#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;


// Function to perform cube root of dot product
double dotProduct(const std::vector<double>& v1, const std::vector<double>& v2) {
    double result = 0.0;
    for(size_t i = 0; i < v1.size(); ++i) {
        result += v1[i] * v2[i];
    }
    return std::cbrt(result);;
}

// Print the matrix
void printMatrix(const std::vector<double> &M, const uint64_t &N) {
	for(uint64_t i = 0; i < N; ++i) {
		for(uint64_t j = 0; j < N; ++j) {
			std::printf("%f ", M[ i * N + j]);
		}
		std::printf("\n");
	}
}

void parallelwavefront(std::vector<double> &M, const uint64_t &N, const int nw = 4) {
    ParallelFor pf(nw);

    double result = 0.0;
	uint64_t row_start = 0;
	uint64_t diag_row = 0;
    
    // For each upper diagonal
    for (uint64_t k = 1; k < N; ++k) {

        // Diagonal level parallelism
        pf.parallel_for(0, N - k, 1, 0, [&](const long m) {
            row_start = m * N;  //index to iterate over the rows
            diag_row = (m + k) * N;  //row of the diagonal element

            result = 0.0;

            for (uint64_t j = 0; j < k; ++j) {
                // read the elements from the lower triangular part of the matrix
                result += M[row_start + (m + j)] * M[diag_row + (m + j + 1)]; // M[m][m+j] * M[m+k][m+j+1]
            }
            // Compute the cube root of the dot product and store the result in the lower triangular part of the matrix
            M[diag_row + m] = std::cbrt(result); // M[m+k][m]
            // Copy the result to the upper triangular part of the matrix
            M[row_start + (m + k)] = M[diag_row + m]; // M[m][m+k] = M[m+k][m]
            }
        );
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
    
    // TODO: use utils macro
    #ifdef BENCHMARK
        auto a = std::chrono::system_clock::now();
        parallelwavefront(M, N, nw);
        auto b = std::chrono::system_clock::now();
        std::chrono::duration<double> delta = b-a;
        std::cout << std::fixed << std::setprecision(6) << delta.count() << std::endl;
    #else
        parallelwavefront(M, N, nw);
        printMatrix(M, N);
    #endif

    return 0;
}
