#include <iostream>
#include <cstdint>
#include <vector>
#include <cmath>
#include <iomanip>
#include <mpi.h>


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N = 1024;

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
    // TODO: add check if N is too big for a certain treshold, save the matrix in a file and read it in the root process FUTURE WORKS

    double start_time = 0;
    int mat_size = N * N;
    std::vector<double> M(mat_size, 0.0);
    std::vector<double> main_diag(N, 0.0);

    // Initialize matrix in the root process
    if (rank == 0) {
        for (int i = 0; i < N; ++i) {
            //M[i * N + i] = static_cast<double>(i + 1) / N;
            main_diag[i] = static_cast<double>(i + 1) / N;
        }
       start_time = MPI_Wtime();
    }

    // TODO: use scatterv
    // TODO: do not send the matrix but only send the corect partial diagonal to each process
    // Broadcast matrix to all processes
    MPI_Bcast(main_diag.data(), N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // use the broadcasted diagonal to update the matrix
    for (int i = 0; i < N; ++i) {
        M[i * N + i] = main_diag[i];
    }

    // Process each diagonal element
    for (int k = 1; k < N; ++k) {
        int rows_per_process = (N - k) / size;
        int remaining_rows = (N - k) % size;

        int start_row = rank * rows_per_process + std::min(rank, remaining_rows);
        int end_row = start_row + rows_per_process + (rank < remaining_rows ? 1 : 0);

        std::vector<double> local_results(end_row - start_row, 0.0);

        // TODO: use omp parallel for with reduce (+)
        for (int m = start_row; m < end_row; ++m) {
            int row_start = m * N;        //index to iterate over the rows
            int diag_row = (m + k) * N;   //row of the diagonal element

            local_results[m - start_row] = 0.0;

            for (int j = 0; j < k; ++j) {
                local_results[m - start_row] += M[row_start + (m + j)] * M[diag_row + (m + j + 1)]; // M[m][m+j] * M[m+k][m+j+1]
            }
            local_results[m - start_row] = std::cbrt(local_results[m - start_row]); // M[m+k][m]
        }

        std::vector<double> diag_temp(N - k, 0.0);

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
            diag_temp = gathered_results;
        } else {
            // Send the results to the root process
            MPI_Gatherv(local_results.data(), local_results.size(), MPI_DOUBLE, nullptr, nullptr, nullptr, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        }

        // Broadcast updated the diagonal to all processes
        MPI_Bcast(diag_temp.data(), N - k, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        for (int m = 0; m < N - k; ++m) {
            int row_start = m * N;  
            int diag_row = (m + k) * N;

            // Store the result in the lower triangular part of the matrix
            M[diag_row + m] = diag_temp[m]; // M[m+k][m]
            // Copy the result to the upper triangular part of the matrix
            M[row_start + (m + k)] = M[diag_row + m]; // M[m][m+k] = M[m+k][m]
        }
    }
    
    // Print the final matrix
    if (rank == 0) {    
        // stop timer
        double end_time = MPI_Wtime();

        #ifdef BENCHMARK
            std::printf("%f\n", end_time - start_time);
            std::cout << M[N - 1] << std::endl;
        #else
            // Clear the lower triangular matrix
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < i; ++j) {
                    M[i * N + j] = 0.0;
                }
            }
            // for (int i = 0; i < N; ++i) {
            //     for (int j = 0; j < N; ++j) {
            //         std::printf("%f ", M[i * N + j]);
            //     }
            //     std::printf("\n");
            // }

            // correctness check: if the upper right element of the matrix is correct
            std::cout << M[N - 1] << std::endl;
        #endif
    }

    MPI_Finalize();
    return 0;
}