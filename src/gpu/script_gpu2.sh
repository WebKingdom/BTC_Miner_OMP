#!/bin/bash

# Copy/paste this job script into a text file and submit with the command:
#    sbatch thefilename
# job standard output will go to the file slurm-%j.out (where %j is the job ID)

#SBATCH --time=00:15:00               # walltime limit (HH:MM:SS)
#SBATCH --nodes=1                     # number of nodes
#SBATCH --ntasks-per-node=24          # 24 processor core(s) per node 
#SBATCH --mem=64G                     # maximum memory per node
#SBATCH --gres=gpu:a100:4
#SBATCH --partition=class-gpu-short   # class node(s)
#SBATCH --job-name="ssz_gpu1"
#SBATCH --output="hpc_gpu1-%j.out"    # job standard output file (%j replaced by job id)

# LOAD MODULES, INSERT CODE, AND RUN YOUR PROGRAMS HERE

# module load intel
# module load nvhpc
module load cuda
module load gcc
module load openssl
module load openmpi_hpc
module load openmpi

# originally 600 seconds = 10 minutes
TIMEOUT=840   # 14 minutes

echo "Start job"
make clean
make

# run executable in background and get the PID of the process
./btc_miner_gpu.exe &
MINER_PID=$!
sleep $TIMEOUT
kill -2 $MINER_PID

make clean
echo "End job after $TIMEOUT seconds"
