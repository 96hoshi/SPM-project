#!/bin/bash

# Check if arguments are provided correctly
if [ $# -lt 1 ]; then
    echo "Usage: $0 <test_case_number>"
    exit 1
fi

# Compilation
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. >> /dev/null
make -j 8  >> /dev/null
cd ../test

# Extract inputs
test_case=$1

# Define matrix sizes for each test case 
declare -a small_sizes=(50 56 100)
declare -a medium_sizes=(128 256 1024)
declare -a big_sizes=(1024 2048 4096)
declare -a final_sizes=(128 516 1000 1001 1024 2048 4096 8192 16384) 

# Define the number of workers to test with
declare -a workers=(4 8 16 32)

# Output file for all results
output_file="./results/speedup_summary.csv"

# Write the CSV header
# echo "method,size,#w,#n,on-demand,time,speedup" >> $output_file

# Function to run the programs for a given matrix size and save results to output file
run_programs() {
    local size=$1

    # Run the sequential version and save output
    SEQ_TIME=$(../build/sequential_wf $size)

    # Record the sequential time in the output file
    echo "seq,$size,1,1,0,$SEQ_TIME,1" >> $output_file

    # Iterate over the number of workers for parallel and farm versions
    for num_workers in "${workers[@]}"; do
        # Run the FastFlow parallel version with specified number of threads and save output
        # PAR_TIME=$(../build/ff_parallel_wf $size $num_workers)

        # Run the FastFlow farm version with specified number of threads and save output
        FARM_TIME=$(../build/ff_farm_wf $size $num_workers)

        # Run the MPI version with specified number of processes and save output
        MPI_TIME=$(mpirun -np 6 ../build/mpi_wf $size)

        # Calculate the speedup
        #PAR_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $PAR_TIME" | bc)
        FARM_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $FARM_TIME" | bc)
        MPI_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $MPI_TIME" | bc)

        # Append results to output file
        #echo "par,$size,$num_workers,1,1,$PAR_TIME,$PAR_SPEEDUP" >> $output_file
        echo "frm,$size,$num_workers,1,1,$FARM_TIME,$FARM_SPEEDUP" >> $output_file
        echo "mpi,$size,$num_workers,1,1,$MPI_TIME,$MPI_SPEEDUP" >> $output_file
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

echo "Results summary saved in $output_file"
