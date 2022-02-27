#!/bin/bash
for i in {1..100}; do
  COTTON_WORKERS=8 ./nqueens 12
done