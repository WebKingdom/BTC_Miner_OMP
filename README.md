# **Simulated Bitcoin Miner OpenMP**
Simulation of a parallelized Bitcoin miner using OpenMP. Used as a proof of concept and to compare serial and the parallelized OpenMP implementation performance.

# **Project Structure**
The ***src*** folder contains all the source code and versions of the simulated Bitcoin mining program. Each version (serial, parallel, and GPU) has a corresponding Makefile and a shell script to run the program for the desired amount of time.

The ***results*** folder contains all the data outputted by each version of the program.

To manually compile and run a selected version of the program simply navigate to the desired source directory and type:
```
make
./btc_miner_parallel.exe
```
Otherwise, run the shell script with:
```
sbatch script_parallel1.sh
```
OR:
```
./script_parallel_local.sh
```

# **Requirements**
OpenSSL must be installed. Visit https://www.openssl.org/ for more information.
