#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include "numgen.c"

#define PACKETSIZE 13
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

  //program input argument
  long inputArgument = ins__args.arg; 

  struct timeval ins__tstart, ins__tstop;

  int myrank,nproc;
  unsigned long int *numbers;
  unsigned long int *numbers_filled;
  unsigned long int iterator = 0;
  unsigned long int res = 0;
  unsigned long int tmp = 0;
  unsigned long int *sublist;
  long int numbers_filled_size = inputArgument + (PACKETSIZE - (inputArgument % PACKETSIZE));

  MPI_Status status;

  MPI_Init(&argc,&argv);

  // obtain my rank
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  // and the number of processes
  MPI_Comm_size(MPI_COMM_WORLD,&nproc);

  if(!myrank){
    gettimeofday(&ins__tstart, NULL);
	  numbers = (unsigned long int*)malloc(inputArgument * sizeof(unsigned long int));
  	numgen(inputArgument, numbers);

    numbers_filled = (unsigned long int*)malloc(numbers_filled_size * sizeof(unsigned long int));
    for(int i=0; i<numbers_filled_size; i++){
      if(i<inputArgument){
        numbers_filled[i] = numbers[i];
      } else {
        numbers_filled[i] = 4;
      }
    }
  }

  // master
  if (myrank == 0){
    // first distribute some ranges to all slaves
    for(int i = 1; i < nproc; i++)
    {
      MPI_Send(numbers_filled + iterator, PACKETSIZE, MPI_LONG, i, DATA, MPI_COMM_WORLD);
      iterator += PACKETSIZE;
    }
    do
    {
      // distribute remaining subranges to the processes which have completed their parts
      MPI_Recv(&tmp, 1, MPI_LONG, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &status);
      res += tmp;
      MPI_Send(numbers_filled + iterator, PACKETSIZE, MPI_LONG, status.MPI_SOURCE, DATA, MPI_COMM_WORLD);
      iterator += PACKETSIZE;
    } while (iterator < numbers_filled_size);

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
          sublist = (unsigned long int*)malloc(PACKETSIZE * sizeof(unsigned long int));
          MPI_Recv(sublist, PACKETSIZE, MPI_LONG, 0, DATA, MPI_COMM_WORLD, &status);
          for(int i = 0; i < PACKETSIZE; i++)
          {
            slave_result += check_prime(sublist[i]);
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