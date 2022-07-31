/**
 * @file events_queue.c
 * @author A. Pepe - M. Ciccaglione
 * @version 0.1
 * @date 2022-07-31
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "job.c"
#include "centers.c"


typedef struct __arrival{
    double time;    // time of the arrival
    job job;        // job associated to the event
    center center;  // ID of the center receiving the arrival 
    struct __arrival *next; // Pointer to next node of the queue
}arrival;

typedef struct __termination{
    double time;    // time of the termination of the job
    job job;        // job associated to the event
    center center;  // ID of the center that processed the job
    int server_index; // Index of the server that becomes idle. This value is not set for digest calculation node
    struct __termination *next; // Pointer to next node of the queue
}termination;
