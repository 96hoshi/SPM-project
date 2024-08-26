#include <iostream>
#include <cstdint>
#include <vector>
#include <cmath>
#include <mpi.h>


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
    // TODO: add check if N is too big for a certain treshold, save the matrix in a file and read it in the root process

    double start_time = 0;
    int mat_size = N * N;
    std::vector<double> M(N * N, 0.0);

    // Initialize matrix in the root process
    if (rank == 0) {
        for (int i = 0; i < N; ++i) {
            M[i * N + i] = static_cast<double>(i + 1) / N;
        }
       start_time = MPI_Wtime();
    }

    // TODO: use scatterv
    // TODO: do not send them matrix but only send the corect partial diagonal to each processor
    // Broadcast matrix to all processes
    MPI_Bcast(M.data(), mat_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Process each diagonal element
    for (int k = 1; k < N; ++k) {
        int rows_per_process = (N - k) / size;
        int remaining_rows = (N - k) % size;

        int start_row = rank * rows_per_process + std::min(rank, remaining_rows);
        int end_row = start_row + rows_per_process + (rank < remaining_rows ? 1 : 0);

        std::vector<double> local_results(end_row - start_row, 0.0);

        for (int m = start_row; m < end_row; ++m) {
            int row_start = m * N;        //index to iterate over the rows
            int diag_row = (m + k) * N;   //row of the diagonal element

            local_results[m - start_row] = 0.0;

            for (int j = 0; j < k; ++j) {
                // read the elements from the lower triangular part of the matrix
                local_results[m - start_row] += M[row_start + (m + j)] * M[diag_row + (m + j + 1)]; // M[m][m+j] * M[m+k][m+j+1]
            }
            // Compute the cube root of the dot product and store the result in the lower triangular part of the matrix
            local_results[m - start_row] = std::cbrt(local_results[m - start_row]); // M[m+k][m]
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
            for (int m = 0; m < N - k; ++m) {
                int row_start = m * N;  
                int diag_row = (m + k) * N;

                // Store the result in the lower triangular part of the matrix
                M[diag_row + m] = gathered_results[m]; // M[m+k][m]
                // Copy the result to the upper triangular part of the matrix
                M[row_start + (m + k)] = M[diag_row + m]; // M[m][m+k] = M[m+k][m]
            }
        } else {
            // Send the results to the root process
            MPI_Gatherv(local_results.data(), local_results.size(), MPI_DOUBLE, nullptr, nullptr, nullptr, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        }

        // Broadcast updated matrix for next iteration
        // TODO: use scatterv to send only the correct part of the matrix instead of the whole matrix
        MPI_Bcast(M.data(), mat_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }
    
    // Print the final matrix
    if (rank == 0) {    
        // stop timer
        double end_time = MPI_Wtime();

        //#ifdef BENCHMARK
        std::printf("%f\n", end_time - start_time);
        //# else
        // Clear the lower triangular matrix
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < i; ++j) {
                M[i * N + j] = 0.0;
            }
        }
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                std::printf("%f ", M[i * N + j]);
            }
            std::printf("\n");
        }
        //#endif
    }

    MPI_Finalize();
    return 0;
}