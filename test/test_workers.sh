#!/bin/bash

# Define the first input parameter (size)
size=1000
# Define the different values for the second input parameter (num_workers)
num_workers_list=(1 2 3 4 5 6 7 8 9 10 11 12 13)

# Output file for all results
output_file="../test/results/workers_summary.csv"

# Compilation
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. > /dev/null
make -j 8 > /dev/null

# Write the CSV header
echo "FF_Farm,size,#w,time" > $output_file

# Loop through each value of num_workers and run the program
for num_workers in "${num_workers_list[@]}"; do 
    # Run the FastFlow farm version with specified number of threads and save output
    FARM_TIME=$(../build/ff_farm_wf $size $num_workers)

    # Append results to output file in CSV format
    echo "$size,$num_workers,$FARM_TIME" >> $output_file
done

echo "Results summary saved in $output_file"
