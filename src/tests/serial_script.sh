#!/bin/bash

# Go to the serial directory and make the serial BTC miner
cd ../serial
make clean
make

echo "Made serial miner, launching..."

./btc_miner_serial.exe > Dsha256_serial.txt &

echo "Waiting 20 seconds..."

# wait 20 seconds
sleep 20

# issue a control-c to stop the miner
killall btc_miner_serial.exe
make clean

# print done
echo "Done"
