#ifndef __stats_h__
    #define __stats_h__
typedef struct __stats{
    int numJobs; // number of processed jobs
    int numNormalJobs; // number of processed jobs sumbitted by Normal users
    int numPremiumJobs; //number of processed jobs sumbitted by Premium users
    double realSimulationTime; // real duration in seconds of the simulation run

    double responseTime[4]; // avg response time in the four centers
    double waitTime[4]; // avg wait time in the four centers
    int numDigestMatching; // number of jobs whose digest has matched
    double avgNumberOFJobs[4]; // E(N), average number of jobs in the four centers
    double interarrivalTime[4]; // 1/lambda for the 4 centers

    struct __stats *next; // pointer to the next struct in the list

}stats;
#endif