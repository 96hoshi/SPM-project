#!/bin/bash

# Define the first input parameter (size)
size=1024 
# Define the different values for the second input parameter (num_workers)
num_workers_list=(1 2 4 8 16)

# output file for all results
output_file="../test/results/workers_summary.txt"

# Compilation
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. > /dev/null
make -j 8 > /dev/null

# Loop through each value of num_workers and run the program
echo "Running ff_farm_wf with different number of workers..." > $output_file
for num_workers in "${num_workers_list[@]}"; do 
    echo "Running with num_workers = $num_workers..." >> $output_file
    ../build/ff_farm_wf $size $num_workers  >> $output_file
    echo "-----------------------------------------"  >> $output_file
done

