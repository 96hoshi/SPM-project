#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

#include "hpc_helpers.hpp"
///#include "utils.hpp"

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
void printMatrix(const std::vector<std::vector<double>>& M) {
    for(size_t i = 0; i < M.size(); ++i) {
        for(size_t j = 0; j < M.size(); ++j) {
            std::printf("%f ", M[i][j]);
        }
        std::printf("\n");
    }
}

// Worker class to process elements of a diagonal
struct Worker: ff_node_t<std::pair<int, int>> {
    std::vector<std::vector<double>>& M;
    Worker(std::vector<std::vector<double>>& M): M(M) { }

    std::pair<int, int>* svc(std::pair<int, int>* task) {
        int m = task->first;
        int k = task->second;

        std::vector<double> v_m(k), v_mk(k);
        for(int j = 0; j < k; ++j) {
            v_m[j] = M[m][m + j];
            v_mk[j] = M[m + k - j][m + k];
        }
        M[m][m + k] = dotProduct(v_m, v_mk);

        return task;
    }
};

// Emitter class to distribute tasks
struct Emitter: ff_monode_t<std::pair<int, int>> {
    int N;
    int k;

    Emitter(int N): N(N), k(1) {}

    std::pair<int, int>* svc(std::pair<int, int>*) {
        if (k >= N) return EOS; // End of stream
        for (int m = 0; m < N - k; ++m) {
            ff_send_out(new std::pair<int, int>(m, k));
        }
        k++;
        return GO_ON;
    }
};

// Function to perform wavefront
void farm_wavefront(std::vector<std::vector<double>>& M, const uint64_t& N, const int nw) {
    std::vector<std::unique_ptr<ff_node>> workers;
    
    // Create workers
    for(int i = 0; i < nw; ++i) {
        workers.push_back(make_unique<Worker>(M));
    }

    // Create farm
    ff_Farm<std::pair<int, int>> farm(move(workers));
    Emitter emitter(N);
    farm.add_emitter(emitter);

    if (farm.run_and_wait_end() < 0) {
        error("running farm");
    }
}

int main(int argc, char *argv[]) {
    uint64_t N = 10;    // default size of the matrix (NxN)
    int nw = 4;         // default number of workers

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

    // Allocate the matrix 
    std::vector<std::vector<double>> M(N, std::vector<double>(N, 0.0));
    for(uint64_t i = 0; i < N; ++i) {
        M[i][i] = static_cast<double>(i + 1) / N;
    }

    // TODO: use utils macro
    #ifdef BENCHMARK
        auto a = std::chrono::system_clock::now();
        farm_wavefront(M, N, nw);
        auto b = std::chrono::system_clock::now();
        std::chrono::duration<double> delta = b-a;
        std::cout << std::fixed << std::setprecision(6) << delta.count() << std::endl;
    #else
        farm_wavefront(M, N, nw);
        printMatrix(M);
    #endif

    // TIMERSTART(faram_wavefront);
    // farm_wavefront(M, N, nw);
    // TIMERSTOP(faram_wavefront);

    // // Print the matrix
    // printMatrix(M);

    return 0;
}
