#!/bin/bash

# Go to the gpu directory and make the GPU BTC miner
cd ../gpu
make clean
make

echo "Made GPU miner, launching..."

./btc_miner_gpu.exe > Dsha256_gpu.txt &

echo "Waiting 20 seconds..."

# wait 20 seconds
sleep 20

# issue a control-c to stop the miner
killall btc_miner_gpu.exe
make clean

# print done
echo "Done"
