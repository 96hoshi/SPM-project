#include <iostream>
#include <vector>
#include <cmath>
#include <mpi.h>
#include <cstdint>

#define MAX_CHUNK_SIZE 64

// Print the matrix
void printMatrix(const std::vector<double>& M, const uint64_t& N) {
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            std::printf("%f ", M[i * N + j]);
        }
        std::printf("\n");
    }
}

// Worker function to perform tasks
void workerLoop(int64_t N) {
    MPI_Status status;
    uint64_t k, m;
    uint task_size;
    int64_t size = N * N;
    std::vector<double> M(size);

    while (true) {
        std::printf("Worker\n");
        MPI_Recv(M.data(), size, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&k, 1, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &status);
        if (k == N) break; // Exit signal

        MPI_Recv(&m, 1, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&task_size, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &status);

        uint64_t mk, i;
        double result;
        uint64_t end = std::min(N - k, m + task_size);

        for (; m < end; ++m) {
            mk = m + k;
            i = m * N;
            result = 0.0;
            for (uint64_t j = 0; j < k; ++j) {
                result += M[i + (m + j)] * M[mk * N + (mk - j)];
            }
            M[mk * N + m] = std::cbrt(result);
            M[i + mk] = M[mk * N + m];
        }

        uint64_t completed = end - m;
        MPI_Send(&completed, 1, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD);
    }
}

// Emitter function to distribute tasks
void emitterLoop(std::vector<double>& M ,uint64_t N, int size) {
    uint64_t k = 1;
    int chunk_size = static_cast<uint64_t>(std::ceil(static_cast<double>(N) / (size - 1)));
    chunk_size = std::min(chunk_size, MAX_CHUNK_SIZE);
    std::printf("Emitter\n");


    while (k < N) {
        std::printf("k = %lu\n", k);
        for (uint64_t i = 0; i < (N - k); i += chunk_size) {
            uint chunk = std::min(chunk_size, static_cast<int>(N - k - i));
            for (int rank = 1; rank < size; ++rank) {
                // Send the matrix
                // TODO: use the correct type for the matrix ??????
                // do the workers need all the matrix or?
                MPI_Send(&M[i * N], N * chunk, MPI_DOUBLE, rank, 0, MPI_COMM_WORLD);
                MPI_Send(&k, 1, MPI_UINT64_T, rank, 0, MPI_COMM_WORLD);
                MPI_Send(&i, 1, MPI_UINT64_T, rank, 0, MPI_COMM_WORLD);
                MPI_Send(&chunk, 1, MPI_UNSIGNED, rank, 0, MPI_COMM_WORLD);
            }
        }

        uint64_t feedback_count = 0;
        while (feedback_count < (N - k)) {
            uint64_t completed;
            MPI_Status status;
            MPI_Recv(&completed, 1, MPI_UINT64_T, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            feedback_count += completed;
        }

        ++k;
    }

    for (int rank = 1; rank < size; ++rank) {
        MPI_Send(&N, 1, MPI_UINT64_T, rank, 0, MPI_COMM_WORLD); // Exit signal
    }
}

int main(int argc, char *argv[]) {
    uint64_t N = 516; // default size of the matrix (NxN)
    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc == 2) {
        N = std::stoul(argv[1]);
    }

    if (rank == 0) {
        if (N < 1) {
            std::printf("N should be greater than 0\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // Allocate the matrix
        std::vector<double> M(N * N);
        for (uint64_t i = 0; i < N; ++i) {
            M[i * N + i] = static_cast<double>(i + 1) / N;
        }

        emitterLoop(M, N, size);

        printMatrix(M, N);
    } else {
        workerLoop(N);
    }

    MPI_Finalize();
    return 0;
}
