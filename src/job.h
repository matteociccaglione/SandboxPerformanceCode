/**
 * @file job.h
 * @author A. Pepe - M. Ciccaglione 
 * @version 1.0
 * @date 2022-08-24
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file contains declarations of user type enumeration, job struct and job queue struct, used to implement FIFO queues.
 * It also declares function to insert and remove jobs in the queue and to get the queue length. 
 */
#ifndef __job_h__
    #define __job_h__
#include<stdlib.h>
#include<stdio.h>

typedef enum __userType{
    PREMIUM = 1,
    NORMAL = 0
}UserType;

typedef struct __job{
    UserType userType;                                      // Job submitted by a premium user or a normal one
    double serviceTime;                                     // Service time 
    double arrivalTime;                                     // Time in which the job arrives to the center
}Job;

typedef struct __job_queue{
    Job job;
    struct __job_queue *next;
}job_queue;

void insertQueue(job_queue **queue, job_queue *node);
job_queue* popQueue(job_queue **queue);
int sizeQueue(job_queue *queue);
#endif