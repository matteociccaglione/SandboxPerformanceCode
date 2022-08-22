#ifndef __job_h__
    #define __job_h__
#include<stdlib.h>
#include<stdio.h>
typedef enum __userType{
    PREMIUM = 1,
    NORMAL = 0
}UserType;

typedef enum __jobType{
    MALWARE = 1,
    BENIGN = 0
}JobType;

typedef struct __job{
    UserType userType; // Job submitted by a premium user or a normal one
    JobType type; // Job type: malware, benign
    double serviceTime; // Service time 
    double arrivalTime; // Time in which the job arrives to the center
}Job;

typedef struct __job_queue{
    Job job;
    struct __job_queue *next;
}job_queue;

void insertQueue(job_queue **queue, job_queue *node);
job_queue* popQueue(job_queue **queue);
int sizeQueue(job_queue *queue);
#endif