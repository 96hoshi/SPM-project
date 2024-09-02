#!/bin/bash
#SBATCH --job-name=mpi_nodes
#SBATCH --output=results/mpi/mpi_nodes%A_%a.out
#SBATCH --error=results/mpi/error_mpi_nodes%A_%a.err 
#SBATCH --time=00:02:00 (hrs:min:sec)
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1

# Compilation
rm -rf ../build
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8

# Number of execution of the program
nodes=4
arg=4096
for i in $(seq 1 $nodes)
do
  echo "1 task per node, nodes $nodes, size: $arg" >> results/mpi/mpi_nodes.txt
  mpirun -N $nodes ./mpi_wf $arg >> results/mpi/mpi_nodes.txt
done
