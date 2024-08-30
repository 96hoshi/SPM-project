#!/bin/bash
#SBATCH --job-name=mpi_4_nodes
#SBATCH --output=results/mpi/mpi_4_nodes%A_%a.out
#SBATCH --error=results/mpi/error_mpi_4_nodes%A_%a.err 
#SBATCH --time=00:30:00 #30 min (hrs:min:sec)
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=6

# Number of execution of the program
num_execution=3
start_val=1024 #from 2048 to 8192
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 6 task per node, 4 nodes, with argument: $arg" >> results/mpi/mpi_4_nodes.txt
  mpirun -np 6 --map-by ppr:1:node ../build/mpi_wf $arg >> results/mpi/mpi_4_nodes.txt
done