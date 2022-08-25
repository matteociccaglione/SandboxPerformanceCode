/**
 * @file events_queue.h
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-07-31
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file contains the definitions of the structs representing the events of the simulation (arrivals and terminations).
 * It also defines the stream for the RNGs and declares a function prototype, used to generate a new arrival in the system. 
 */
#ifndef __events_queue_h__
    #define __events_queue_h__
#include "centers.h"
#include "job.h"
#include <stdlib.h>
#include <stdio.h>
#include "../lib/rngs.h"
#include "../lib/rvgs.h"

#define MEAN_INTERARRIVAL_STREAM 0
#define DIGEST_SERVICE_TIME_STREAM 1
#define USER_PROBABILITY_STREAM 2
#define DIGEST_MATCHING_PROBABILITY_STREAM 4
#define MEAN_SERVICE_TIME_PREMIUM_STREAM 5
#define MEAN_SERVICE_TIME_NORMAL_STREAM 6
#define MEAN_SERVICE_TIME_RELIABLE_STREAM 7
#define MEAN_SERVICE_TIME_ML_STREAM 8
#define ML_RESULT_STREAM 9

typedef struct __arrival{
    double time;                // time of the arrival
    Job job;                    // job associated to the event
    center center;              // ID of the center receiving the arrival 
    struct __arrival *next;     // Pointer to next node of the queue
}arrival;

typedef struct __termination{
    double time;                // time of the termination of the job
    Job job;                    // job associated to the event
    center center;              // ID of the center that processed the job
    int server_index;           // Index of the server that becomes idle. This value is not set for digest calculation node
    struct __termination *next; // Pointer to next node of the queue
}termination;

typedef struct __event_list
{
    arrival *arrivals;          // list of arrival events; it's an ordered by time list
    termination *terminations;  // list of termination events; it's an ordered by time list
} event_list;

arrival *getArrival(double simulationTime);
#endif
