// compile:
// g++ -std=c++20 -O3 -march=native -I<path-to-include> sequential.cpp -o UTW
//
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>

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
                v_mk[j] = M[(m + k - j) * N + (m + k)]; // M[i+k-j][i+k]
            }
            // Store the result in the matrix M
            M[m * N + m + k] = dotProduct(v_m, v_mk);   // M[i][i+k]
		}
	}
}

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

	TIMERSTART(wavefront);
	wavefront(M, N); 
    TIMERSTOP(wavefront);

	printMatrix(M, N);

    return 0;
}
