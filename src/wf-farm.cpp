#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <cmath>
#include <ff/ff.hpp>
#include <ff/farm.hpp>

#include "hpc_helpers.hpp"

///#include "utils.hpp"

using namespace ff;

//TODO: use proper type
struct Task {
    uint64_t k;
    uint64_t m;
    uint64_t task_size;
};


// Print the matrix
void printMatrix(const std::vector<double>& M, const uint64_t& N) {
    for(size_t i = 0; i < N; ++i) {
        for(size_t j = 0; j < N; ++j) {
            std::printf("%f ", M[i * N + j]);
        }
        std::printf("\n");
    }
}

// Emitter class to distribute tasks
struct Emitter: ff_monode_t<bool, Task> {
    uint64_t N;         // Number of elements in the matrix (NxN)
    uint64_t k;         // Current diagonal index
    int nw;             // Number of workers
    int chunk_size;     // Number of elements in the diagonal for each worker

    Emitter(uint64_t N, int nw) : N(N), nw(nw), k(1){
        // Same chunk size to all workers for static scheduling
        chunk_size = static_cast<int>(N / nw);
    }

    Task* svc(bool* f) {

        for (uint64_t i = 0; i < (N - k); i += chunk_size) {
            uint64_t chunk = std::min(chunk_size, static_cast<int>(N - k - i));
            ff_send_out(new Task{k, i, chunk});
        }
        // Increase the diagonal index
        ++k;
        
        if (k == N) {
            return EOS;
        }
        return GO_ON;
    }
};

struct Collector : ff_minode_t<Task, bool> {
    uint64_t N;                 // Number of elements in the matrix (NxN)
    uint64_t k;                 // Current diagonal index
    uint64_t feedback_count;    // Number of feedback received

    Collector(uint64_t N) : N(N), feedback_count(0), k(1) {}

    bool* svc(Task* task) {
        if (task == nullptr) {
            return GO_ON;
        }
        // Receive feedback from workers
        feedback_count += task->task_size;

        if (feedback_count == N - k) {
            feedback_count = 0;
            ++k;
            // Send signal to the emitter
            bool* done = new bool(true);
            ff_send_out(done);
        }
        delete task;
        return GO_ON;
    }
};

// Worker class to perform tasks
struct Worker: ff_node_t<Task, Task> {
    std::vector<double>& M;
    uint64_t N;

    Worker(std::vector<double>& M, uint64_t N) : M(M), N(N) {}

    Task* svc(Task* task) {
        uint64_t m = task->m;                    // Row index
        const uint64_t k = task->k;              // Diagonal index
        const uint64_t task_size = task->task_size;   // Number of elements in the diagonal

        // Perform the computation for the matrix diagonal element
        std::vector<double> v_m(k), v_mk(k);        
        uint64_t end = std::min(N - k, m + task_size);

        // Compute the diagonal for the given task length
        for (; m < end; ++m) {
            auto mk = m + k;
            auto i = m * N;

            for (uint64_t j = 0; j < k; ++j) {
                v_m[j] = M[i + (m + j)];        // M[m][m+j]
                v_mk[j] = M[(mk - j) * N + mk]; // M[m+k-j][m+k]
            }
            // Compute the cube root of the dot product
            // TODO: compare function call with inline code
            M[i + mk] = dotProduct(v_m, v_mk); // M[m][m+k]
        }

        // Send feedback to the emitter to indicate completion
        return task;
    }

    // Function to perform cube root of dot product
    inline double dotProduct(const std::vector<double>& v1, const std::vector<double>& v2) {
        double result = 0.0;
        for(size_t i = 0; i < v1.size(); ++i) {
            result += v1[i] * v2[i];
        }
        return std::cbrt(result);
    }
};

// Function to perform wavefront
int farm_wavefront(std::vector<double>& M, const uint64_t& N, const int nw) {
    std::vector<std::unique_ptr<ff_node>> workers;
    
    // Create workers
    for(int i = 0; i < nw; ++i) {
        workers.push_back(make_unique<Worker>(M, N));
    }

    // Create farm
    ff_Farm<std::pair<int, int>> farm(std::move(workers));
    Emitter emitter(N, nw);
    Collector collector(N);
    farm.add_emitter(emitter);
    farm.add_collector(collector);
    farm.wrap_around();
    // farm.set_scheduling_ondemand(); 

    if (farm.run_and_wait_end() < 0) {
        error("running farm");
        return -1;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    uint64_t N = 516;    // default size of the matrix (NxN)
    uint64_t nw = 4;     // default number of workers

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

    // TODO: use utils macro
    #ifdef BENCHMARK
        auto a = std::chrono::system_clock::now();
        farm_wavefront(M, N, nw);
        auto b = std::chrono::system_clock::now();
        std::chrono::duration<double> delta = b-a;
        std::cout << std::fixed << std::setprecision(6) << delta.count() << std::endl;
    #else
        if(farm_wavefront(M, N, nw) < 0) {
            error("running farm_wavefront");
            return -1;
        }
        printMatrix(M, N);
    #endif

    return 0;
}
