#!/bin/bash

# build
make

echo "== SERIAL =="
./hello_serial $N
echo

echo "== PARALLEL =="
for t in 1 2 4 8 16; do
    echo "OMP_NUM_THREADS=$t"
    OMP_NUM_THREADS=$t ./hello $N
done
echo

