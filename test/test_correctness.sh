#!/bin/bash

# Check if arguments are provided correctly
if [ $# -lt 2 ]; then
    echo "Usage: $0 <test_case_number> <num_threads>"
    exit 1
fi

# Extract inputs
test_case=$1
num_threads=$2

# Define matrix sizes for each test case 
declare -a case1_sizes=(5 10 50)
declare -a case2_sizes=(200 500 1000)
declare -a case3_sizes=(2000 5000 10000)

# Compilation
cd ../build/
cmake -DENABLE_BENCHMARK=OFF .. 
make -j 8 

# Function to run the programs for a given matrix size
run_programs() {
    local size=$1

    echo "num_threads: $num_threads"
    # Run the sequential version and get the output
    ../build/wf_sequential $size > seq_output.txt

    # Run the FastFlow parallel version with specified number of threads
    ../build/wf_parallel $size $num_threads > par_output.txt

    # Run the FastFlow farm version with specified number of threads
    ../build/wf_farm $size $num_threads > farm_output.txt

    # Compare the output matrices
    echo "Comparing matrices for size $size with tolerance 10^-4..."
    compare_matrices seq_output.txt par_output.txt "Parallel"
    compare_matrices seq_output.txt farm_output.txt "Farm"
}

# Function to compare matrices with tolerance
compare_matrices() {
    local file1=$1
    local file2=$2
    local label=$3

    # Use Python for more accurate floating-point comparison
    python3 - <<EOF
import sys
import numpy as np

def compare_matrices(file1, file2, tolerance):
    # Load matrices from files
    matrix1 = np.loadtxt(file1)
    matrix2 = np.loadtxt(file2)

    # Compare matrices element-wise within the specified tolerance
    if np.allclose(matrix1, matrix2, atol=tolerance):
        print("$label: MATCHES.")
    else:
        print("$label: DOES NOT MATCH.")
        print("Matrix 1:")
        print(matrix1)
        print("Matrix 2:")
        print(matrix2)

compare_matrices("$file1", "$file2", 1e-4)
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
    # Add more cases as needed
    *)
        echo "Invalid test case number."
        exit 1
        ;;
esac

# Loop through sizes for the selected test case
for size in "${sizes[@]}"; do
    echo "Matrix size: $size"
    run_programs $size
    echo "----------------------------------------"
done

# Clean up temporary output files if needed
rm -f seq_output.txt par_output.txt farm_output.txt
