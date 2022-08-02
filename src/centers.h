/**
 * @file centers.h
 * @author A. Pepe - M. Ciccaglione
 * @version 0.1
 * @date 2022-07-31
 * 
 * @copyright Copyright (c) 2022
 * 
 * This file contains all data structures used to maintain status information of the 
 * system's centers during the simulation.
 */
#ifndef __centers_h__
    #define __centers_h__
#include "config.h"
#include "job.h"
void initializeServerArray(double *service_time,int *servers, int n);
typedef struct __digestCenter{
    // This is a SSQ center
    int jobs; // number of job in the center (queue + single server)
    int jobsInQueue; // number of jobs in the queue
    double area; // time integrated number of jobs in the center
    double queueArea; // time integrated number of jobs in the queue
    double service_time; // service time of the job in service
    int index; // number of completely processed jobs
    int indexPremium; // number of completely processed PREMIUM jobs
    int digestMatching; // number of jobs whose digest matches with digest of already analyzed jobs
    double lastEventTime;
    double probabilityOfMatching;
    double interarrivalTime;
    job_queue *queue;
}digestCenter;

typedef struct __normalAnalysisCenter
{
    // This is a MSQ center
    int jobs; // number of job in the center (queue + single server)
    int jobsInQueue; // number of jobs in the queue
    int servers[N_NORMAL]; // status of single server of the MSQ center (busy=1, idle=0)
    double area; // time integrated number of jobs in the center
    double queueArea; // time integrated number of jobs in the queue
    double service_time[N_NORMAL]; // service time of the jobs in service in each server
    int index; // number of completely processed jobs
    int numberOfTimeouts; // number of jobs terminated with expiration of timeout
    double lastEventTime;
    double interarrivalTime;
    double lastArrivalTime;
    job_queue *queue;
}normalAnalysisCenter;

typedef struct __premiumAnalysisCenter
{
    // This is a MSQ center
    int jobs; // number of job in the center (queue + single server)
    int jobsInQueue; // number of jobs in the queue
    int servers[N_PREMIUM]; // status of single server of the MSQ center (busy=1, idle=0)
    double area; // time integrated number of jobs in the center
    double queueArea; // time integrated number of jobs in the queue
    double service_time[N_PREMIUM]; // service time of the jobs in service in each server
    int index; // number of completely processed jobs
    int numberOfTimeouts; // number of jobs terminated with expiration of timeout
    double lastEventTime;
    double interarrivalTime;
    double lastArrivalTime;
    job_queue *queue;
}premiumAnalysisCenter;

typedef struct __reliableAnalysisCenter
{
    // This is a MSQ center
    int jobs; // number of job in the center (queue + single server)
    int jobsInQueuePremium; // number of premium jobs in the queue
    int jobsInQueueNormal; //number of normal jobs in the queue
    int premiumJobs; // number of premium job in the center (queue + single server)
    int servers[N_RELIABLE]; // status of single server of the MSQ center (busy=1, idle=0)
    double area; // time integrated number of jobs in the center
    double queueArea; // time integrated number oj jobs in both queue
    double queueAreaPremium; // time integrated number of premium jobs in the queue
    double queueAreaNormal; // time integrated number of normal jobs in the queue
    double areaPremium; // time integrated number of premium jobs in the center
    double service_time[N_RELIABLE]; // service time of the jobs in service in each server
    int index; // number of completely processed jobs
    int premiumIndex; // number of completely processed premium jobs
    int numberOfTimeouts; // number of jobs terminated with expiration of timeout
    double service_time_premium[N_RELIABLE];
    double service_time_normal[N_RELIABLE];
    double areaNormal;
    double lastEventTimeNormal;
    int normalJobs;
    int normalIndex;
    double lastEventTime;
    double lastEventTimePremium;
    double interarrivalTime;
    double lastArrivalTime;
    job_queue *queueNormal;
    job_queue *queuePremium;
}reliableAnalysisCenter;


typedef enum __center{
    CENTER_DIGEST = 0,
    CENTER_NORMAL = 1,
    CENTER_PREMIUM = 2,
    CENTER_RELIABLE = 3
}center;
#endif