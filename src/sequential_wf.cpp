// compile:
// g++ -std=c++20 -O3 -march=native -I<path-to-include> sequential.cpp -o UTW
//
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>
#include <chrono>

#include "hpc_helpers.hpp"
//#include "utils.hpp"
		

// Function to perform dot product
double dotProduct(const std::vector<double>& v1, const std::vector<double>& v2) {
    double result = 0.0;
    for(size_t i = 0; i < v1.size(); ++i) {
        result += v1[i] * v2[i];
    }
	// return sqrt 3 of the result
	return std::cbrt(result);
}

// Print the matrix
void printMatrix(const std::vector<double> &M, const uint64_t &N) {
	for(uint64_t m = 0; m < N; ++m) {
		for(uint64_t j = 0; j < N; ++j) {
			std::printf("%f ", M[ m * N + j]);
		}
		std::printf("\n");
	}
}

#ifdef V0
void wavefront(std::vector<double> &M, const uint64_t &N) {
	std::vector<double> v_m(1), v_mk(1);

	// For each upper diagonal
    for (uint64_t k = 1; k < N; ++k) {
        v_m.resize(k);
        v_mk.resize(k);

        // For each element in the diagonal
        for (uint64_t m = 0; m < N - k; ++m) {
            for (uint64_t j = 0; j < k; ++j) {
                v_m[j] = M[m * N + (m + j)]; 			// M[i][i+j]
				// column vector to perform the dot product
                v_mk[k - j - 1] = M[(m + k - j) * N + (m + k)]; // M[i+k-j][i+k]
            }
            // Store the result in the matrix M
            M[m * N + m + k] = dotProduct(v_m, v_mk);   // M[i][i+k]
		}
	}
}
#else
//TODO: compare with the row*column product version (1.154813 upper right)
void wavefront(std::vector<double> &M, const uint64_t &N) {
	double result = 0.0;
	uint64_t row_start = 0;
	uint64_t diag_row = 0;

	for (uint64_t k = 1; k < N; ++k) {
		
		for (uint64_t m = 0; m < N - k; ++m) {
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
	}
	
	// clear intermedaite results in the lower triangular part of the matrix
    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = 0; j < i; ++j) {
            M[i * N + j] = 0.0;
        }
    }
}
#endif


int main(int argc, char *argv[]) {
	int min    = 0;      // default minimum time (in microseconds)
	int max    = 1000;   // default maximum time (in microseconds)
	uint64_t N = 512;    // default size of the matrix (NxN)
	
	if (argc != 1 && argc != 2 && argc != 4) {
		std::printf("use: %s N [min max]\n", argv[0]);
		std::printf("     N size of the square matrix\n");
		std::printf("     min waiting time (us)\n");
		std::printf("     max waiting time (us)\n");		
		return -1;
	}
	if (argc > 1) {
		N = std::stol(argv[1]);
		if (argc > 2) {
			min = std::stol(argv[2]);
			max = std::stol(argv[3]);
		}
	}

	// allocate the matrix
	std::vector<double> M(N * N, 0.0);

	// Initialize major diagonal
    for(uint64_t i = 0; i < N; ++i) {
        M[i * N + i] = static_cast<double>(i + 1) / N;
    }

	// TODO: use utils macro
	#ifdef BENCHMARK
		auto a = std::chrono::system_clock::now();
		wavefront(M, N);
		auto b = std::chrono::system_clock::now();
		std::chrono::duration<double> delta = b-a;
		std::cout << std::fixed << std::setprecision(6) << delta.count() << std::endl;
	#else
		wavefront(M, N); 
		printMatrix(M, N);
	#endif

	// TIMERSTART(wavefront);
	// wavefront(M, N); 
	// TIMERSTOP(wavefront);
	// printMatrix(M, N);

    return 0;
}
