#ifndef __stats_h__
    #define __stats_h__
#include<stdio.h>
#include<stdlib.h>
typedef struct __stats{
    int numJobs; // number of processed jobs
    int numNormalJobs; // number of processed jobs sumbitted by Normal users
    int numPremiumJobs; //number of processed jobs sumbitted by Premium users
    double realSimulationTime; // real duration in seconds of the simulation run

    double responseTime[5]; // avg response time in the four centers
    double waitTime[5]; // avg wait time in the four centers
    int numDigestMatching; // number of jobs whose digest has matched
    double avgNumberOFJobs[5]; // E(N), average number of jobs in the four centers
    double interarrivalTime[5]; // 1/lambda for the 4 centers
    double serviceTime[5]; // avg service time in the four centers
    double ro[5];
    double numOfTimeouts[3]; // Num of timeouts in the normal, premium and reliable center
    double numOfBypass;
    double globalResponseTime;  // response time for a job
    double globalPremiumResponseTime; // response time for a preium job
    double globalNormalResponseTime; // response time for a normal job
    double globalFailurePercentage; // percentage of jobs processed but exited from the system for timeout expiration

    double *samplesResponseTime; // array of samples (one each 5 minutes)
    int sampleArraySize;
}stats;

typedef struct __avgStats{
    double numJobs;         // average number of processed jobs through runs
    double numNormalJobs;   // average number of processed jobs submitted by Normal users
    double numPremiumJobs;  // average number of processed jobs submitted by Premium users

    double responseTime[4]; // avg response time in the four centers
    double waitTime[4]; // avg wait time in the four centers
    double numDigestMatching; // number of jobs whose digest has matched
    double avgNumberOFJobs[4]; // E(N), average number of jobs in the four centers
    double interarrivalTime[4]; // 1/lambda for the 4 centers
    double serviceTime[4]; // avg service time in the four centers
    double numOfTimeouts[3]; // Num of timeouts in the last 3 centers
}avgStats;

#endif