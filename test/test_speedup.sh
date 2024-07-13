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
declare -a case1_sizes=(10 50 100)
declare -a case2_sizes=(200 500 1000)
declare -a case3_sizes=(2000 5000 10000)

# Compilation
#mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8 

# Function to run the programs for a given matrix size
run_programs() {
    local size=$1

    echo "num_threads: $num_threads"
    # Run the sequential version and get the time
    SEQ_TIME=$(../build/wf_sequential $size)
    echo "Sequential:  $SEQ_TIME seconds"

    # Run the FastFlow parallel version with specified number of threads
    PAR_TIME=$(../build/wf_parallel $size $num_threads)
    echo "FF Parallel: $PAR_TIME seconds"

    # Run the FastFlow farm version with specified number of threads
    FARM_TIME=$(../build/wf_farm $size $num_threads)
    echo "FF Farm:     $FARM_TIME seconds"

    # Calculate the speedup
    PAR_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $PAR_TIME" | bc)
    FARM_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $FARM_TIME" | bc)

    echo "Speedup Parallel: $PAR_SPEEDUP"
    echo "Speedup Farm:     $FARM_SPEEDUP"
    echo
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