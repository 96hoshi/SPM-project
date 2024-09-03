#!/bin/bash
#SBATCH --job-name=ff_scaling
#SBATCH --output=results/times/ff_scaling%_j.out
#SBATCH --error=results/times/error_ff_scaling%_j.err 
#SBATCH -t 00:40:00 #(hrs:min:sec)
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1

# Check if arguments are provided correctly
if [ $# -lt 2 ]; then
    echo "Usage: $0 <test_case_number> <on_demand>"
    exit 1
fi

# Configuring FastFlow according to the architecture of the machine
fastflow_dir="../include/ff"
cd "$fastflow_dir" || { echo "Failed to change directory to $fastflow_dir"; exit 1; }
echo "Y" | ./mapping_string.sh
cd ..

# Compilation
if [ -d "../build" ]; then
    rm -rf ../build
fi
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8

# Extract inputs
test_case=$1
on_demand=$2

# Define matrix sizes for each test case 
declare -a small_sizes=(50 89 128)
declare -a medium_sizes=(128 256 1024)
declare -a big_sizes=(1024 2048 4096)
declare -a final_sizes=(128 516 1000 1001 1024 2048 4096 8192)

# Define the number of workers to test with
declare -a workers=(1 2 4 8 16 32)

# Write the CSV header
echo "method,size,#w,on-demand,time,speedup,efficiency"

# Function to run the programs for a given matrix size and save results to output file
run_programs() {
    local size=$1

    # Run the sequential version and save output
    SEQ_TIME=$(sequential_wf $size)

    # Record the sequential time in the output file
    echo "seq,$size,1,0,$SEQ_TIME,1"

    # Iterate over the number of workers for parallel and farm versions
    for num_workers in "${workers[@]}"; do
        # Run the FastFlow farm version with specified number of threads and save output
        FARM_TIME=$(ff_farm_wf $size $num_workers $on_demand)

        # Run the parallel version with specified number of threads and save output
        PAR_TIME=$(ff_parallel_wf $size $num_workers)

        # Calculate the speedup
        FARM_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $FARM_TIME" | bc)
        PAR_SPEEDUP=$(echo "scale=2; $SEQ_TIME / $PAR_TIME" | bc)

        # Calculate the efficiency
        FARM_EFFICIENCY=$(echo "scale=2; $FARM_SPEEDUP / $num_workers" | bc)
        PAR_EFFICIENCY=$(echo "scale=2; $PAR_SPEEDUP / $num_workers" | bc)

        # Append results to output file
        echo "frm,$size,$num_workers,$on_demand,$FARM_TIME,$FARM_SPEEDUP,$FARM_EFFICIENCY"
        echo "par,$size,$num_workers,1,$PAR_TIME,$PAR_SPEEDUP,$PAR_EFFICIENCY"
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
