btc_miner_parallel : btc_miner_parallel.o
	g++ -o btc_miner_parallel.exe btc_miner_parallel.o -fopenmp -lpthread -lssl -lcrypto
btc_miner_parallel.o : btc_miner_parallel.cpp
	g++ -c btc_miner_parallel.cpp -fopenmp -lpthread -fconcepts
clean :
	rm -f *.o btc_miner_parallel.exe
