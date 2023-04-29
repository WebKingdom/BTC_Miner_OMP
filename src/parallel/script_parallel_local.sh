#!/bin/bash
# originally 600 seconds = 10 minutes
TIMEOUT=840   # 14 minutes

echo "Start job"
make clean
make

# run executable in background and get the PID of the process
./btc_miner_parallel.exe > local_parallel1.out 2>&1 &
MINER_PID=$!
sleep $TIMEOUT
kill -2 $MINER_PID

make clean
echo "End job after $TIMEOUT seconds"
