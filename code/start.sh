#!/bin/bash
echo [1\/4] Make
make &
wait
echo [2\/4] Inserting the module
sudo insmod fibermodule.ko &
wait
echo [3\/4] Compiling
gcc -o t test.c &
wait
echo [4\/4] Running the module
./t
exec bash
