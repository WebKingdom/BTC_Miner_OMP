gpu_test1 : gpu_test1.o
	g++-9 -o gpu_test1.exe gpu_test1.o -fopt-info-all-omp -fno-stack-protector -fcf-protection=none -fopenmp -lpthread
gpu_test1.o : gpu_test1.cpp
	g++-9 -c gpu_test1.cpp -fopt-info-all-omp -fno-stack-protector -fcf-protection=none -fopenmp -lpthread
clean :
	rm -f *.o gpu_test1.exe
