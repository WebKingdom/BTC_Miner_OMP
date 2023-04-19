#!/bin/bash

# Go to the parallel directory and make the parallel BTC miner
cd ../parallel
make clean
make

echo "Made parallel miner, launching..."

./btc_miner_parallel.exe > Dsha256_parallel.txt &

echo "Waiting 20 seconds..."

# wait 20 seconds
sleep 20

# issue a control-c to stop the miner
killall btc_miner_parallel.exe
make clean

# print done
echo "Done"
