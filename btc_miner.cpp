#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include <omp.h>

using namespace std;

int main() {
  // Test OpenMP:
  // get the number of threads
  int nthreads = omp_get_max_threads();
  // print the number of threads
  cout << "Number of threads: " << nthreads << endl;


  return 0;
}
