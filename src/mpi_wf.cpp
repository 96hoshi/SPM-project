#include <iostream>
#include <vector>
#include <cmath>
#include <mpi.h>
#include <cstdint>

// Function to compute dot product of two arrays
double compute_dot_product(const std::vector<double> arr1, const std::vector<double> arr2, int size) {
    double dot_product = 0.0;
    for (int i = 0; i < size; ++i) {
        dot_product += arr1[i] * arr2[i];
    }
    return dot_product;
}

// Function to compute cubic root
double compute_cubic_root(double value) {
    return cbrt(value);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N = 5;

    if (argc < 2) {
        if (rank == 0) {
            std::printf("use: %s N\n", argv[0]);
            std::printf("     N size of the square matrix\n");
        }
        MPI_Finalize();
        return -1;
    } else {
        N = std::stol(argv[1]);
    }

    // Check the size of the matrix
    if (N < 1) {
        if (rank == 0) {
            std::printf("N should be greater than 0\n");
        }
        MPI_Finalize();
        return -1;
    }

    double start_time = 0;
    int mat_size = N * N;
    std::vector<double> matrix(N * N, 0.0);

    // Initialize matrix in the root process
    if (rank == 0) {
        for (int i = 0; i < N; ++i) {
            matrix[i * N + i] = static_cast<double>(i + 1) / N;
        }
       start_time = MPI_Wtime();
    }

    // TODO: use scatterv
    // Broadcast matrix to all processes
    MPI_Bcast(matrix.data(), mat_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Process each diagonal element
    for (int k = 1; k < N; ++k) {
        int rows_per_process = (N - k) / size;
        int remaining_rows = (N - k) % size;

        int start_row = rank * rows_per_process + std::min(rank, remaining_rows);
        int end_row = start_row + rows_per_process + (rank < remaining_rows ? 1 : 0);

        std::vector<double> local_results(end_row - start_row, 0.0);

        // TODO: use Reudce
        for (int i = start_row; i < end_row; ++i) {
            // read colums per each process
            for (int j = 0; j < k; ++j) {
                local_results[i - start_row] += matrix[i * N + i + j ] * matrix[(i + k - j) * N + i + k];
            }
            local_results[i - start_row] = compute_cubic_root(local_results[i - start_row]);
         }

        // Gather results at the root process
        if (rank == 0) {
            std::vector<int> recv_counts(size);
            std::vector<int> displs(size);

            for (int i = 0; i < size; ++i) {
                // Compute the number of elements to receive from each process
                recv_counts[i] = (N - k) / size + (i < remaining_rows ? 1 : 0);
                // Compute the displacement for each process 
                displs[i] = i * rows_per_process + std::min(i, remaining_rows);
            }

            std::vector<double> gathered_results(N - k, 0.0);
            // Gather the results from all processes
            MPI_Gatherv(local_results.data(), local_results.size(), MPI_DOUBLE, gathered_results.data(), recv_counts.data(), displs.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);

            // Update the matrix with the gathered results
            for (int i = 0; i < N - k; ++i) {
                matrix[i * N + (i + k)] = gathered_results[i];
            }
        } else {
            // Send the results to the root process
            MPI_Gatherv(local_results.data(), local_results.size(), MPI_DOUBLE, nullptr, nullptr, nullptr, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        }
        // Broadcast updated matrix for next iteration
        MPI_Bcast(matrix.data(), mat_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }
    
    // Print the final matrix
    if (rank == 0) {    
        // stop timer
        double end_time = MPI_Wtime();

        #ifdef BENCHMARK
        std::printf("%f\n", end_time - start_time);
        # else 
            //std::printf("Final matrix:\n");
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    std::printf("%f ", matrix[i * N + j]);
                }
                std::printf("\n");
            }
        #endif
    }

    MPI_Finalize();
    return 0;
}