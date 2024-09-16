#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>
#include <chrono>
#include <iomanip>


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
    uint64_t j = 0;
    double result = 0.0;
    uint64_t row = 0;
    uint64_t row_t = 0;

	for (uint64_t k = 1; k < N; ++k) {
		for (uint64_t m = 0; m < N - k; ++m) {
		row = m * N;  		  //index to iterate over the rows
		row_t = (m + k) * N;  //row of the diagonal element
		result = 0.0;

		for (uint64_t j = 0; j < k; ++j) {
			// read the elements from the lower triangular part of the matrix
			result += M[row + (m + j)] * M[row_t + (m + j + 1)]; // M[m][m+j] * M[m+k][m+j+1]
		}
		result = std::cbrt(result);
		// Compute the cube root of the dot product and store the result in the lower triangular part of the matrix
		M[row_t + m] = result; // M[m+k][m]
		// Copy the result to the upper triangular part of the matrix
		M[row + (m + k)] = result; // M[m][m+k] = M[m+k][m]
		}
	}
	
	// clear intermedaite results in the lower triangular part of the matrix
    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = 0; j < i; ++j) {
            M[i * N + j] = 0.0;
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

	#ifdef BENCHMARK
		auto a = std::chrono::system_clock::now();
		wavefront(M, N);
		auto b = std::chrono::system_clock::now();
		std::chrono::duration<double> delta = b-a;
		std::cout << std::fixed << std::setprecision(6) << delta.count() << std::endl;
	#else
		wavefront(M, N); 
		std::cout << M[N - 1] << std::endl;
	#endif

    return 0;
}
