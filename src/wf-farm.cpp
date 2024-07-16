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
    int task_size;
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
struct Emitter: ff_monode_t<Task, Task> {
    uint64_t N;         // Number of elements in the matrix (NxN)
    uint64_t k;         // Current diagonal index
    int nw;             // Number of workers
    int chunk_size;     // Number of elements in the diagonal for each worker
    int feedback_count; // Counter to track feedback from workers
    bool diag_done;

    Emitter(uint64_t N, int nw) : N(N), nw(nw), k(1), feedback_count(0), diag_done(false) {
        // Same chunk size to all workers for static scheduling
        chunk_size = static_cast<int>(N / nw);
    }

    Task* svc(Task* task) {
        // first input
        printf("Emitter: received task\n");
        if (task == nullptr || diag_done) { // Send tasks to workers
            diag_done = false;
            for (uint64_t i = 0; i < (N - k); i += chunk_size) {
                int chunk = std::min(chunk_size, static_cast<int>(N - k - i));
                ff_send_out(new Task{k, i, chunk});
                printf("Emitter: sent task (%ld, %ld, %d)\n", k, i, chunk);
                }
            printf("Emitter: sent all tasks for diagonal %ld\n", k);
        } else {
            // check feedback from workers
            if (feedback_count < N - k) {
                // Increment feedback count
                feedback_count += task->task_size;
                printf("FB: received feedback %d\n", feedback_count);
                delete task;
            }
            if (feedback_count == N - k) {
                // All feedback received
                printf("FB: received all feedback\n");
                feedback_count = 0;
                ++k;
                diag_done = true;
                // Continue processing, if not reached the end
                return GO_ON;
            }
        } if (k == N) {
            printf("Emitter: sent EOS\n");
            return EOS;
        }

        return GO_ON;
    }

    void svc_end() {
        // Do nothing
        printf("Emitter: END\n");
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
        const int task_size = task->task_size;   // Number of elements in the diagonal

        // Perform the computation for the matrix diagonal element
        std::vector<double> v_m(k), v_mk(k);
        printf("Worker: received task (%ld, %ld, %d)\n", k, m, task_size);
        
        int end = std::min(N - k, m + task_size);

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
        ff_send_out(task);
        delete task;

        return GO_ON;
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
    ff_Farm<std::pair<int, int>> farm(move(workers));
    Emitter emitter(N, nw);
    farm.add_emitter(emitter);
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
        farm_wavefront(M, N, nw) < 0;
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

    // TIMERSTART(faram_wavefront);
    // farm_wavefront(M, N, nw);
    // TIMERSTOP(faram_wavefront);

    return 0;
}
