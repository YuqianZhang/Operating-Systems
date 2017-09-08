/***********************************************************************
 *  Module: Scheduler
 *
 *  Class:  G22-2250-001 
 *  Description:
 *  	Reference implementation of the Lab2 assignment for above Class
 *
 *  Author: Hubertus Franke (frankeh@cs.nyu.edu)
 *  
 ***********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

typedef int stime_t;

void report()
{
    // print the scheduler info

    // for each process print using this format print out 
    // the specifics of the process
    {
	int     id = 1;            // unique id for this process
	stime_t arrival =400;      // specified arrival time of process
	stime_t totaltime = 2000;  // specified total exec time of process
	stime_t cpuburst = 10;     // specified cpuburst of process
	stime_t ioburst = 20;      // specified ioburst of process
	int     static_prio = 0;   // static priority of the process 
	                           // 0 if not prio scheduler
	stime_t state_ts = 2900;   // time we entered into this state
	stime_t iowaittime = 1111; // total iowaittime of process during sim
	stime_t cpuwaittime= 2222; // time we were ready to run but did not

	printf("\nhow to each process result line should be printed:\n");	
	printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
	       id,
	       arrival, totaltime, cpuburst, ioburst, static_prio,
	       state_ts, // last time stamp
	       state_ts - arrival,
	       iowaittime,
	       cpuwaittime);
    }

    {
	// compute the following variables based on the simulation
	// and the final state of all the processes 
	
	int    maxfintime = 10;
	double cpu_util = 255.9887;
	double io_util  = 3.1415927;
	double avg_turnaround = 128.367;
	double avg_waittime = 0.0;
	double throughput = 8.9567;

	printf("\nhow to print the SUM line:\n");	
	printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
	       maxfintime,
	       cpu_util,
	       io_util,
	       avg_turnaround,
	       avg_waittime, 
	       throughput);
    }

}

round_check()
{
    printf("\nCheck correct precision output: should show '0.333 0.667'\n");
    printf("%.3lf %.3lf\n",1.0/3.0, 2.0/3.0);
}

main()
{
    report();
    round_check();
}
