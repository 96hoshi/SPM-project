#!/bin/bash

# Check if arguments are provided correctly
if [ $# -lt 2 ]; then
    echo "Usage: $0 <test_case_number> <num_workers>"
    exit 1
fi

# Extract inputs
test_case=$1
num_workers=$2

# Define matrix sizes for each test case 
declare -a case1_sizes=(5 10 50)
declare -a case2_sizes=(128 516 1024)
declare -a case3_sizes=(2000 5000 10000)

# Compilation
cd ../build/
cmake -DENABLE_BENCHMARK=OFF .. >> /dev/null
make -j 8  >> /dev/null

# Function to run the programs for a given matrix size
run_programs() {
    local size=$1

    echo "Matrix size: $size"
    echo "num_workers: $num_workers"
    # Run the sequential version and get the output
    ../build/sequential_wf $size > seq_output.txt

    # Run the FastFlow parallel version with specified number of threads
    ../build/ff_parallel_wf $size $num_workers > par_output.txt

    # Run the FastFlow farm version with specified number of threads
    ../build/ff_farm_wf $size $num_workers > farm_output.txt

    # Run the MPI version with specified number of processes
    mpirun -np $num_workers ../build/mpi_wf $size > mpi_output.txt

    # Compare the output matrices
    echo "Size $size with tolerance 10^-6..."
    compare_matrices seq_output.txt par_output.txt "Parallel"
    compare_matrices seq_output.txt farm_output.txt "Farm"
    compare_matrices seq_output.txt mpi_output.txt "MPI"
    echo "------------------------------------"
}

# Function to compare the last element of matrices with tolerance
compare_matrices() {
    local file1=$1
    local file2=$2
    local label=$3

    # Use Python for comparison of only the last element
    python3 - <<EOF
import sys
import numpy as np

def compare_last_element(file1, file2, tolerance):
    # Load matrices from files
    matrix1 = np.loadtxt(file1)
    matrix2 = np.loadtxt(file2)

    # Get the last element from both matrices
    last_element_1 = matrix1[0, -1]
    last_element_2 = matrix2[0, -1]

    # Compare the last elements within the specified tolerance
    if np.isclose(last_element_1, last_element_2, atol=tolerance):
        print("$label: MATCHES.")
    else:
        print("$label: DOES NOT MATCH.")
        print("Sequential last element: ", last_element_1)
        print("$label last element: ", last_element_2)

compare_last_element("$file1", "$file2", 1e-6)
EOF
}

# Determine which test case to run
case $test_case in
    1)
        echo "Running test case 1"
        sizes=("${case1_sizes[@]}")
        ;;
    2)
        echo "Running test case 2"
        sizes=("${case2_sizes[@]}")
        ;;
    3)
        echo "Running test case 3"
        sizes=("${case3_sizes[@]}")
        ;;
    *)
        echo "Invalid test case number."
        exit 1
        ;;
esac

# Loop through sizes for the selected test case
for size in "${sizes[@]}"; do
    run_programs $size
done

# Clean up temporary output files if needed
rm -f seq_output.txt farm_output.txt mpi_output.txt
