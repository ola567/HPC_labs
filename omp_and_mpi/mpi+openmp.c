#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>
#include "numgen.c"

#define PACKETSIZE 10000
#define DATA 0
#define RESULT 1
#define FINISH 2

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

  struct timeval ins__tstart, ins__tstop;

  int threadsupport;
  int myrank,nproc;
  // Initialize MPI with desired support for multithreading -- state your desired support level

  MPI_Init_thread(&argc, &argv,MPI_THREAD_FUNNELED,&threadsupport); 

  if (threadsupport<MPI_THREAD_FUNNELED) {
    printf("\nThe implementation does not support MPI_THREAD_FUNNELED, it supports level %d\n",threadsupport);
    MPI_Finalize();
    return -1;
  }

  unsigned long int *numbers;
  unsigned long int iterator = 0;
  unsigned long int res = 0;
  unsigned long int tmp = 0;
  unsigned long int *sublist;
  long int numberssize = inputArgument;

  MPI_Status status;

  // obtain my rank
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  // and the number of processes
  MPI_Comm_size(MPI_COMM_WORLD,&nproc);

  if(!myrank){
    gettimeofday(&ins__tstart, NULL);

    if(numberssize < 2*(nproc - 1) * PACKETSIZE){
      numberssize = 2*(nproc - 1) * PACKETSIZE;
    }
    else {
      long number_of_packets = numberssize / PACKETSIZE + 1;
      numberssize = number_of_packets * PACKETSIZE;
    }

	  numbers = (unsigned long int*)malloc(numberssize * sizeof(unsigned long int));
  	numgen(inputArgument, numbers);

    for(int i=inputArgument; i<numberssize; i++){
      numbers[i] = 4;
    }
    printf("%ld", numberssize);
  }

  // master
  if (myrank == 0){
    // first distribute some ranges to all slaves
    for(int i = 1; i < nproc; i++)
    {
      MPI_Send(numbers + iterator, PACKETSIZE, MPI_LONG, i, DATA, MPI_COMM_WORLD);
      iterator += PACKETSIZE;
    }
    do
    {
      // distribute remaining subranges to the processes which have completed their parts
      MPI_Recv(&tmp, 1, MPI_LONG, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &status);
      res += tmp;
      MPI_Send(numbers + iterator, PACKETSIZE, MPI_LONG, status.MPI_SOURCE, DATA, MPI_COMM_WORLD);
      iterator += PACKETSIZE;
    } while (iterator < numberssize);

    // now receive results from the processes
    for(int i = 0; i < (nproc - 1); i++)
    {
      MPI_Recv(&tmp, 1, MPI_LONG, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &status);
      res += tmp;
    }
    // shut down the slaves
    for (int i = 1; i < nproc; i++)
    {
      MPI_Send(NULL, 0, MPI_LONG, i, FINISH, MPI_COMM_WORLD);
    }
    // now display the result
    printf ("\nHi, I am process 0, the result is %ld\n", res);}
  else
  {	
    // slave
    do
    {
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == DATA)
        {
          unsigned long int slave_result = 0;
          long if_prime = 0;
          sublist = (unsigned long int*)malloc(PACKETSIZE * sizeof(unsigned long int));
          MPI_Recv(sublist, PACKETSIZE, MPI_LONG, 0, DATA, MPI_COMM_WORLD, &status);

          #pragma omp parallel for private(if_prime) reduction(+:slave_result)
          for(int i = 0; i < PACKETSIZE; i++) {
            if_prime = check_prime(sublist[i]);
            slave_result += if_prime;
          }
        // send the result back
        MPI_Send(&slave_result, 1, MPI_LONG, 0, RESULT, MPI_COMM_WORLD);
        }
    } while (status.MPI_TAG != FINISH);
  }

  // synchronize/finalize your computations
  if (!myrank) {
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }
  
  MPI_Finalize();

}