/**
 * @file stats.h
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-08-24
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file declares the struct stats, used to contain average values of the statistics computed in a simulation run.
 * It also declares the prototype of the function used to compute the statistics.
 */
#ifndef __stats_h__
    #define __stats_h__
#include <stdio.h>
#include <stdlib.h>
#include "centers.h"

typedef struct __stats{
    int numJobs;                                    // number of processed jobs
    int numNormalJobs;                              // number of processed jobs sumbitted by Normal users
    int numPremiumJobs;                             // number of processed jobs sumbitted by Premium users
    double realSimulationTime;                      // real duration in seconds of the simulation run

    double responseTime[5];                         // avg response time in the centers
    double waitTime[5];                             // avg wait time in the centers
    int numDigestMatching;                          // number of jobs whose digest has matched
    double avgNumberOFJobs[5];                      // E(N), average number of jobs in the centers
    double interarrivalTime[5];                     // avg interarrival time at the centers
    double serviceTime[5];                          // avg service time in the centers
    double rho[5];                                   // utilizations of the centers
    double numOfTimeouts[3];                        // number of timeouts in the normal, premium and reliable center
    double numOfBypass;                             // number of bypass in the ML center
    double bypassPercentage;                        // percentage of bypass
    double globalResponseTime;                      // response time for a job
    double globalPremiumResponseTime;               // response time for a job submitted by a premium user 
    double globalNormalResponseTime;                // response time for a job submitted by a normal user
    double globalFailurePercentage;                 // percentage of jobs processed but exited from the system due to timeout expiration

    double *samplesResponseTime;                    // array of global response time samples (one each 5 minutes)
    int sampleArraySize;                            // size of the array of samples
}stats;

stats computeStatistics(digestCenter digestCenter, normalAnalysisCenter normalAnalysisCenter, premiumAnalysisCenter premiumAnalysisCenter, reliableAnalysisCenter reliableAnalysisCenter, machineLearningCenter mlCenter, char *filename, int runNumber, int simulationTime);

#endif