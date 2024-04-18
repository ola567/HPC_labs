#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include "numgen.c"

int main(int argc,char **argv) {

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  //program input argument
  long inputArgument = ins__args.arg; 

  struct timeval ins__tstart, ins__tstop;

  // variables
  int myrank,nproc;
  unsigned long int *numbers;
  MPI_Request *requests;
  int requestcount = 0;
  int requestcompleted;
  int sentcount = 0;
  int recvcount = 0;
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
  }

  // run your computations here (including MPI communication)
  if (myrank == 0)
  {
    requests = (MPI_Request *)malloc(3*(nproc - 1) * sizeof(MPI_Request));
    if (!requests)
    {
        printf("\nNot enough memory");
        MPI_Finalize();
        return -1;
    }

    ranges = (double *)malloc(4 * (nproc - 1) * sizeof(double));
    if (!ranges)
    {
        printf ("\nNot enough memory");
        MPI_Finalize();
        return -1;
    }

    resulttemp = (double *)malloc((nproc - 1) * sizeof (double));
    if (!resulttemp)
    {
        printf ("\nNot enough memory");
        MPI_Finalize();
        return -1;
    }

    // first distribute some ranges to all slaves
    for (i = 1; i < nproc; i++)
    {
        range[1] = range[0] + RANGESIZE;
        MPI_Send(range, 2, MPI_DOUBLE, i, DATA, MPI_COMM_WORLD);
        sentcount++;
        range[0] = range[1];
    }

    // the first nproc requests will be for receiving, the latter ones for sending
    for (i = 0; i < 2 * (nproc - 1); i++) {
      requests[i] = MPI_REQUEST_NULL;	// none active at this point
    }
      
    // start receiving for results from the slaves
    for (i = 1; i < nproc; i++) {
      MPI_Irecv(&(resulttemp[i - 1]), 1, MPI_DOUBLE, i, RESULT, MPI_COMM_WORLD, &(requests[i - 1]));
    }

    // start sending new data parts to the slaves
    for (i = 1; i < nproc; i++)
    {
      range[1] = range[0] + RANGESIZE;
      ranges[2 * i - 2] = range[0];
      ranges[2 * i - 1] = range[1];

      // send it to process i
      MPI_Isend(&(ranges[2 * i - 2]), 2, MPI_DOUBLE, i, DATA, MPI_COMM_WORLD, &(requests[nproc - 2 + i]));

      sentcount++;
      range[0] = range[1];
    }
    while (range[1] < b)
    {
        // wait for completion of any of the requests
        MPI_Waitany(2 * nproc - 2, requests, &requestcompleted, MPI_STATUS_IGNORE);
        
        // if it is a result then send new data to the process
        // and add the result
        if (requestcompleted < (nproc - 1))
        {
          result += resulttemp[requestcompleted];
          recvcount++;

          // first check if the send has terminated
          MPI_Wait (&(requests[nproc - 1 + requestcompleted]), MPI_STATUS_IGNORE);
          // now send some new data portion to this process
          range[1] = range[0] + RANGESIZE;

          if (range[1] > b)
            range[1] = b;

          ranges[2 * requestcompleted] = range[0];
          ranges[2 * requestcompleted + 1] = range[1];
          MPI_Isend (&(ranges[2 * requestcompleted]), 2, MPI_DOUBLE, requestcompleted + 1, DATA, MPI_COMM_WORLD, &(requests[nproc - 1 + requestcompleted]));
          sentcount++;
          range[0] = range[1];

          // now issue a corresponding recv
          MPI_Irecv (&(resulttemp[requestcompleted]), 1, MPI_DOUBLE, requestcompleted + 1, RESULT, MPI_COMM_WORLD, &(requests[requestcompleted]));
        }
    }
	// now send the FINISHING ranges to the slaves
	// shut down the slaves
	range[0] = range[1];
	for (i = 1; i < nproc; i++)
	{
    ranges[2 * i - 4 + 2 * nproc] = range[0];
    ranges[2 * i - 3 + 2 * nproc] = range[1];
    MPI_Isend (range, 2, MPI_DOUBLE, i, DATA, MPI_COMM_WORLD, &(requests[2 * nproc - 3 + i]));
	}

	// now receive results from the processes - that is finalize the pending requests
  MPI_Waitall (3 * nproc - 3, requests, MPI_STATUSES_IGNORE);
	// now simply add the results
	for (i = 0; i < (nproc - 1); i++)
	{
    result += resulttemp[i];
	}
	// now receive results for the initial sends
  for (i = 0; i < (nproc - 1); i++)
	{
    MPI_Recv (&(resulttemp[i]), 1, MPI_DOUBLE, i + 1, RESULT, MPI_COMM_WORLD, &status);
    result += resulttemp[i];
    recvcount++;
	}
	// now display the result
	printf ("\nHi, I am process 0, the result is %f\n", result);
  } else //slave
  {		
    requests = (MPI_Request *) malloc (2 * sizeof (MPI_Request));

    if (!requests)
    {
      printf ("\nNot enough memory");
      MPI_Finalize ();
      return -1;
    }

    requests[0] = requests[1] = MPI_REQUEST_NULL;
    ranges = (double *) malloc (2 * sizeof (double));

    if (!ranges)
    {
        printf ("\nNot enough memory");
        MPI_Finalize ();
        return -1;
    }

    resulttemp = (double *) malloc (2 * sizeof (double));
    if (!resulttemp)
    {
        printf ("\nNot enough memory");
        MPI_Finalize ();
        return -1;
    }

	  // first receive the initial data
    MPI_Recv (range, 2, MPI_DOUBLE, 0, DATA, MPI_COMM_WORLD, &status);
    while (range[0] < range[1])
	  {			
      // if there is some data to process
	    // before computing the next part start receiving a new data part
	    MPI_Irecv (ranges, 2, MPI_DOUBLE, 0, DATA, MPI_COMM_WORLD, &(requests[0]));

	    // compute my part
      resulttemp[1] = SimpleIntegration (range[0], range[1]);

      // now finish receiving the new part
	    // and finish sending the previous results back to the master
      MPI_Waitall (2, requests, MPI_STATUSES_IGNORE);
	    range[0] = ranges[0];
	    range[1] = ranges[1];
	    resulttemp[0] = resulttemp[1];

	    // and start sending the results back
      MPI_Isend (&resulttemp[0], 1, MPI_DOUBLE, 0, RESULT, MPI_COMM_WORLD, &(requests[1]));
	}

	  // now finish sending the last results to the master
	 MPI_Wait (&(requests[1]), MPI_STATUS_IGNORE);
  }

  // synchronize/finalize your computations

  if (!myrank) {
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }
  
  MPI_Finalize();

}
