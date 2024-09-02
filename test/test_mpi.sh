#!/bin/bash
#SBATCH --job-name=mpi_4_nodes
#SBATCH --output=results/mpi/mpi_4_nodes%A_%a.out
#SBATCH --error=results/mpi/error_mpi_4_nodes%A_%a.err 
#SBATCH --time=00:20:00 #30 min (hrs:min:sec)
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1

# Check if arguments are provided correctly
if [ $# -lt 1 ]; then
    echo "Usage: $0 <test_case_number>"
    exit 1
fi

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
  echo "1 task per node, 4 nodes, size: $arg" >> results/mpi/mpi_4_nodes.txt
  mpirun -np 1 --map-by ppr:1:node ./mpi_wf $arg >> results/mpi/mpi_4_nodes.txt
done
