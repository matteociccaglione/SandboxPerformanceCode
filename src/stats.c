/**
 * @file stats.c
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-08-24
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file contains the implementation of the function used to compute simulation statistics,
 * starting from the recovered data of each center of the system.
 */
#include "stats.h"
#include "config.h"
#include "verify.h"

/**
 * @brief This function compute the mean values of all the needed statistics for a simulation run.
 * It stores them in a stats struct and also write them into a CSV file.
 * 
 * @param digestCenter digestCenter struct, containing data of the simulation for the center 
 * @param normalAnalysisCenter normalAnalysisCenter struct, containing data of the simulation for the center
 * @param premiumAnalysisCenter premiumAnalysisCenter struct, containing data of the simulation for the center
 * @param reliableAnalysisCenter digestCenter struct, containing data of the simulation for the center
 * @param mlCenter digestCenter struct, containing data of the simulation for the center
 * @param filename 
 * @param runNumber 
 * @param simulationTime 
 * @return stats 
 */
stats computeStatistics(digestCenter digestCenter, normalAnalysisCenter normalAnalysisCenter, premiumAnalysisCenter premiumAnalysisCenter, reliableAnalysisCenter reliableAnalysisCenter, machineLearningCenter mlCenter, char *filename, int runNumber, int simulationTime)
{
    stats statistics;
    statistics.realSimulationTime = simulationTime;                                                     // final value of the simulation clock
    statistics.numJobs = digestCenter.index;                                                            // jobs processed by the entire system (all of them pass through the digest center)
    statistics.numNormalJobs = digestCenter.index - digestCenter.indexPremium;                          // jobs processed and submitted by normal users
    statistics.numPremiumJobs = digestCenter.indexPremium;                                              // jobs processed and submitted by premium users

    // Output file for simulation statistics
    FILE *file = fopen(filename, "a+");
    fprintf(file, "%d,", runNumber);

    // Digest center stats
    double digestResponseTime = digestCenter.area / digestCenter.index;                                 // E(Ts)
    double digestRho = (digestCenter.serviceArea / digestCenter.index) / (digestCenter.interarrivalTime / digestCenter.index);   // rho = lambda * E(S)
    double delayArea = digestCenter.queueArea;

    if(digestCenter.index == 0){
        digestResponseTime = 0.0;
        digestRho = 0.0;
    }
    statistics.numDigestMatching = digestCenter.digestMatching;                                         // number of jobs whose digest has matched
    statistics.responseTime[0] = digestResponseTime;    
    statistics.waitTime[0] = delayArea / digestCenter.index;                                            // E(Tq)
    statistics.serviceTime[0] = digestCenter.serviceArea / digestCenter.index;                          // E(S)
    statistics.interarrivalTime[0] = digestCenter.interarrivalTime / digestCenter.index;                // avg interarrival time = sum of interarrival times / number of jobs
    statistics.avgNumberOFJobs[0] = digestCenter.area / digestCenter.interarrivalTime;                  // E(N) = lambda * E(Ts)
    statistics.rho[0] = digestRho;
    if (digestCenter.index == 0){
        statistics.waitTime[0] = 0.0;
        statistics.serviceTime[0] = 0.0;
        statistics.interarrivalTime[0] = 0.0;
    }
    if (digestCenter.interarrivalTime == 0.0)
    {
        statistics.avgNumberOFJobs[0] = 0.0;
    }
    
    

    // Normal analysis center
    double normalResponseTime = normalAnalysisCenter.area / normalAnalysisCenter.index;                 // E(Ts)
    delayArea = normalAnalysisCenter.queueArea;           

    double rho = normalAnalysisCenter.serviceArea / (normalAnalysisCenter.interarrivalTime * N_NORMAL); // rho = E(Si)/N * lambda

    statistics.responseTime[1] = normalResponseTime;
    statistics.waitTime[1] = delayArea / normalAnalysisCenter.index;                                    // E(Tq)
    statistics.serviceTime[1] = normalAnalysisCenter.serviceArea / normalAnalysisCenter.index;          // E(Si)
    statistics.interarrivalTime[1] = normalAnalysisCenter.interarrivalTime / normalAnalysisCenter.index;// avg interarrival time = sum of interarrival times / number of jobs
    statistics.avgNumberOFJobs[1] = normalAnalysisCenter.area / normalAnalysisCenter.interarrivalTime;  // E(N) = lambda * E(Ts)
    statistics.rho[1] = rho;
    statistics.numOfTimeouts[0] = normalAnalysisCenter.numberOfTimeouts;                                // number of jobs in timeout in normal analysis center
    if(normalAnalysisCenter.index == 0){
        normalResponseTime = 0.0;
        rho = 0.0;
        statistics.responseTime[1] = 0.0;
        statistics.rho[1] = 0.0;
        statistics.waitTime[1] = 0.0;
        statistics.serviceTime[1] = 0.0;
        statistics.interarrivalTime[1] = 0.0;
    }
    if(normalAnalysisCenter.interarrivalTime == 0.0){
        statistics.avgNumberOFJobs[1] = 0.0;
    }


    // Premium analysis center
    double premiumResponseTime = premiumAnalysisCenter.area / premiumAnalysisCenter.index;              // E(Ts)
    delayArea = premiumAnalysisCenter.queueArea;
    double premiumRho = premiumAnalysisCenter.serviceArea / (premiumAnalysisCenter.interarrivalTime * N_PREMIUM); // rho = E(Si)/N * lambda

    statistics.responseTime[2] = premiumResponseTime;
    statistics.rho[2] = premiumRho;
    statistics.waitTime[2] = delayArea / premiumAnalysisCenter.index;                                   // E(Tq)
    statistics.serviceTime[2] = premiumAnalysisCenter.serviceArea / premiumAnalysisCenter.index;        // E(Si)
    statistics.interarrivalTime[2] = premiumAnalysisCenter.interarrivalTime / premiumAnalysisCenter.index;// avg interarrival time = sum of interarrival times / number of jobs
    statistics.avgNumberOFJobs[2] = premiumAnalysisCenter.area / premiumAnalysisCenter.interarrivalTime;// E(N) = lambda * E(Ts)
    statistics.numOfTimeouts[1] = premiumAnalysisCenter.numberOfTimeouts;                               // number of jobs in timeout in premium analysis center
    if (premiumAnalysisCenter.index == 0){
        premiumResponseTime = 0.0;
        premiumRho = 0.0;
        statistics.responseTime[2] = 0.0;
        statistics.rho[2] = 0.0;
        statistics.waitTime[2] = 0.0;
        statistics.serviceTime[2] = 0.0;
        statistics.interarrivalTime[2] = 0.0;
    }
    if(premiumAnalysisCenter.interarrivalTime == 0){
        statistics.avgNumberOFJobs[2] = 0.0;
    }
    

    // Reliable analysis center
    double reliableResponseTime = reliableAnalysisCenter.area / reliableAnalysisCenter.index;           // E(Ts)
    delayArea = reliableAnalysisCenter.queueArea;
    double reliableRho = reliableAnalysisCenter.serviceArea / (reliableAnalysisCenter.interarrivalTime * N_RELIABLE);   // rho = E(Si)/N * lambda
    
    statistics.rho[3] = reliableRho;
    statistics.responseTime[3] = reliableResponseTime;          
    statistics.waitTime[3] = delayArea / reliableAnalysisCenter.index;                                  // E(Tq)
    statistics.serviceTime[3] = reliableAnalysisCenter.serviceArea / reliableAnalysisCenter.index;      // E(Si)
    statistics.interarrivalTime[3] = reliableAnalysisCenter.interarrivalTime / reliableAnalysisCenter.index;// avg interarrival time = sum of interarrival times / number of jobs
    statistics.avgNumberOFJobs[3] = reliableAnalysisCenter.area / reliableAnalysisCenter.interarrivalTime;  // E(N) = lambda * E(Ts)
    statistics.numOfTimeouts[2] = reliableAnalysisCenter.numberOfTimeouts;                              // number of jobs in timeout in reliable analysis center
    if (reliableAnalysisCenter.index == 0){
        reliableResponseTime = 0.0;
        reliableRho = 0.0;
        statistics.responseTime[3] = 0.0;
        statistics.waitTime[3] = 0.0;
        statistics.serviceTime[3] = 0.0;
        statistics.interarrivalTime[3] = 0.0;
    }
    if (reliableAnalysisCenter.interarrivalTime == 0.0){
        statistics.avgNumberOFJobs[3] = 0.0;
    }       


    // ML performances
    statistics.rho[4] = mlCenter.serviceArea / (N_ML * mlCenter.interarrivalTime);                      // rho = E(Si)/N * lambda
    statistics.numOfBypass = mlCenter.numOfBypass;                                                      // number of jobs that bypassed the ML service
    statistics.bypassPercentage = (double)mlCenter.numOfBypass / (digestCenter.index - digestCenter.digestMatching);    // percentage of bypass
    statistics.responseTime[4] = mlCenter.area / mlCenter.index;                                        // E(Ts)
    statistics.serviceTime[4] = mlCenter.serviceArea / mlCenter.index;                                  // E(Si)
    statistics.avgNumberOFJobs[4] = mlCenter.area / mlCenter.interarrivalTime;                          // E(N) = lambda * E(Ts)
    statistics.interarrivalTime[4] = mlCenter.interarrivalTime / mlCenter.index;                        // avg interarrival time = sum of interarrival times / number of jobs
    if (mlCenter.index == 0){
        statistics.rho[4] = 0.0;
        statistics.numOfBypass = 0;
        statistics.responseTime[4] = 0.0;
        statistics.serviceTime[4] = 0.0;
        statistics.interarrivalTime[4] = 0.0;
    }
    if (mlCenter.interarrivalTime == 0.0){
        statistics.avgNumberOFJobs[4] = 0.0;
    }
    
    if (digestCenter.index - digestCenter.digestMatching == 0){
        statistics.bypassPercentage = 0.0;
    }
    


    // Global performances
    // E(Ts) = SUM {Vi * E(Ts,i)}, where Vi are the visits to the center i
    double globalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / digestCenter.index + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.index + reliableResponseTime * ((double)(premiumAnalysisCenter.numberOfTimeouts + normalAnalysisCenter.numberOfTimeouts) / digestCenter.index);
    double globalPremiumResponseTime = digestResponseTime + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.indexPremium + reliableResponseTime * (double)premiumAnalysisCenter.numberOfTimeouts / digestCenter.indexPremium;
    double globalNormalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / (digestCenter.index - digestCenter.indexPremium) + reliableResponseTime * (double)normalAnalysisCenter.numberOfTimeouts / (digestCenter.index - digestCenter.indexPremium);
    
    if (IMPROVEMENT)
    {
        double mlResponseTime = mlCenter.area / mlCenter.index;                                         // E(Ts)
        if (mlCenter.index == 0){
            mlResponseTime = 0.0;
        }
        
        // recompute global response times, taking into account the added center in the improved system
        globalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / digestCenter.index + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.index + reliableResponseTime * ((double)(premiumAnalysisCenter.numberOfTimeouts + normalAnalysisCenter.numberOfTimeouts) / digestCenter.index) + mlResponseTime * ((double)mlCenter.index / digestCenter.index);
        globalPremiumResponseTime = digestResponseTime + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.indexPremium + reliableResponseTime * (double)premiumAnalysisCenter.numberOfTimeouts / digestCenter.indexPremium + mlResponseTime * ((double)mlCenter.indexPremium / digestCenter.indexPremium);
        globalNormalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / (digestCenter.index - digestCenter.indexPremium) + reliableResponseTime * (double)normalAnalysisCenter.numberOfTimeouts / (digestCenter.index - digestCenter.indexPremium) + mlResponseTime * ((double)(mlCenter.index - mlCenter.indexPremium) / (digestCenter.index - digestCenter.indexPremium));
    }

    // The percentage of failure is the number of jobs that exited the system with a timeout among all the jobs served by the system
    double percentageFailure = (double)reliableAnalysisCenter.numberOfTimeouts / digestCenter.index;
    
    statistics.globalResponseTime = globalResponseTime;
    statistics.globalPremiumResponseTime = globalPremiumResponseTime;
    statistics.globalFailurePercentage = percentageFailure;
    statistics.globalNormalResponseTime = globalNormalResponseTime;

    if (FINITE_HORIZON){
        // verification implies average calculations over runs to match theretical values
        // so it does not make sense for infinite horizon simulation (one long run), because it should be evaluated at each batch
        verify(&digestCenter, &normalAnalysisCenter, &premiumAnalysisCenter, &reliableAnalysisCenter, &mlCenter);
    }

    // Write results in output CSV file
    if (IMPROVEMENT)
    {
        fprintf(file, "%d,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f\n",
                statistics.numDigestMatching,
                statistics.serviceTime[0],
                statistics.serviceTime[1],
                statistics.serviceTime[2],
                statistics.serviceTime[3],
                statistics.serviceTime[4],
                statistics.responseTime[0],
                statistics.responseTime[1],
                statistics.responseTime[2],
                statistics.responseTime[3],
                statistics.responseTime[4],
                statistics.waitTime[0],
                statistics.waitTime[1],
                statistics.waitTime[2],
                statistics.waitTime[3],
                statistics.interarrivalTime[0],
                statistics.interarrivalTime[1],
                statistics.interarrivalTime[2],
                statistics.interarrivalTime[3],
                statistics.interarrivalTime[4],
                statistics.avgNumberOFJobs[0],
                statistics.avgNumberOFJobs[1],
                statistics.avgNumberOFJobs[2],
                statistics.avgNumberOFJobs[3],
                statistics.avgNumberOFJobs[4],
                statistics.numOfTimeouts[0],
                statistics.numOfTimeouts[1],
                statistics.numOfTimeouts[2],
                statistics.numOfBypass,
                statistics.globalResponseTime,
                statistics.globalPremiumResponseTime,
                statistics.globalNormalResponseTime,
                statistics.globalFailurePercentage,
                statistics.rho[0],
                statistics.rho[1],
                statistics.rho[2],
                statistics.rho[3],
                statistics.rho[4]);
    }
    else
    {
        fprintf(file, "%d,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f\n",
                statistics.numDigestMatching,
                statistics.serviceTime[0],
                statistics.serviceTime[1],
                statistics.serviceTime[2],
                statistics.serviceTime[3],
                statistics.responseTime[0],
                statistics.responseTime[1],
                statistics.responseTime[2],
                statistics.responseTime[3],
                statistics.waitTime[0],
                statistics.waitTime[1],
                statistics.waitTime[2],
                statistics.waitTime[3],
                statistics.interarrivalTime[0],
                statistics.interarrivalTime[1],
                statistics.interarrivalTime[2],
                statistics.interarrivalTime[3],
                statistics.avgNumberOFJobs[0],
                statistics.avgNumberOFJobs[1],
                statistics.avgNumberOFJobs[2],
                statistics.avgNumberOFJobs[3],
                statistics.numOfTimeouts[0],
                statistics.numOfTimeouts[1],
                statistics.numOfTimeouts[2],
                statistics.globalResponseTime,
                statistics.globalPremiumResponseTime,
                statistics.globalNormalResponseTime,
                statistics.globalFailurePercentage,
                statistics.rho[0],
                statistics.rho[1],
                statistics.rho[2],
                statistics.rho[3]);
    }
    fclose(file);
    return statistics;
}