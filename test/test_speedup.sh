#!/bin/bash

# Check if arguments are provided correctly
if [ $# -lt 2 ]; then
    echo "Usage: $0 <test_case_number> <on_demand>"
    exit 1
fi

# Compilation
rm -rf ../build
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8 
cd ../test/

# Extract inputs
test_case=$1
on_demand=$2

# Define matrix sizes for each test case 
declare -a small_sizes=(50 56 100)
declare -a medium_sizes=(128 256 1024)
declare -a big_sizes=(1024 2048 4096)
declare -a final_sizes=(128 516 1000 1001 1024 2048 4096 8192) 

# Define the number of workers to test with
declare -a workers=(2 4 6)

# Output file for all results
output_file="./results/speedup.csv"

# Write the CSV header
echo "method,size,#w,on-demand,time,speedup" >> $output_file

# Function to run the programs for a given matrix size and save results to output file
run_programs() {
    local size=$1

    # Run the sequential version and save output
    SEQ_TIME=$(../build/sequential_wf $size)

    # Record the sequential time in the output file
    echo "seq,$size,1,0,$SEQ_TIME,1" >> $output_file

    # Iterate over the number of workers for parallel and farm versions
    for num_workers in "${workers[@]}"; do
        # Run the FastFlow farm version with specified number of threads and save output
        FARM_TIME=$(../build/ff_farm_wf $size $num_workers $on_demand)

        # Run the parallel version with specified number of threads and save output
        PAR_TIME=$(../build/ff_parallel_wf $size $num_workers)

        # Calculate the speedup
        FARM_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $FARM_TIME" | bc)
        PAR_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $PAR_TIME" | bc)

        # Append results to output file
        echo "frm,$size,$num_workers,$on_demand,$FARM_TIME,$FARM_SPEEDUP" >> $output_file
        echo "par,$size,$num_workers,1,$PAR_TIME,$PAR_SPEEDUP" >> $output_file
    done
}

# Determine which test case to run
case $test_case in
    1)
        sizes=("${small_sizes[@]}")
        ;;
    2)
        sizes=("${medium_sizes[@]}")
        ;;
    3)
        sizes=("${big_sizes[@]}")
        ;;
    4)
        sizes=("${final_sizes[@]}")
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
