#!/bin/bash
#SBATCH --job-name=mpi_4_nodes_correctness
#SBATCH --output=results/mpi/mpi_4_nodes%A_%a.out
#SBATCH --error=results/mpi/error_mpi_4_nodes%A_%a.err 
#SBATCH --time=00:10:00 #30 min (hrs:min:sec)
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1

# Compilation
if [ -d "../build" ]; then
    rm -rf ../build
fi
mkdir ../build
cd ../build/
cmake -DENABLE_BENCHMARK=OFF .. 
make -j 8 

# Define path to store results
path_results="./results/correctness/"

# Number of execution of the program
num_execution=2
start_val=1024 #from 1024 to 8192
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "MPI execution 1 task per node, 4 nodes, with argument: $arg"
  mpirun -n 4 --bind-to none mpi_wf $arg >> $path_results/mpi_$arg.txt
done

# compare the element of each matrix the output file contains only one number that must to match
compare_matrices() {
    local file1=$1
    local file2=$2
    local name=$3

    # Compare the output matrices
    diff $file1 $file2 > /dev/null
    if [ $? -eq 0 ]; then
        echo "$name: Correct"
    else
        echo "$name: Incorrect"
        echo "Sequential output:"
        cat $file1
        echo "Parallel output:"
        cat $file2
    fi
}

# Compare the output matrices
echo "Comparing the output matrices"
for i in $(seq 0 $((num_execution-1)))
do
  arg=$((start_val * (2 ** i)))
  echo "Size $arg"
  compare_matrices $path_results/seq_output_$arg.txt $path_results/mpi_$arg.txt "MPI"
  echo "------------------------------------"
done
