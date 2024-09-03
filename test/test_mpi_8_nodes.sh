#!/bin/bash
#SBATCH --job-name=mpi_nodes
#SBATCH --output=results/mpi/mpi_nodes_%j.out
#SBATCH --error=results/mpi/error_mpi_nodes%j.err 
#SBATCH -t 00:10:00 #(hrs:min:sec)
#SBATCH --nodes=8
#SBATCH --ntasks-per-node=1

# Compilation
if [ -d "../build" ]; then
    rm -rf ../build
fi
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8

# Write the CSV header
echo "size,task_per_node,nodes,,time"

# Run the program for different sizes from 1024 to 8192
#map=("ppr:1:node" "ppr:2:node" "ppr:4:node" "ppr:8:node")
nodes=(2 3 4 5 6 7 8)
size=4096
num_execution=4

for i in "${nodes[@]}";
do
  MPI_TIME=$(mpirun -n $nodes --map-by ppr:1:node ./mpi_wf $size)
  echo "$size,1,$nodes,$MPI_TIME"
done
