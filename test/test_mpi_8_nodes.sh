#!/bin/bash
#SBATCH --job-name=mpi_final_core
#SBATCH --output=results/mpi/mpi_final_core_%j.out
#SBATCH --error=results/mpi/error_mpi_final_core_%j.err 
#SBATCH -t 00:59:00 #(hrs:min:sec)
#SBATCH --nodes=8

# Compilation
if [ -d "../build" ]; then
    rm -rf ../build
fi  
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=ON .. 
make -j 8

# Write the CSV header
echo "size,task_per_node,nodes,time"

# Run the program for different sizes
nodes=(2 3 4 5 6 7 8)
sizes=(1024 2048 4096 8192 10000 16384)

for size in "${sizes[@]}";
do
  for i in "${nodes[@]}";
  do
    MPI_TIME=$(mpirun -x OMP_NUM_THREADS=1 --bind-to core --map-by ppr:1:node -n $i ./mpi_wf $size)
    echo "$size,1,$i,$MPI_TIME"
    MPI_TIME_2=$(mpirun -x OMP_NUM_THREADS=2 --bind-to core --map-by ppr:2:node -n $i ./mpi_wf $size)
    echo "$size,2,$i,$MPI_TIME_2" 
    MPI_TIME_4=$(mpirun -x OMP_NUM_THREADS=4 --bind-to core --map-by ppr:4:node -n $i ./mpi_wf $size)
    echo "$size,4,$i,$MPI_TIME_4"
  done
done
