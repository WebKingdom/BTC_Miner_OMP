btc_miner_gpu : btc_miner_gpu.o
	g++ -O2 -o btc_miner_gpu.exe btc_miner_gpu.o -ffast-math -fno-stack-protector -fcf-protection=none -fopenmp -lpthread -lssl -lcrypto
btc_miner_gpu.o : btc_miner_gpu.cpp
	g++ -c btc_miner_gpu.cpp -ffast-math -fno-stack-protector -fcf-protection=none -fopenmp -lpthread -fconcepts
clean :
	rm -f *.o btc_miner_gpu.exe
