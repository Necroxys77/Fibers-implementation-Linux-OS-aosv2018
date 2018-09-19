#!/bin/bash
echo [1\/3] Make clean 
make clean &
wait
echo [2\/3] Removing temp files
rm t &
wait
echo [3\/3] Removing module
sudo rmmod fibermodule 
exec bash
