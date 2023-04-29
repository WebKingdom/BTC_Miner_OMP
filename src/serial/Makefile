btc_miner_serial : btc_miner_serial.o
	g++ -O2 -o btc_miner_serial.exe btc_miner_serial.o -ffast-math -fno-stack-protector -fcf-protection=none -fopenmp -lssl -lcrypto
btc_miner_serial.o : btc_miner_serial.cpp
	g++ -c btc_miner_serial.cpp -ffast-math -fno-stack-protector -fcf-protection=none -fopenmp
clean :
	rm -f *.o btc_miner_serial.exe
