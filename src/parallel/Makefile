btc_miner_parallel : btc_miner_parallel.o
	g++ -O2 -o btc_miner_parallel.exe btc_miner_parallel.o -ffast-math -fno-stack-protector -fcf-protection=none -fopenmp -lssl -lcrypto
btc_miner_parallel.o : btc_miner_parallel.cpp
	g++ -c btc_miner_parallel.cpp -ffast-math -fno-stack-protector -fcf-protection=none -fopenmp
clean :
	rm -f *.o btc_miner_parallel.exe
