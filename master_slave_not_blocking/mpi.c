#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include "numgen.c"

#define PACKETSIZE 10
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

  // variables
  int myrank,nproc;
  long *numbers;
  long iterator = 0;
  long *sublist1;
  long *sublist2;
  long *resulttemp;
  long *finisharray;
  MPI_Request *requests;
  int requestcount = 0;
  int requestcompleted;
  long result;
  long numberssize = inputArgument + (PACKETSIZE - (inputArgument % PACKETSIZE));

  MPI_Status status;

  MPI_Init(&argc,&argv);

  // obtain my rank
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  MPI_Comm_size(MPI_COMM_WORLD,&nproc);

  if(!myrank){
    gettimeofday(&ins__tstart, NULL);

    long number_of_packets = numberssize / PACKETSIZE;
    long packet_per_slave = number_of_packets / (nproc - 1);

    if(packet_per_slave < 2){
      long packets_to_add = nproc * 2 - number_of_packets;
      numberssize += packets_to_add * PACKETSIZE;
    }

	  numbers = (unsigned long int*)malloc(numberssize * sizeof(unsigned long int));
  	numgen(inputArgument, numbers);

    for(int i=inputArgument; i<numberssize; i++){
      numbers[i] = 0;
    }
  }

  // run your computations here (including MPI communication)
  if (myrank == 0)
  {
    finisharray = (long *)malloc(PACKETSIZE * sizeof(long));
    for(int i=0; i<PACKETSIZE; i++){
      finisharray[i] = -1;
    }
    requests = (MPI_Request *)malloc(3*(nproc - 1) * sizeof(MPI_Request));
    resulttemp = (long *)malloc((nproc - 1) * sizeof (long));

    // first distribute some ranges to all slaves
    for(int i = 1; i < nproc; i++)
    {
        MPI_Send(numbers + iterator, PACKETSIZE, MPI_LONG, i, DATA, MPI_COMM_WORLD);
        iterator += PACKETSIZE;
    }

    // the first nproc requests will be for receiving, the latter ones for sending
    for (int i = 0; i < 3 * (nproc - 1); i++) {
      requests[i] = MPI_REQUEST_NULL;	// none active at this point
    }
      
    // start receiving for results from the slaves
    for (int i = 1; i < nproc; i++) {
      MPI_Irecv(&(resulttemp[i - 1]), 1, MPI_LONG, i, RESULT, MPI_COMM_WORLD, &(requests[i - 1]));
    }

    // start sending new data parts to the slaves
    for (int i = 1; i < nproc; i++)
    {
      // send it to process i
      MPI_Isend(numbers + iterator, PACKETSIZE, MPI_LONG, i, DATA, MPI_COMM_WORLD, &(requests[nproc - 2 + i]));
      iterator++;
    }
    while (iterator < numberssize)
    {
        // wait for completion of any of the requests
        MPI_Waitany(2 * nproc - 2, requests, &requestcompleted, MPI_STATUS_IGNORE);
        
        // if it is a result then send new data to the process
        // and add the result
        if (requestcompleted < (nproc - 1))
        {
          result += resulttemp[requestcompleted];

          // first check if the send has terminated
          MPI_Wait(&(requests[nproc - 1 + requestcompleted]), MPI_STATUS_IGNORE);
          // now send some new data portion to this process
          MPI_Isend(numbers + iterator, PACKETSIZE, MPI_LONG, requestcompleted + 1, DATA, MPI_COMM_WORLD, &(requests[nproc - 1 + requestcompleted]));
          iterator++;

          // now  a corresponding recv
          MPI_Irecv(&(resulttemp[requestcompleted]), 1, MPI_LONG, requestcompleted + 1, RESULT, MPI_COMM_WORLD, &(requests[requestcompleted]));
        }
    }
  // send finalize signal
  for (int i = 1; i < nproc; i++)
	{
	    MPI_Isend(finisharray, PACKETSIZE, MPI_LONG, i, DATA, MPI_COMM_WORLD, &(requests[2 * nproc - 3 + i]));
	}
	// now receive results from the processes - that is finalize the pending requests
  MPI_Waitall(3 * nproc - 3, requests, MPI_STATUSES_IGNORE);
	// now simply add the results
	for (int i = 0; i < (nproc - 1); i++)
	{
    result += resulttemp[i];
	}
	// now receive results for the initial sends
  for (int i = 0; i < (nproc - 1); i++)
	{
    MPI_Recv(&(resulttemp[i]), 1, MPI_DOUBLE, i + 1, RESULT, MPI_COMM_WORLD, &status);
    result += resulttemp[i];
	}
	// now display the result
	printf ("\nHi, I am process 0, the result is %ld\n", result);
  } else //slave
  {		
    requests = (MPI_Request *) malloc (2 * sizeof (MPI_Request));
    requests[0] = requests[1] = MPI_REQUEST_NULL;
    sublist1 = (long *)malloc(PACKETSIZE * sizeof(long));
    sublist2 = (long *)malloc(PACKETSIZE * sizeof(long));
    resulttemp = (long *) malloc (2 * sizeof (long));

	  // first receive the initial data
    MPI_Recv(sublist1, PACKETSIZE, MPI_LONG, 0, DATA, MPI_COMM_WORLD, &status);

    while (sublist1[0] != -1)
	  {			
      // if there is some data to process
	    // before computing the next part start receiving a new data part
	    MPI_Irecv(sublist2, PACKETSIZE, MPI_LONG, 0, DATA, MPI_COMM_WORLD, &(requests[0]));

      unsigned long int slave_result = 0;
      for(int i = 0; i < PACKETSIZE; i++)
      {
        resulttemp[1] += check_prime(sublist1[i]);
      }

      // now finish receiving the new part
	    // and finish sending the previous results back to the master
      MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);
	    resulttemp[0] = resulttemp[1];
      sublist1 = sublist2;

	    // and start sending the results back
      MPI_Isend(&resulttemp[0], 1, MPI_LONG, 0, RESULT, MPI_COMM_WORLD, &(requests[1]));
	}

	  // now finish sending the last results to the master
	 MPI_Wait(&(requests[1]), MPI_STATUS_IGNORE);
  }

  // synchronize/finalize your computations

  if (!myrank) {
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }
  
  MPI_Finalize();

}
