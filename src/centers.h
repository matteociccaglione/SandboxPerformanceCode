/**
 * @file centers.h
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-07-31
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file contains all data structures used to maintain status information of the 
 * system's centers during the simulation.
 */

#ifndef __centers_h__
    #define __centers_h__
#include "config.h"
#include "job.h"
void initializeServerArray(int *servers, int n);

typedef struct __digestCenter{
    // This is a SSQ center
    int jobs;                                   // number of job in the center (queue + single server)
    int jobsInQueue;                            // number of jobs in the queue
    double area;                                // time integrated number of jobs in the center
    double queueArea;                           // time integrated number of jobs in the queue
    double serviceArea;                         // time integrated number of jobs in service
    int index;                                  // number of completely processed jobs
    int indexPremium;                           // number of completely processed PREMIUM jobs
    int digestMatching;                         // number of jobs whose digest matches with digest of already analyzed jobs
    double lastEventTime;                       // simulation time of the last event occurred at the center
    double probabilityOfMatching;               // probability of a matching digest (it increases over time)
    double interarrivalTime;                    // sum of interarrival times
    job_queue *queue;                           // queue of jobs
}digestCenter;

typedef struct __normalAnalysisCenter
{
    // This is a MSQ center
    int jobs;                                   // number of job in the center (queue + single server)
    int jobsInQueue;                            // number of jobs in the queue
    int servers[N_NORMAL];                      // status of single server of the MSQ center (busy=1, idle=0)
    double area;                                // time integrated number of jobs in the center
    double queueArea;                           // time integrated number of jobs in the queue
    double serviceArea;                         // time integrated number of jobs in service
    int index;                                  // number of completely processed jobs
    int numberOfTimeouts;                       // number of jobs terminated with expiration of timeout
    double lastEventTime;                       // simulation time of the last event occurred at the center
    double interarrivalTime;                    // sum of interarrival times
    double lastArrivalTime;                     // simulation time of the last arrival event occurred at the center
    job_queue *queue;                           // queue of jobs
}normalAnalysisCenter;

typedef struct __premiumAnalysisCenter
{
    // This is a MSQ center
    int jobs;                                   // number of job in the center (queue + single server)
    int jobsInQueue;                            // number of jobs in the queue
    int servers[N_PREMIUM];                     // status of single server of the MSQ center (busy=1, idle=0)
    double area;                                // time integrated number of jobs in the center
    double queueArea;                           // time integrated number of jobs in the queue
    double serviceArea;                         // time integrated number of jobs in service
    int index;                                  // number of completely processed jobs
    int numberOfTimeouts;                       // number of jobs terminated with expiration of timeout
    double lastEventTime;                       // simulation time of the last event occurred at the center
    double interarrivalTime;                    // sum of interarrival times
    double lastArrivalTime;                     // simulation time of the last arrival event occurred at the center
    job_queue *queue;                           // queue of jobs
}premiumAnalysisCenter;

typedef struct __reliableAnalysisCenter
{
    // This is a SSQ in original system; a MSQ in the improved system
    int jobs;                                   // number of job in the center (queue + single server)
    int jobsInQueuePremium;                     // number of premium jobs in the queue
    int jobsInQueueNormal;                      //number of normal jobs in the queue
    int premiumJobs;                            // number of premium job in the center (queue + single server)
    int normalJobs;                             // number of normal job in the center (queue + single server)
    int servers[N_RELIABLE];                    // status of single server of the MSQ center (busy=1, idle=0)
    double area;                                // time integrated number of jobs in the center
    double queueArea;                           // time integrated number oj jobs in both queue
    double queueAreaPremium;                    // time integrated number of premium jobs in the queue
    double queueAreaNormal;                     // time integrated number of normal jobs in the queue
    double serviceArea;                         // time integrated number of jobs in service
    double serviceAreaPremium;                  // time integrated number of jobs in service of premium jobs
    double serviceAreaNormal;                   // time integrated number of jobs in service of normal jobs
    double areaPremium;                         // time integrated number of premium jobs in the center
    double areaNormal;                          // time integrated number of normal jobs in the center
    int index;                                  // number of completely processed jobs
    int premiumIndex;                           // number of completely processed premium jobs
    int normalIndex;                            // number of completely processed normal jobs
    int numberOfTimeouts;                       // number of jobs terminated with expiration of timeout
    double lastEventTime;                       // simulation time of the last event occurred at the center
    double lastEventTimePremium;                // simulation time of the last event occurred at the center about premium jobs
    double lastEventTimeNormal;                 // simulation time of the last event occurred at the center about normal jobs
    double interarrivalTime;                    // sum of interarrival times
    double lastArrivalTime;                     // simulation time of the last arrival event occurred at the center
    int jobAnalyzed;                            // number of jobs analyzed without timeout expiration
    job_queue *queueNormal;                     // queue of normal jobs               
    job_queue *queuePremium;                    // queue of premium jobs
}reliableAnalysisCenter;

typedef struct __machineLearningCenter
{
    int jobs;                                   // number of job in the center (queue + single server)
    double area;                                // time integrated number of jobs in the center
    double serviceArea;                         // time integrated number of jobs in service
    int index;                                  // number of completely processed jobs
    int indexPremium;                           // number of completely processed PREMIUM jobs
    int mlSuccess;                              // number of positive result from machine learning model
    double lastEventTime;                       // simulation time of the last event occurred at the center
    double interarrivalTime;                    // sum of interarrival times
    double lastArrivalTime;                     // simulation time of the last arrival event occurred at the center
    int numOfBypass;                            // number of jobs that bypassed the ML center because all servers were busy
}machineLearningCenter;

typedef enum __center{
    CENTER_DIGEST = 0,
    CENTER_NORMAL = 1,
    CENTER_PREMIUM = 2,
    CENTER_RELIABLE = 3,
    CENTER_ML = 4
}center;
#endif