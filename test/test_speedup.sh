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
declare -a case1_sizes=(10 50 100)
declare -a case2_sizes=(200 500 1000)
declare -a case3_sizes=(2000 5000 10000)

# Output file for all results
output_file="./results/speedup_summary.txt"

# Function to run the programs for a given matrix size and save results to output file
run_programs() {
    local size=$1

    # Run the sequential version and save output
    SEQ_TIME=$(../build/wf_sequential $size)

    # Run the FastFlow parallel version with specified number of threads and save output
    PAR_TIME=$(../build/wf_parallel $size $num_workers)

    # Run the FastFlow farm version with specified number of threads and save output
    FARM_TIME=$(../build/wf_farm $size $num_workers)

    # Calculate the speedup and save results
    PAR_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $PAR_TIME" | bc)
    FARM_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $FARM_TIME" | bc)

    # Append results to output file
    echo "Test Case: $test_case, Matrix Size: $size, Num Workers: $num_workers" >> $output_file
    echo "Sequential Time:  $SEQ_TIME seconds" >> $output_file
    echo "FF Parallel Time: $PAR_TIME seconds" >> $output_file
    echo "FF Farm Time:     $FARM_TIME seconds" >> $output_file
    echo "Speedup Parallel: $PAR_SPEEDUP" >> $output_file
    echo "Speedup Farm:     $FARM_SPEEDUP" >> $output_file
    echo "------------------------------------" >> $output_file
}

# Determine which test case to run
case $test_case in
    1)
        sizes=("${case1_sizes[@]}")
        ;;
    2)
        sizes=("${case2_sizes[@]}")
        ;;
    3)
        sizes=("${case3_sizes[@]}")
        ;;
    *)
        echo "Invalid test case number."
        exit 1
        ;;
esac

# Loop through sizes for the selected test case and save the results
for size in "${sizes[@]}"; do
    run_programs $size
done

echo "Results summary saved in $output_file"
