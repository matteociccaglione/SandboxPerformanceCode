/**
 * @file centers.c
 * @author A. Pepe - M. Ciccaglione
 * @version 0.1
 * @date 2022-07-31
 * 
 * @copyright Copyright (c) 2022
 * 
 * This file contains all data structures used to maintain status information of the 
 * system's centers during the simulation.
 */

#include "config.h"

typedef struct __digestCenter{
    // This is a SSQ center
    int jobs; // number of job in the center (queue + single server)
    double area; // time integrated number of jobs in the center
    double service_time; // service time of the job in service
    int index; // number of completely processed jobs
    int digestMatching; // number of jobs whose digest matches with digest of already analyzed jobs
}digestCenter;

typedef struct __normalAnalysisCenter
{
    // This is a MSQ center
    int jobs; // number of job in the center (queue + single server)
    int servers[N_NORMAL]; // status of single server of the MSQ center (busy=1, idle=0)
    double area; // time integrated number of jobs in the center
    double service_time[N_NORMAL]; // service time of the jobs in service in each server
    int index; // number of completely processed jobs
    int numberOfTimeouts; // number of jobs terminated with expiration of timeout
}normalAnalysisCenter;

typedef struct __premiumAnalysisCenter
{
    // This is a MSQ center
    int jobs; // number of job in the center (queue + single server)
    int servers[N_PREMIUM]; // status of single server of the MSQ center (busy=1, idle=0)
    double area; // time integrated number of jobs in the center
    double service_time[N_PREMIUM]; // service time of the jobs in service in each server
    int index; // number of completely processed jobs
    int numberOfTimeouts; // number of jobs terminated with expiration of timeout
}premiumAnalysisCenter;

typedef struct __reliableAnalysisCenter
{
    // This is a MSQ center
    int jobs; // number of job in the center (queue + single server)
    int servers[N_RELIABLE]; // status of single server of the MSQ center (busy=1, idle=0)
    double area; // time integrated number of jobs in the center
    double service_time[N_RELIABLE]; // service time of the jobs in service in each server
    int index; // number of completely processed jobs
    int numberOfTimeouts; // number of jobs terminated with expiration of timeout
}reliableAnalysisCenter;


typedef enum __center{
    DIGEST = 0,
    NORMAL = 1,
    PREMIUM = 2,
    RELIABLE = 3
}center;
