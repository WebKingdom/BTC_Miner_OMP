btc_miner : btc_miner.o
	g++ -o btc_miner.exe btc_miner.o -fopenmp -lpthread
btc_miner.o : btc_miner.cpp
	g++ -c btc_miner.cpp -fopenmp -lpthread
clean :
	rm -f *.o btc_miner
