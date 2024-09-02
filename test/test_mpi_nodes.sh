#!/bin/bash
#SBATCH --job-name=mpi_nodes
#SBATCH --output=results/mpi/mpi_nodes%A_%a.out
#SBATCH --error=results/mpi/error_mpi_nodes%A_%a.err 
#SBATCH -t 00:10:00 #(hrs:min:sec)
#SBATCH --nodes=8
#SBATCH --ntasks-per-node=1

# Compilation
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8

# Number of execution of the program
nodes=(1 2 3 4 5 6 7 8)
arg=4096
for i in "${nodes[@]}";
do
  echo "1 task per node, nodes $nodes, size: $arg"
  mpirun -n $nodes --map-by ppr:1:node ./mpi_wf $arg
done
