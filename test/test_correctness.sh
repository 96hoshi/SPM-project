#!/bin/bash

# Check if matrix sizes are provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <size1> [<size2> ...]"
    exit 1
fi

# Compilation
#mkdir ../build 
cd ../build/
cmake -DENABLE_BENCHMARK=OFF ..
make -j 8

# Function to run the programs for a given matrix size
run_programs() {
    local size=$1

    # Run the sequential version and capture output
    SEQ_OUTPUT=$(../build/wf_sequential $size)

    # Run the FastFlow parallel version and capture output
    PAR_OUTPUT=$(../build/wf_parallel $size)

    # Run the FastFlow farm version and capture output
    FARM_OUTPUT=$(../build/wf_farm $size 4)

    # Compare outputs for correctness
    if [ "$SEQ_OUTPUT" == "$PAR_OUTPUT" ]; then
        echo "FastFlow Parallel matches."
    else
        echo "FastFlow Parallel  does not match."
        echo "Sequential: $SEQ_OUTPUT"
        echo "Parallel  : $PAR_OUTPUT"
    fi

    if [ "$SEQ_OUTPUT" == "$FARM_OUTPUT" ]; then
        echo "FastFlow Farm: matches."
    else
        echo "FastFlow Farm does not match."
        echo "Sequential: $SEQ_OUTPUT"
        echo "Farm      : $FARM_OUTPUT"
    fi

    echo
}

# Loop through each matrix size provided as argument
for size in "$@"; do
    echo "----------------------------------------"
    echo "Testing for matrix size: $size"
    run_programs $size
done
