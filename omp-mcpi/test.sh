#!/bin/bash
#
#SBATCH --job-name=mcpi_omp
#SBATCH --nodes=1
#SBATCH --ntasks=1

NDARTS=1000000

make

echo "SERIAL:"
./mcpi_ser "$NDARTS"

echo "PARALLEL:"
OMP_NUM_THREADS=1  ./mcpi "$NDARTS"
OMP_NUM_THREADS=2  ./mcpi "$NDARTS"
OMP_NUM_THREADS=4  ./mcpi "$NDARTS"
OMP_NUM_THREADS=8  ./mcpi "$NDARTS"
OMP_NUM_THREADS=16 ./mcpi "$NDARTS"

