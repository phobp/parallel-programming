#!/bin/bash
#
#SBATCH --job-name=par_gauss
#SBATCH --nodes=1
#SBATCH --ntasks=1

function call_parallel {
    echo "THREADS $1 SIZE $2"
    OMP_NUM_THREADS=$1  ./par_gauss "$2"
}

make
for i in 1000 2000 4000 8000;
do
    echo
    echo "SERIAL: (SIZE: $i)"
    ./gauss "$i"
    echo "PARALLEL:"
    for p in 1 2 4 8 16;
    do
        call_parallel $p $i
    done
done
echo "DONE"
