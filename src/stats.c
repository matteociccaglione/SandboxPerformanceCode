#include "stats.h"
#include "config.h"
#include "verify.h"

stats computeStatistics(digestCenter digestCenter, normalAnalysisCenter normalAnalysisCenter, premiumAnalysisCenter premiumAnalysisCenter, reliableAnalysisCenter reliableAnalysisCenter, machineLearningCenter mlCenter, char *filename, int runNumber, int simulationTime)
{
    stats statistics;
    statistics.realSimulationTime = simulationTime;                            // final value of the simulation clock
    statistics.numJobs = digestCenter.index;                                   // jobs processed by the entire system (all of them pass through the digest center)
    statistics.numNormalJobs = digestCenter.index - digestCenter.indexPremium; // jobs processed and submitted by normal users
    statistics.numPremiumJobs = digestCenter.indexPremium;                     // jobs processed and submitted by premium users

    // Output file for simulation statistics
    FILE *file = fopen(filename, "a+");
    fprintf(file, "%d,", runNumber);

    //  Digest center
    double digestResponseTime = digestCenter.area / digestCenter.index;
    double digestRo = (digestCenter.serviceArea / digestCenter.index) / (digestCenter.interarrivalTime / digestCenter.index);


    double delayArea = digestCenter.queueArea;

    // digestCenter.digestMatching, (double)digestCenter.digestMatching / digestCenter.index);
    if(digestCenter.index == 0){
        digestResponseTime = 0.0;
        digestRo = 0.0;
    }
    statistics.numDigestMatching = digestCenter.digestMatching;
    statistics.responseTime[0] = digestResponseTime;
    statistics.waitTime[0] = delayArea / digestCenter.index;
    statistics.serviceTime[0] = digestCenter.serviceArea / digestCenter.index;
    statistics.interarrivalTime[0] = digestCenter.interarrivalTime / digestCenter.index;
    statistics.avgNumberOFJobs[0] = digestCenter.area / digestCenter.interarrivalTime;
    statistics.ro[0] = digestRo;
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
    double normalResponseTime = normalAnalysisCenter.area / normalAnalysisCenter.index;
    delayArea = normalAnalysisCenter.area - normalAnalysisCenter.serviceArea;

    double meanServiceTime = normalAnalysisCenter.serviceArea;
    double rho = meanServiceTime / (normalAnalysisCenter.interarrivalTime * N_NORMAL);

    statistics.responseTime[1] = normalResponseTime;
    statistics.waitTime[1] = delayArea / normalAnalysisCenter.index;
    statistics.serviceTime[1] = normalAnalysisCenter.serviceArea / normalAnalysisCenter.index;
    statistics.interarrivalTime[1] = normalAnalysisCenter.interarrivalTime / normalAnalysisCenter.index;
    statistics.avgNumberOFJobs[1] = normalAnalysisCenter.area / normalAnalysisCenter.interarrivalTime;
    statistics.ro[1] = rho;
    statistics.numOfTimeouts[0] = normalAnalysisCenter.numberOfTimeouts;
    if(normalAnalysisCenter.index == 0){
        normalResponseTime = 0.0;
        rho = 0.0;
        statistics.responseTime[1] = 0.0;
        statistics.ro[1] = 0.0;
        statistics.waitTime[1] = 0.0;
        statistics.serviceTime[1] = 0.0;
        statistics.interarrivalTime[1] = 0.0;
    }
    if(normalAnalysisCenter.interarrivalTime == 0.0){
        statistics.avgNumberOFJobs[1] = 0.0;
    }


    // Premium analysis center
    double premiumResponseTime = premiumAnalysisCenter.area / premiumAnalysisCenter.index;
    delayArea = premiumAnalysisCenter.queueArea;
    meanServiceTime = premiumAnalysisCenter.serviceArea;
    double premiumRho = meanServiceTime / (premiumAnalysisCenter.interarrivalTime * N_PREMIUM);

    statistics.responseTime[2] = premiumResponseTime;
    statistics.ro[2] = premiumRho;
    statistics.waitTime[2] = delayArea / premiumAnalysisCenter.index;
    statistics.serviceTime[2] = premiumAnalysisCenter.serviceArea / premiumAnalysisCenter.index;
    statistics.interarrivalTime[2] = premiumAnalysisCenter.interarrivalTime / premiumAnalysisCenter.index;
    statistics.avgNumberOFJobs[2] = premiumAnalysisCenter.area / premiumAnalysisCenter.interarrivalTime;
    statistics.numOfTimeouts[1] = premiumAnalysisCenter.numberOfTimeouts;
    if (premiumAnalysisCenter.index == 0){
        premiumResponseTime = 0.0;
        premiumRho = 0.0;
        statistics.responseTime[2] = 0.0;
        statistics.ro[2] = 0.0;
        statistics.waitTime[2] = 0.0;
        statistics.serviceTime[2] = 0.0;
        statistics.interarrivalTime[2] = 0.0;
    }
    if(premiumAnalysisCenter.interarrivalTime == 0){
        statistics.avgNumberOFJobs[2] = 0.0;
    }
    

    // Reliable analysis center
    double reliableResponseTime = reliableAnalysisCenter.area / reliableAnalysisCenter.index;
    
    delayArea = reliableAnalysisCenter.queueArea;
    meanServiceTime = reliableAnalysisCenter.serviceArea;

    double reliableRho = meanServiceTime / (reliableAnalysisCenter.interarrivalTime * N_RELIABLE);
    
    statistics.ro[3] = reliableRho;
    statistics.responseTime[3] = reliableResponseTime;
    statistics.waitTime[3] = delayArea / reliableAnalysisCenter.index;
    statistics.serviceTime[3] = reliableAnalysisCenter.serviceArea / reliableAnalysisCenter.index;
    statistics.interarrivalTime[3] = reliableAnalysisCenter.interarrivalTime / reliableAnalysisCenter.index;
    statistics.avgNumberOFJobs[3] = reliableAnalysisCenter.area / reliableAnalysisCenter.interarrivalTime;
    statistics.numOfTimeouts[2] = reliableAnalysisCenter.numberOfTimeouts;
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
    statistics.ro[4] = mlCenter.serviceArea / (N_ML * mlCenter.interarrivalTime);
    statistics.numOfBypass = mlCenter.numOfBypass;
    statistics.bypassPercentage = (double)mlCenter.numOfBypass / (digestCenter.index - digestCenter.digestMatching);
    statistics.responseTime[4] = mlCenter.area / mlCenter.index;
    statistics.serviceTime[4] = mlCenter.serviceArea / mlCenter.index;
    statistics.avgNumberOFJobs[4] = mlCenter.area / mlCenter.interarrivalTime;
    statistics.interarrivalTime[4] = mlCenter.interarrivalTime / mlCenter.index;
    if (mlCenter.index == 0){
        statistics.ro[4] = 0.0;
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
    
    //double globalWaitTime = (digestCenter.queueArea + normalAnalysisCenter.queueArea + premiumAnalysisCenter.queueArea + reliableAnalysisCenter.queueArea) / digestCenter.index;
    //double globalPremiumWaitTime = (digestCenter.queueArea + premiumAnalysisCenter.queueArea + reliableAnalysisCenter.queueAreaPremium) / digestCenter.indexPremium;
    //double globalNormalWaitTime = (digestCenter.queueArea + normalAnalysisCenter.queueArea + reliableAnalysisCenter.queueAreaNormal) / (digestCenter.index - digestCenter.indexPremium);
    double globalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / digestCenter.index + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.index + reliableResponseTime * ((double)(premiumAnalysisCenter.numberOfTimeouts + normalAnalysisCenter.numberOfTimeouts) / digestCenter.index);


    double globalPremiumResponseTime = digestResponseTime + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.indexPremium + reliableResponseTime * (double)premiumAnalysisCenter.numberOfTimeouts / digestCenter.indexPremium;
    double globalNormalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / (digestCenter.index - digestCenter.indexPremium) + reliableResponseTime * (double)normalAnalysisCenter.numberOfTimeouts / (digestCenter.index - digestCenter.indexPremium);
    if (IMPROVEMENT)
    {
        double mlResponseTime = mlCenter.area / mlCenter.index;
        if (mlCenter.index == 0){
            mlResponseTime = 0.0;
        }
        
        globalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / digestCenter.index + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.index + reliableResponseTime * ((double)(premiumAnalysisCenter.numberOfTimeouts + normalAnalysisCenter.numberOfTimeouts) / digestCenter.index) + mlResponseTime * ((double)mlCenter.index / digestCenter.index);
        globalPremiumResponseTime = digestResponseTime + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.indexPremium + reliableResponseTime * (double)premiumAnalysisCenter.numberOfTimeouts / digestCenter.indexPremium + mlResponseTime * ((double)mlCenter.indexPremium / digestCenter.indexPremium);
        globalNormalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / (digestCenter.index - digestCenter.indexPremium) + reliableResponseTime * (double)normalAnalysisCenter.numberOfTimeouts / (digestCenter.index - digestCenter.indexPremium) + mlResponseTime * ((double)(mlCenter.index - mlCenter.indexPremium) / (digestCenter.index - digestCenter.indexPremium));
    }
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
                statistics.ro[0],
                statistics.ro[1],
                statistics.ro[2],
                statistics.ro[3],
                statistics.ro[4]);
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
                statistics.ro[0],
                statistics.ro[1],
                statistics.ro[2],
                statistics.ro[3]);
    }
    fclose(file);
    return statistics;
}