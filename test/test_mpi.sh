#!/bin/bash
#SBATCH --job-name=mpi_4_nodes
#SBATCH --output=results/mpi/mpi_4_nodes%A_%a.out
#SBATCH --error=results/mpi/error_mpi_4_nodes%A_%a.err 
#SBATCH -t 00:10:00 #30 min (hrs:min:sec)
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1

# Compilation
rm -rf ../build
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8

# Number of execution of the program
num_execution=4
start_val=1024
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "1 task per node, 4 nodes, size: $arg"
  mpirun -n 4 --map-by ppr:1:node ./mpi_wf $arg 
done
