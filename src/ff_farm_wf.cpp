#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <ff/ff.hpp>
#include <ff/farm.hpp>

#include "hpc_helpers.hpp"
//#include "utils.hpp"

using namespace ff;

#define MAX_CHUNK_SIZE 128


// Print the matrix
void printMatrix(const std::vector<double>& M, const uint64_t& N) {
    for(size_t i = 0; i < N; ++i) {
        for(size_t j = 0; j < N; ++j) {
            std::printf("%f ", M[i * N + j]);
        }
        std::printf("\n");
    }
}

//TODO: use proper type
struct Task {
    uint64_t k;
    uint64_t m;
    uint task_size;
};

// Emitter class to distribute tasks
struct Emitter: ff_monode_t<uint, Task> {
    uint64_t N;         // Number of elements in the matrix (NxN)
    uint64_t k;         // Current diagonal index
    uint nw;            // Number of workers
    int chunk_size;     // Number of elements in the diagonal for each worker
    uint feedback_count;// Number of feedback received


    Emitter(uint64_t N, int nw) : N(N), nw(nw), k(1), feedback_count(0) {
        // Same chunk size to all workers for static scheduling
        //chunk_size = static_cast<uint64_t>(std::ceil(static_cast<double>(N) / nw));
        chunk_size = 1;
    }

    Task* svc(uint* task_size) {
        if (task_size != nullptr && *task_size) {
            // Receive feedback from workers
            feedback_count += *task_size;
            delete task_size;
        } 
        if (feedback_count == N - k) {
            feedback_count = 0;
            ++k;
        }
        if(feedback_count == 0){
            // Send tasks to workers
            for (uint64_t m = 0; m < (N - k); m += chunk_size) {
                uint chunk = std::min(chunk_size, static_cast<int>(N - k - m));
                ff_send_out(new Task{k, m, chunk});
            }
        }
        if (k == N) {
            return EOS;
        }
        return GO_ON;
    }
};

struct Worker: ff_node_t<Task, uint> {
    std::vector<double>& M;
    uint64_t N;
    uint64_t row_start = 0;           // Row index
    uint64_t diag_row = 0;            // Row index for the diagonal
    double result = 0.0;

    Worker(std::vector<double>& M, uint64_t N) : M(M), N(N) {}

    uint* svc(Task* task) {
        uint64_t m = task->m;                   // Row index
        const uint64_t k = task->k;             // Diagonal index
        const uint task_size = task->task_size; // Number of elements in the diagonal

        uint64_t end = std::min(N - k, m + task_size);

        for (; m < end; ++m) {
            row_start = m * N;       // index to iterate over the rows
            diag_row = (m + k) * N;  // row of the diagonal element

            result = 0.0;
            for (uint64_t j = 0; j < k; ++j) {
                // read the elements from the lower triangular part of the matrix
                result += M[row_start + (m + j)] * M[diag_row + (m + j + 1)]; // M[m][m+j] * M[m+k][m+k-j]
            }
            // Compute the cube root of the dot product and store the result in the lower triangular part of the matrix
            M[diag_row + m] = std::cbrt(result); // M[m+k][m]
            // Copy the result to the upper triangular part of the matrix
            M[row_start + (m + k)] = M[diag_row + m]; // M[m][m+k] = M[m+k][m]
        }
        uint* size = new uint(task_size);
        return size;
    }
};

// Function to perform wavefront
int farm_wavefront(std::vector<double>& M, const uint64_t& N, const int nw, const int on_demand) {
    std::vector<std::unique_ptr<ff_node>> workers;
    
    // Create workers
    for(int i = 0; i < nw; ++i) {
        workers.push_back(make_unique<Worker>(M, N));
    }

    ff_Farm<std::pair<int, int>> farm(std::move(workers));
    Emitter emitter(N, nw);

    farm.add_emitter(emitter);
    farm.remove_collector();
    farm.wrap_around();
    if(on_demand == 1) {
        farm.set_scheduling_ondemand();
    }

    if (farm.run_and_wait_end() < 0) {
        error("running farm");
        return -1;
    }
    // clear intermedaite results in the lower triangular part of the matrix
    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = 0; j < i; ++j) {
            M[i * N + j] = 0.0;
        }
    }

    return 0;
}


int main(int argc, char *argv[]) {
    uint64_t N = 516;    // default size of the matrix (NxN)
    uint64_t nw = 4;     // default number of workers
    int on_demand = 0;   // on-demand scheduling

    if (argc != 2 && argc != 3 && argc != 4) {
		std::printf("use: %s N nw\n", argv[0]);
		std::printf("     N size of the square matrix\n");
        std::printf("     nw number of workers\n");
        std::printf("     on-nemand 0 for not enabled, 1 for enalbed\n");
		return -1;
	}
	if (argc > 2) {
		N = std::stol(argv[1]);
        nw = std::stoi(argv[2]);
        on_demand = std::stoi(argv[3]);
	}

    // Check the size of the matrix
    if (N < 1) {
        std::printf("N should be greater than 0\n");
        return -1;
    }
    // Check the number of workers
    if (nw < 1) {
        std::printf("number of workers should be greater than 0\n");
        return -1;
    }

    // Allocate the matrix 
    std::vector<double> M(N * N, 0.0);
    for(uint64_t i = 0; i < N; ++i) {
        M[i * N + i ] = static_cast<double>(i + 1) / N;
    }

    #ifdef BENCHMARK
        auto a = std::chrono::system_clock::now();
        farm_wavefront(M, N, nw, on_demand);
        auto b = std::chrono::system_clock::now();
        std::chrono::duration<double> delta = b-a;
        std::cout << std::fixed << std::setprecision(6) << delta.count() << std::endl;
    #else
        if(farm_wavefront(M, N, nw, on_demand) < 0) {
            error("running farm_wavefront");
            return -1;
        }
        //printMatrix(M, N);
        std::cout << M[N - 1] << std::endl;
    #endif

    return 0;
}
