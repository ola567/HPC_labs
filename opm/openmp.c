#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>
#include "numgen.c"

int check_prime(long number) {
  for(long i=2; i*i <= number; i++){
    if(number % i == 0){
        return 0;
    }
  }
  return 1;
}

int main(int argc,char **argv) {


  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  //set number of threads
  omp_set_num_threads(ins__args.n_thr);
  
  //program input argument
  long inputArgument = ins__args.arg; 
  long prime_numbers = 0;
  long if_prime = 0;

  unsigned long int *numbers = (unsigned long int*)malloc(inputArgument * sizeof(unsigned long int));
  numgen(inputArgument, numbers);

  struct timeval ins__tstart, ins__tstop;
  gettimeofday(&ins__tstart, NULL);
  
  // run your computations here (including OpenMP stuff)
  #pragma omp parallel for private(if_prime) reduction(+:prime_numbers)
  for(int i = 0; i < inputArgument; i++) {
    if_prime = check_prime(numbers[i]);
    prime_numbers += if_prime;
  }
  
  printf("Ilość liczb pierwszych: %ld", prime_numbers);
  
  // synchronize/finalize your computations
  gettimeofday(&ins__tstop, NULL);
  ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);

}
