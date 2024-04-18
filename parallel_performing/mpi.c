#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

int main(int argc,char **argv) {

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  //program input argument
  long inputArgument = ins__args.arg;
  long divide_by;
  long result;
  long sum = 0;

  struct timeval ins__tstart, ins__tstop;

  int myrank,nproc;
  
  MPI_Init(&argc,&argv);

  // obtain my rank
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  // and the number of processes
  MPI_Comm_size(MPI_COMM_WORLD,&nproc);

  if(!myrank)
      gettimeofday(&ins__tstart, NULL);


  // run your computations here (including MPI communication)
  divide_by = myrank + 2;
  for(long i=divide_by; i*i <= inputArgument; i += nproc){
    if(inputArgument % i == 0){
        sum += 1;
    }
  }

  //reduce 
  MPI_Reduce(&sum,&result,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);

  // synchronize/finalize your computations

  if (!myrank) {
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
    if(result>0){
      printf("Liczba nie jest pierwsza\n");
    } else {
      printf("Liczba jest pierwsza\n");
    }
  }
  
  MPI_Finalize();

}
