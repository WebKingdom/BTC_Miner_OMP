btc_miner_serial : btc_miner_serial.o
	g++ -o btc_miner_serial.exe btc_miner_serial.o -lssl -lcrypto
btc_miner_serial.o : btc_miner_serial.cpp
	g++ -c btc_miner_serial.cpp -fconcepts
clean :
	rm -f *.o btc_miner_serial.exe
