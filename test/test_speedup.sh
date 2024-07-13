#!/bin/bash

# Check if matrix sizes are provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <size1> [<size2> ...]"
    exit 1
fi

# Compilation
#mkdir ../build 
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8 

# Function to run the programs for a given matrix size
run_programs() {
    local size=$1

    # Run the sequential version and get the time
    SEQ_TIME=$(../build/wf_sequential $size)
    echo "Sequential:  $SEQ_TIME seconds"

    # Run the FastFlow parallel version and get the time
    PAR_TIME=$(../build/wf_parallel $size)
    echo "FF Parallel: $PAR_TIME seconds"

    # Run the FastFlow farm version and get the time
    FARM_TIME=$(../build/wf_farm $size 4)
    echo "FF Farm:     $FARM_TIME seconds"

    # Calculate the speedup
    PAR_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $PAR_TIME" | bc)
    FARM_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $FARM_TIME" | bc)

    echo "Speedup Parallel: $PAR_SPEEDUP"
    echo "Speedup Farm:     $FARM_SPEEDUP"
    echo
}

# Loop through each matrix size provided as argument
for size in "$@"; do
    echo "----------------------------------------"
    echo "Testing for matrix size: $size"
    run_programs $size
done
