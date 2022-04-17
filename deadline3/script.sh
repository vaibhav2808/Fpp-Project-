#!/bin/bash
export COTTON_WORKERS=32
echo "Starting the test"
make
make test
./nqueens
./sor
./heat