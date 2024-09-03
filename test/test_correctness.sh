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
declare -a case1_sizes=(50 100 500)
declare -a case2_sizes=(128 516 1024)
declare -a case3_sizes=(2048 4096 8192)

# Define path to store results
path_results="../test/results/correctness/"

# Compilation
if [ -d "../build" ]; then
    rm -rf ../build
fi
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=OFF .. 
make -j 8 

# Function to run the programs for a given matrix size
run_programs() {
    local size=$1

    echo "Matrix size: $size"
    echo "#workers: $num_workers"
    # Run the sequential version and get the output save it with the size with proper name
    ./sequential_wf $size > $path_results/seq_output_$size.txt
    # Run the FastFlow version with specified number of threads
    ./ff_parallel_wf $size $num_workers > $path_results/parallel_output_$size.txt
    # Run the FastFlow farm version with specified number of threads
    ./ff_farm_wf $size $num_workers 0 > $path_results/farm_output_$size.txt
    # Run the MPI version with specified number of processes
    mpirun -np 4 ./mpi_wf $size > $path_results/mpi_output_$size.txt

    # Compare the output matrices
    echo "Size $size"
    compare_matrices $path_results/seq_output_$size.txt $path_results/farm_output_$size.txt "FastFlow"
    compare_matrices $path_results/seq_output_$size.txt $path_results/mpi_output_$size.txt "MPI"
    echo "------------------------------------"
}

# compare the element of each matrix the output file contains only one number that must to match
compare_matrices() {
    local file1=$1
    local file2=$2
    local name=$3

    # Compare the output matrices
    diff $file1 $file2 > /dev/null
    if [ $? -eq 0 ]; then
        echo "$name: Correct"
    else
        echo "$name: Incorrect"
        echo "Sequential output:"
        cat $file1
        echo "Parallel output:"
        cat $file2
    fi
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

