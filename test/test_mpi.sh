#!/bin/bash
#SBATCH --job-name=mpi_4_nodes
#SBATCH --output=../../results/mpi/mpi_4_nodes%A_%a.out
#SBATCH --error=../../results/mpi/error_mpi_4_nodes%A_%a.err
#SBATCH --time=00:30:00 #30 min (hrs:min:sec)
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=4

# Number of execution of the program
num_execution=4
start_val=1024 #from 2048 to 16'384
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 1 task per node, 4 nodes, with argument: $arg"
  mpirun -np 4 --map-by ppr:1:node ../../build/src/parallel_mpi $arg
done