#!/bin/bash
#SBATCH --job-name=mpi_4_nodes
#SBATCH --output=results/mpi/mpi_4_nodes%_j.out
#SBATCH --error=results/mpi/error_mpi_4_nodes%_j.err 
#SBATCH -t 00:50:00 #(hrs:min:sec)
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1

# Compilation
if [ -d "../build" ]; then
    rm -rf ../build
fi
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8

# Number of execution of the program
num_execution=5
start_val=1024
nodes=4

# Write the CSV header
echo "task_per_node,nodes,size,time"

# Run the program for different sizes from 1024 to 8192
for i in $(seq 0 $((num_execution-1))) 
do
  arg=$((start_val * (2 ** i)))
  MPI_TIME=$(mpirun -n $nodes --map-by ppr:1:node ./mpi_wf $arg)
  echo "1,$nodes,$arg,$MPI_TIME"
done
