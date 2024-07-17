#!/bin/bash

# Define the first input parameter (size)
size=1000 
# Define the different values for the second input parameter (num_workers)
num_workers_list=(1 2 4 8)

# Output file for all results
output_file="./results/workers_summary.txt"

# Compilation
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. > /dev/null
make -j 8 > /dev/null

# Loop through each value of num_workers and run the program
echo "Running wf_farm with different number of workers..."
for num_workers in "${num_workers_list[@]}"; do
    echo "Running with num_workers = $num_workers..."
    ../build/wf_farm $size $num_workers
    echo "-----------------------------------------"
done

echo "All results saved in $output_file"
