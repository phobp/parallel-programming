#!/bin/bash

N=500

# build
make matmult matmult_serial

echo "== SERIAL =="
./matmult_serial $N

echo "== PARALLEL =="
for t in 1 2 4 8 16; do
    OMP_NUM_THREADS=$t ./matmult $N
done

