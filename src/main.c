#include "events_queue.h"
#include "../lib/rngs.h"
#include "../lib/rvgs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stats.h"
#include "../lib/rvms.h"
#include "handle_events.h"
#include "verify.h"
#include "estimations.h"

#define START 0.0
double simulationTime = START;

/**
 * @brief This function is used to determine if the list of the events is empty,
 * in order to stop the simulation run.
 *
 * @param ev The event_list struct containing the events
 * @return int 0 if there are still events to be processed; 1 otherwise (if the lists are empty)
 */
int isEmptyList(event_list ev)
{
    if (ev.arrivals == NULL && ev.terminations == NULL)
        return 1;
    return 0;
}


void verify(digestCenter *digestCenter, normalAnalysisCenter *normalCenter, premiumAnalysisCenter *premiumCenter, reliableAnalysisCenter *reliableCenter, machineLearningCenter *mlCenter);

digestCenter initializeDigest()
{
    // Digest center initialization
    digestCenter digestCenter;
    digestCenter.area = 0.0;
    digestCenter.queueArea = 0.0;
    digestCenter.digestMatching = 0;
    digestCenter.lastEventTime = 0.0;
    digestCenter.index = 0;
    digestCenter.indexPremium = 0;
    digestCenter.jobs = 0;
    digestCenter.jobsInQueue = 0;
    digestCenter.queue = NULL;
    digestCenter.probabilityOfMatching = INITIAL_DIGEST_MATCHING_PROB;
    digestCenter.interarrivalTime = 0.0;
    digestCenter.serviceArea = 0.0;
    return digestCenter;
}
premiumAnalysisCenter initializePremium()
{
    premiumAnalysisCenter premiumAnalysisCenter;
    premiumAnalysisCenter.area = 0.0;
    premiumAnalysisCenter.queueArea = 0.0;
    premiumAnalysisCenter.numberOfTimeouts = 0;
    premiumAnalysisCenter.lastEventTime = 0.0;
    premiumAnalysisCenter.index = 0;
    premiumAnalysisCenter.jobs = 0;
    premiumAnalysisCenter.jobsInQueue = 0;
    initializeServerArray(premiumAnalysisCenter.servers, N_PREMIUM);
    premiumAnalysisCenter.queue = NULL;
    premiumAnalysisCenter.interarrivalTime = 0.0;
    premiumAnalysisCenter.lastArrivalTime = 0.0;
    premiumAnalysisCenter.serviceArea = 0.0;
    return premiumAnalysisCenter;
}

normalAnalysisCenter initializeNormal()
{
    normalAnalysisCenter normalAnalysisCenter;
    // Normal center initialization
    normalAnalysisCenter.area = 0.0;
    normalAnalysisCenter.queueArea = 0.0;
    normalAnalysisCenter.numberOfTimeouts = 0;
    normalAnalysisCenter.lastEventTime = 0.0;
    normalAnalysisCenter.index = 0;
    normalAnalysisCenter.jobs = 0;
    normalAnalysisCenter.jobsInQueue = 0;
    initializeServerArray(normalAnalysisCenter.servers, N_NORMAL);
    normalAnalysisCenter.queue = NULL;
    normalAnalysisCenter.interarrivalTime = 0.0;
    normalAnalysisCenter.lastArrivalTime = 0.0;
    normalAnalysisCenter.serviceArea = 0.0;
    return normalAnalysisCenter;
}

reliableAnalysisCenter initializeReliable()
{
    reliableAnalysisCenter reliableAnalysisCenter;
    // Reliable center initialization
    reliableAnalysisCenter.area = 0.0;
    reliableAnalysisCenter.areaNormal = 0.0;
    reliableAnalysisCenter.areaPremium = 0.0;
    reliableAnalysisCenter.queueArea = 0.0;
    reliableAnalysisCenter.queueAreaNormal = 0.0;
    reliableAnalysisCenter.queueAreaPremium = 0.0;
    reliableAnalysisCenter.jobs = 0;
    reliableAnalysisCenter.jobsInQueueNormal = 0;
    reliableAnalysisCenter.jobsInQueuePremium = 0;
    reliableAnalysisCenter.premiumJobs = 0;
    reliableAnalysisCenter.numberOfTimeouts = 0;
    reliableAnalysisCenter.lastEventTime = 0.0;
    reliableAnalysisCenter.index = 0;
    reliableAnalysisCenter.premiumIndex = 0;
    initializeServerArray(reliableAnalysisCenter.servers, N_RELIABLE);
    reliableAnalysisCenter.queueNormal = NULL;
    reliableAnalysisCenter.queuePremium = NULL;
    reliableAnalysisCenter.lastEventTimePremium = 0.0;
    reliableAnalysisCenter.interarrivalTime = 0.0;
    reliableAnalysisCenter.lastArrivalTime = 0.0;
    reliableAnalysisCenter.lastEventTimeNormal = 0.0;
    reliableAnalysisCenter.normalJobs = 0;
    reliableAnalysisCenter.normalIndex = 0;
    reliableAnalysisCenter.serviceArea = 0.0;
    reliableAnalysisCenter.serviceAreaNormal = 0.0;
    reliableAnalysisCenter.serviceAreaPremium = 0.0;
    reliableAnalysisCenter.jobAnalyzed = 0;
    return reliableAnalysisCenter;
}
machineLearningCenter initializeMl()
{
    machineLearningCenter ml;
    ml.jobs = 0;
    ml.index = 0;
    ml.indexPremium = 0;
    ml.interarrivalTime = 0.0;
    ml.lastEventTime = 0.0;
    ml.mlSuccess = 0;
    ml.area = 0.0;
    ml.serviceArea = 0.0;
    ml.numOfBypass = 0;
    ml.lastArrivalTime = 0.0;
    return ml;
}
event_list handleArrival(digestCenter *digestCenter, normalAnalysisCenter *normalAnalysisCenter, premiumAnalysisCenter *premiumAnalysisCenter, reliableAnalysisCenter *reliableAnalysisCenter, machineLearningCenter *mlCenter, event_list events)
{
    switch (events.arrivals->center)
    {
        // Before calling the appropriate function to handle the event, update areas used to calculate statistics
        // Areas to be updated : time-integrated number of jobs in center, time-integrated number of jobs in queue, time-integrated number of jobs in service
    case CENTER_DIGEST:
        digestCenter->area += (events.arrivals->time - digestCenter->lastEventTime) * digestCenter->jobs;
        digestCenter->serviceArea += (events.arrivals->time - digestCenter->lastEventTime) * (digestCenter->jobs - digestCenter->jobsInQueue);
        digestCenter->queueArea += (events.arrivals->time - digestCenter->lastEventTime) * digestCenter->jobsInQueue;
        simulationTime = handleDigestArrival(digestCenter, &events, simulationTime);
        break;
    case CENTER_NORMAL:
        normalAnalysisCenter->area += (events.arrivals->time - normalAnalysisCenter->lastEventTime) * normalAnalysisCenter->jobs;
        normalAnalysisCenter->serviceArea += (events.arrivals->time - normalAnalysisCenter->lastEventTime) * (normalAnalysisCenter->jobs - normalAnalysisCenter->jobsInQueue);
        normalAnalysisCenter->queueArea += (events.arrivals->time - normalAnalysisCenter->lastEventTime) * normalAnalysisCenter->jobsInQueue;
        simulationTime = handleNormalArrival(normalAnalysisCenter, &events, simulationTime);
        break;
    case CENTER_PREMIUM:
        premiumAnalysisCenter->area += (events.arrivals->time - premiumAnalysisCenter->lastEventTime) * premiumAnalysisCenter->jobs;
        premiumAnalysisCenter->serviceArea += (events.arrivals->time - premiumAnalysisCenter->lastEventTime) * (premiumAnalysisCenter->jobs - premiumAnalysisCenter->jobsInQueue);
        premiumAnalysisCenter->queueArea += (events.arrivals->time - premiumAnalysisCenter->lastEventTime) * (premiumAnalysisCenter->jobsInQueue);
        simulationTime = handlePremiumArrival(premiumAnalysisCenter, &events, simulationTime);
        break;
    case CENTER_RELIABLE:
        reliableAnalysisCenter->area += (events.arrivals->time - reliableAnalysisCenter->lastEventTime) * reliableAnalysisCenter->jobs;
        reliableAnalysisCenter->serviceArea += (events.arrivals->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->jobs - (reliableAnalysisCenter->jobsInQueueNormal + reliableAnalysisCenter->jobsInQueuePremium));
        reliableAnalysisCenter->queueArea += (events.arrivals->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->jobsInQueueNormal + reliableAnalysisCenter->jobsInQueuePremium);
        reliableAnalysisCenter->areaPremium += (events.arrivals->time - reliableAnalysisCenter->lastEventTime) * reliableAnalysisCenter->premiumJobs;
        reliableAnalysisCenter->serviceAreaPremium += (events.arrivals->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->premiumJobs - reliableAnalysisCenter->jobsInQueuePremium);
        reliableAnalysisCenter->queueAreaPremium += (events.arrivals->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->jobsInQueuePremium);
        reliableAnalysisCenter->areaNormal += (events.arrivals->time - reliableAnalysisCenter->lastEventTime) * reliableAnalysisCenter->normalJobs;
        reliableAnalysisCenter->queueAreaNormal += (events.arrivals->time - reliableAnalysisCenter->lastEventTime) * reliableAnalysisCenter->jobsInQueueNormal;
        reliableAnalysisCenter->serviceAreaNormal += (events.arrivals->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->normalJobs - reliableAnalysisCenter->jobsInQueueNormal);
        simulationTime = handleReliableArrival(reliableAnalysisCenter, &events, simulationTime);
        break;
    case CENTER_ML:
        if (mlCenter->jobs < N_ML)
        {
            mlCenter->area += (events.arrivals->time - mlCenter->lastEventTime) * mlCenter->jobs;
            mlCenter->serviceArea += (events.arrivals->time - mlCenter->lastEventTime) * mlCenter->jobs;
        }
        simulationTime = handleMachineLearningArrival(mlCenter, &events, simulationTime);
    }
    return events;
}

event_list handleTermination(digestCenter *digestCenter, normalAnalysisCenter *normalAnalysisCenter, premiumAnalysisCenter *premiumAnalysisCenter, reliableAnalysisCenter *reliableAnalysisCenter, machineLearningCenter *mlCenter, event_list events)
{
    switch (events.terminations->center)
    {
        // Before calling the appropriate function to handle the event, update areas used to calculate statistics
        // Areas to be updated : time-integrated number of jobs in center, time-integrated number of jobs in queue, time-integrated number of jobs in service
    case CENTER_DIGEST:
        digestCenter->area += (events.terminations->time - digestCenter->lastEventTime) * digestCenter->jobs;
        digestCenter->serviceArea += (events.terminations->time - digestCenter->lastEventTime) * (digestCenter->jobs - digestCenter->jobsInQueue);
        digestCenter->queueArea += (events.terminations->time - digestCenter->lastEventTime) * digestCenter->jobsInQueue;
        simulationTime = handleDigestTermination(digestCenter, &events, simulationTime);
        break;
    case CENTER_NORMAL:
        normalAnalysisCenter->area += (events.terminations->time - normalAnalysisCenter->lastEventTime) * normalAnalysisCenter->jobs;
        normalAnalysisCenter->serviceArea += (events.terminations->time - normalAnalysisCenter->lastEventTime) * (normalAnalysisCenter->jobs - normalAnalysisCenter->jobsInQueue);
        normalAnalysisCenter->queueArea += (events.terminations->time - normalAnalysisCenter->lastEventTime) * normalAnalysisCenter->jobsInQueue;
        simulationTime = handleNormalTermination(normalAnalysisCenter, &events, digestCenter, simulationTime);
        break;
    case CENTER_PREMIUM:
        premiumAnalysisCenter->area += (events.terminations->time - premiumAnalysisCenter->lastEventTime) * premiumAnalysisCenter->jobs;
        premiumAnalysisCenter->serviceArea += (events.terminations->time - premiumAnalysisCenter->lastEventTime) * (premiumAnalysisCenter->jobs - premiumAnalysisCenter->jobsInQueue);
        premiumAnalysisCenter->queueArea += (events.terminations->time - premiumAnalysisCenter->lastEventTime) * (premiumAnalysisCenter->jobsInQueue);
        simulationTime = handlePremiumTermination(premiumAnalysisCenter, &events, digestCenter, simulationTime);
        break;
    case CENTER_RELIABLE:
        reliableAnalysisCenter->area += (events.terminations->time - reliableAnalysisCenter->lastEventTime) * reliableAnalysisCenter->jobs;
        reliableAnalysisCenter->serviceArea += (events.terminations->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->jobs - (reliableAnalysisCenter->jobsInQueueNormal + reliableAnalysisCenter->jobsInQueuePremium));
        reliableAnalysisCenter->queueArea += (events.terminations->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->jobsInQueueNormal + reliableAnalysisCenter->jobsInQueuePremium);
        reliableAnalysisCenter->areaPremium += (events.terminations->time - reliableAnalysisCenter->lastEventTime) * reliableAnalysisCenter->premiumJobs;
        reliableAnalysisCenter->serviceAreaPremium += (events.terminations->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->premiumJobs - reliableAnalysisCenter->jobsInQueuePremium);
        reliableAnalysisCenter->queueAreaPremium += (events.terminations->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->jobsInQueuePremium);
        reliableAnalysisCenter->areaNormal += (events.terminations->time - reliableAnalysisCenter->lastEventTime) * reliableAnalysisCenter->normalJobs;
        reliableAnalysisCenter->queueAreaNormal += (events.terminations->time - reliableAnalysisCenter->lastEventTime) * reliableAnalysisCenter->jobsInQueueNormal;
        reliableAnalysisCenter->serviceAreaNormal += (events.terminations->time - reliableAnalysisCenter->lastEventTime) * (reliableAnalysisCenter->normalJobs - reliableAnalysisCenter->jobsInQueueNormal);
        simulationTime = handleReliableTermination(reliableAnalysisCenter, &events, digestCenter, simulationTime);
        break;
    case CENTER_ML:
        mlCenter->area += (events.terminations->time - mlCenter->lastEventTime) * mlCenter->jobs;
        mlCenter->serviceArea += (events.terminations->time - mlCenter->lastEventTime) * mlCenter->jobs;
        simulationTime = handleMachineLearningTermination(mlCenter, &events, simulationTime);
    }
    return events;
}



stats *infiniteHorizonSimulation(int batchNumber, int batchSize, char *filename)
{
    int nBatch = 0;
    int jobsInBatch = 0;
    simulationTime = START;
    double batchTime = 0.0;                                               // set the initial value of the simulation clock
    event_list events;                                                    // struct that contains the event lists of the simulation
    digestCenter digestCenter = initializeDigest();                       // struct containing info on the Digest Center during the simulation
    normalAnalysisCenter normalAnalysisCenter = initializeNormal();       // struct containing info on the Normal Analysis Center during the simulation
    premiumAnalysisCenter premiumAnalysisCenter = initializePremium();    // struct containing info on the Premium Analysis Center during the simulation
    reliableAnalysisCenter reliableAnalysisCenter = initializeReliable(); // struct containing info on the Reliable Analysis Center during the simulation
    machineLearningCenter mlCenter = initializeMl();
    // Initialize event lists
    events.arrivals = NULL;               // list of arrivals
    events.terminations = NULL;           // list of terminations
    PlantSeeds(123456789);                // seeds for RNGS
    insertList(&events, getArrival(simulationTime), 0); // generate the first arrival and put it in the list of events
    stats *allStatistics = malloc(sizeof(stats) * batchNumber);
    while (nBatch < batchNumber)
    {
        if (nextEvent(events) == 0)
        {         
            if (events.arrivals->center == CENTER_DIGEST)
            {
                jobsInBatch++;
            }
                      
            // distinguish the center that has to process the arrival event
            events = handleArrival(&digestCenter, &normalAnalysisCenter, &premiumAnalysisCenter, &reliableAnalysisCenter, &mlCenter, events);

            if (jobsInBatch == batchSize)
            {
                // Compute statistics
                allStatistics[nBatch] = computeStatistics(digestCenter, normalAnalysisCenter, premiumAnalysisCenter, reliableAnalysisCenter, mlCenter, filename, nBatch, simulationTime - batchTime);
                // Reset center values
                digestCenter.index = 0;
                digestCenter.indexPremium = 0;
                digestCenter.area = 0.0;
                digestCenter.queueArea = 0.0;
                digestCenter.serviceArea = 0.0;
                digestCenter.digestMatching = 0;
                digestCenter.interarrivalTime = 0.0;
                normalAnalysisCenter.index = 0;
                normalAnalysisCenter.area = 0.0;
                normalAnalysisCenter.queueArea = 0.0;
                normalAnalysisCenter.serviceArea = 0.0;
                normalAnalysisCenter.numberOfTimeouts = 0;
                normalAnalysisCenter.interarrivalTime = 0.0;
                premiumAnalysisCenter.interarrivalTime = 0.0;
                premiumAnalysisCenter.index = 0;
                premiumAnalysisCenter.area = 0.0;
                premiumAnalysisCenter.serviceArea = 0.0;
                premiumAnalysisCenter.queueArea = 0.0;
                premiumAnalysisCenter.numberOfTimeouts = 0;
                reliableAnalysisCenter.index = 0;
                reliableAnalysisCenter.premiumIndex = 0;
                reliableAnalysisCenter.normalIndex = 0;
                reliableAnalysisCenter.numberOfTimeouts = 0;
                reliableAnalysisCenter.area = 0.0;
                reliableAnalysisCenter.areaNormal = 0.0;
                reliableAnalysisCenter.areaPremium = 0.0;
                reliableAnalysisCenter.queueArea = 0.0;
                reliableAnalysisCenter.queueAreaNormal = 0.0;
                reliableAnalysisCenter.queueAreaPremium = 0.0;
                reliableAnalysisCenter.serviceArea = 0.0;
                reliableAnalysisCenter.serviceAreaNormal = 0.0;
                reliableAnalysisCenter.serviceAreaPremium = 0.0;
                reliableAnalysisCenter.interarrivalTime = 0.0;

                if (IMPROVEMENT){
                    mlCenter.index = 0;
                    mlCenter.indexPremium = 0;
                    mlCenter.area = 0.0;
                    mlCenter.serviceArea = 0.0;
                    mlCenter.mlSuccess = 0;
                    mlCenter.numOfBypass = 0;
                    mlCenter.interarrivalTime = 0.0;
                }
                

                if ((nBatch + 1)%5 == 0 || (nBatch + 1) == batchNumber){
                    printf("Batch %d DONE\n", nBatch+1);
                }

                nBatch++;
                batchTime = simulationTime;
                jobsInBatch = 0;
            }
            
        }
        else
        {
            // Next event is a termination
            events = handleTermination(&digestCenter, &normalAnalysisCenter, &premiumAnalysisCenter, &reliableAnalysisCenter, &mlCenter, events);
        }
    }
    return allStatistics;
}

/**
 * @brief This function is used to do a single simulation run of the system.
 *
 * @param runNumber The progressive ID of the simulation run
 * @param filename Filename of the file where to save statistics
 * @return stats Returns a struct containing recovered statistics through the simulation run
 */
stats oneTimeSimulation(int runNumber, char *filename)
{

    // Initial setup
    simulationTime = START;                                               // set the initial value of the simulation clock
    double sampleTime = START;                                            // local variable used to periodically sample response times
    double *sampleResponseTime = malloc(320 * sizeof(double));
    int sampleIndex = 0;
    event_list events;                                                    // struct that contains the event lists of the simulation
    digestCenter digestCenter = initializeDigest();                       // struct containing info on the Digest Center during the simulation
    normalAnalysisCenter normalAnalysisCenter = initializeNormal();       // struct containing info on the Normal Analysis Center during the simulation
    premiumAnalysisCenter premiumAnalysisCenter = initializePremium();    // struct containing info on the Premium Analysis Center during the simulation
    reliableAnalysisCenter reliableAnalysisCenter = initializeReliable(); // struct containing info on the Reliable Analysis Center during the simulation
    machineLearningCenter mlCenter = initializeMl();
    // Initialize event lists
    events.arrivals = NULL;               // list of arrivals
    events.terminations = NULL;           // list of terminations
    PlantSeeds(123456789 + runNumber);    // seeds for RNGS
    insertList(&events, getArrival(simulationTime), 0); // generate the first arrival and put it in the list of events

    // Run the simulation until the simulation time is greater or equal of the defined observation period.
    // If so, continue the simulation until the lists of events are completely processed and become empty
    while (simulationTime < OBSERVATION_PERIOD || !(isEmptyList(events)))
    {
        double eventTime;
        if(nextEvent(events) == 0){
            eventTime = events.arrivals->time;
        }else{
            eventTime = events.terminations->time;
        }

        // sample response times every 5 minutes
        if (eventTime - sampleTime > 5*60){
            double digestResponseTime = digestCenter.area/digestCenter.index;
            double normalResponseTime = normalAnalysisCenter.area/normalAnalysisCenter.index;
            double premiumResponseTime = premiumAnalysisCenter.area/premiumAnalysisCenter.index;
            double reliableResponseTime = reliableAnalysisCenter.area/reliableAnalysisCenter.index;
            if (digestCenter.index == 0){
                digestResponseTime = 0.0;
            }
            if (normalAnalysisCenter.index == 0){
                normalResponseTime = 0.0;
            }
            if (premiumAnalysisCenter.index == 0){
                premiumResponseTime = 0.0;
            }
            if (reliableAnalysisCenter.index == 0){
                reliableResponseTime = 0.0;
            }
            

            double globalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / digestCenter.index + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.index + reliableResponseTime * ((double)(premiumAnalysisCenter.numberOfTimeouts + normalAnalysisCenter.numberOfTimeouts) / digestCenter.index);
            if (IMPROVEMENT){
                double mlResponseTime = mlCenter.area / mlCenter.index;
                if (mlCenter.index == 0){
                    mlResponseTime = 0.0;
                }
                globalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / digestCenter.index + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.index + reliableResponseTime * ((double)(premiumAnalysisCenter.numberOfTimeouts + normalAnalysisCenter.numberOfTimeouts) / digestCenter.index) + mlResponseTime * ((double)mlCenter.index / digestCenter.index);
            }          

            sampleResponseTime[sampleIndex] = globalResponseTime;
            sampleIndex++;
            sampleTime = eventTime;
        }

        // next event is an arrival
        if (nextEvent(events) == 0)
        {
            // distinguish the center that has to process the arrival event
            events = handleArrival(&digestCenter, &normalAnalysisCenter, &premiumAnalysisCenter, &reliableAnalysisCenter, &mlCenter, events);
            
        }
        else
        {
            // Next event is a termination
            events = handleTermination(&digestCenter, &normalAnalysisCenter, &premiumAnalysisCenter, &reliableAnalysisCenter, &mlCenter, events);
        }
    }

    // struct used to contain statistics
    stats statistics = computeStatistics(digestCenter, normalAnalysisCenter, premiumAnalysisCenter, reliableAnalysisCenter, mlCenter, filename, runNumber, simulationTime);
    statistics.samplesResponseTime = sampleResponseTime;
    statistics.sampleArraySize = sampleIndex + 1;
    return statistics;
}

void writeCSVLine(FILE *file, char *statName, char *actualValue)
{
    fprintf(file, "%s, %s, %s\n",
            statName,
            "",
            actualValue);
}

/**
 * @brief The main function. It starts the simulation.
 *
 * @return int
 */
int main()
{
    char *centerNames[5] = {"digest", "normal", "premium", "reliable", "ml"};
    FILE *f = fopen("simulation_stats.csv", "w+");
    FILE *estimations = fopen("interval_estimation.csv", "w+");
    fprintf(estimations, "Statistic, Analytical result, Experimental result\n");
    stats statistics[ITERATIONS];
    int nCenters = 4;
    if (IMPROVEMENT)
        nCenters = 5;
    if (IMPROVEMENT)
    {
        fprintf(f, "#RUN,Digest Matching, Service time Digest, Service time Normal, Service time Premium, Service time Reliable,Service time ML,Response time Digest, Response time Normal, Response time Premium, Response time Reliable, Response time ML, Wait time Digest, Wait time Normal, Wait time Premium, Wait time Reliable,Interarrival time Digest, Interarrival time Normal, Interarrival time Premium, Interarrival time Reliable,Interarrival time ML, Avg num of jobs Digest, Avg num of jobs Normal, Avg num of jobs Premium, Avg num of jobs Reliable, Avg num of jobs ML, Num of timeouts Normal, Num of timeouts Premium, Num of timeouts Reliable, Num of bypass, Global Response Time, Global Premium Response Time, Global Normal Response Time, Percentage of Failure, Rho Digest, Rho Normal, Rho Premium, Rho Reliable, Rho ML\n");
    }
    else
        fprintf(f, "#RUN,Digest Matching, Service time Digest, Service time Normal, Service time Premium, Service time Reliable,Response time Digest, Response time Normal, Response time Premium, Response time Reliable, Wait time Digest, Wait time Normal, Wait time Premium, Wait time Reliable,Interarrival time Digest, Interarrival time Normal, Interarrival time Premium, Interarrival time Reliable, Avg num of jobs Digest, Avg num of jobs Normal, Avg num of jobs Premium, Avg num of jobs Reliable, Num of timeouts Normal, Num of timeouts Premium, Num of timeouts Reliable, Global Response Time, Global Premium Response Time, Global Normal Response Time, Percentage of Failure, Rho Digest, Rho Normal, Rho Premium, Rho Reliable\n");
    fclose(f);
    if (FINITE_HORIZON)
    {
        printf("Finite Horizon simulation with %d runs\n\n", ITERATIONS);
        for (int i = 0; i < ITERATIONS; i++)
        {
            statistics[i] = oneTimeSimulation(i, "simulation_stats.csv");
            if ((i + 1) % 10 == 0 || i + 1 == ITERATIONS)
            {
                printf("Run %d DONE\n", i + 1);
            }
        }

        // Calculate mean values of the statistics recovered through the ITERATIONS runs
        double confidence = 0.95;
        double array[ITERATIONS];
        double *results;
        double **confidenceIntervals = malloc(sizeof(double *) * 27);
        char *actualValue = malloc(60 * sizeof(char));
        char *statName = malloc(100 * sizeof(char));
        printf("\n\nConfidence intervals:\n\n");
        // num jobs
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].numJobs;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[0] = results;
        printf("Num jobs processed : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(estimations, "Number of processed jobs", actualValue);
        free(results);

        // num normal jobs
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].numNormalJobs;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[1] = results;
        printf("Num normal jobs processed : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(estimations, "Number of processed normal jobs", actualValue);
        free(results);

        // num premium jobs
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].numPremiumJobs;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[2] = results;
        printf("Num premium jobs processed : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(estimations, "Number of processed premium jobs", actualValue);
        free(results);

        // num digest Matching
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].numDigestMatching;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[3] = results;
        printf("Num digest matching : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(estimations, "Number of matching digests", actualValue);
        free(results);

        // response times
        for (int j = 0; j < nCenters; j++)
        {
            for (int i = 0; i < ITERATIONS; i++)
            {
                array[i] = (double)statistics[i].responseTime[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Response time center %d : %6.6f +/- %6.6f sec\n", j + 1, results[0], results[1]);
            sprintf(statName, "Response time %s center", centerNames[j]);
            sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
            writeCSVLine(estimations, statName, actualValue);
            free(results);
        }

        // wait times
        for (int j = 0; j < 4; j++)
        {
            for (int i = 0; i < ITERATIONS; i++)
            {
                array[i] = (double)statistics[i].waitTime[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Wait time center %d : %6.6f +/- %6.6f sec\n", j + 1, results[0], results[1]);
            sprintf(statName, "Waiting time %s center", centerNames[j]);
            sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
            writeCSVLine(estimations, statName, actualValue);
            free(results);
        }

        // service times
        for (int j = 0; j < nCenters; j++)
        {
            for (int i = 0; i < ITERATIONS; i++)
            {
                array[i] = (double)statistics[i].serviceTime[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Service time center %d : %6.6f +/- %6.6f sec\n", j + 1, results[0], results[1]);
            sprintf(statName, "Service time %s center", centerNames[j]);
            sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
            writeCSVLine(estimations, statName, actualValue);
            free(results);
        }

        // interarrival times
        for (int j = 0; j < nCenters; j++)
        {
            for (int i = 0; i < ITERATIONS; i++)
            {
                array[i] = (double)statistics[i].interarrivalTime[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Inter-arrival time center %d : %6.6f +/- %6.6f sec\n", j + 1, results[0], results[1]);
            sprintf(statName, "Interarrival time %s center", centerNames[j]);
            sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
            writeCSVLine(estimations, statName, actualValue);
            free(results);
        }

        // average number of jobs in center
        for (int j = 0; j < nCenters; j++)
        {
            for (int i = 0; i < ITERATIONS; i++)
            {
                array[i] = (double)statistics[i].avgNumberOFJobs[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Average number of jobs in the center %d : %6.6f +/- %6.6f jobs\n", j + 1, results[0], results[1]);
            sprintf(statName, "Average number of jobs in %s center", centerNames[j]);
            sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
            writeCSVLine(estimations, statName, actualValue);
            free(results);
        }

        // timeouts
        for (int j = 0; j < 3; j++)
        {
            for (int i = 0; i < ITERATIONS; i++)
            {
                array[i] = (double)statistics[i].numOfTimeouts[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Average number of timeouts in the center %d : %6.6f +/- %6.6f jobs\n", j + 2, results[0], results[1]);
            sprintf(statName, "Timeouts at %s center", centerNames[j + 1]);
            sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
            writeCSVLine(estimations, statName, actualValue);
            free(results);
        }
        // num bypass
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].numOfBypass;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[3] = results;
        printf("Num bypass : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(estimations, "Number of mbypass", actualValue);
        free(results);

        // bypass percentage
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].bypassPercentage;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[3] = results;
        printf("Bypass percentage: %6.6f +/- %6.6f\n", results[0], results[1]);
        free(results);

        // Global response time
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].globalResponseTime;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[3] = results;
        printf("Global response time : %6.6f +/- %6.6f sec\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(estimations, "Global response time", actualValue);
        free(results);

        // Global PREMIUM response time
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].globalPremiumResponseTime;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[3] = results;
        printf("Global PREMIUM response time : %6.6f +/- %6.6f sec\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(estimations, "Global Premium response time", actualValue);
        free(results);

        // Global NORMAL response time
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].globalNormalResponseTime;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[3] = results;
        printf("Global NORMAL response time : %6.6f +/- %6.6f sec\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(estimations, "Global Normal response time", actualValue);
        free(results);

        // Global failure percentage
        for (int i = 0; i < ITERATIONS; i++)
        {
            array[i] = (double)statistics[i].globalFailurePercentage;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[3] = results;
        printf("Percentage of failure : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(estimations, "Failure percentage", actualValue);
        free(results);
        // Rho
        // timeouts
        for (int j = 0; j < nCenters; j++)
        {
            for (int i = 0; i < ITERATIONS; i++)
            {
                array[i] = (double)statistics[i].ro[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Rho in the center %d : %6.6f +/- %6.6f jobs\n", j + 1, results[0], results[1]);
            sprintf(statName, "Rho in the %s center", centerNames[j]);
            sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
            writeCSVLine(estimations, statName, actualValue);
            free(results);
        }

        // close the file of interval estimations
        fclose(estimations);



        // TRANSIENT STUDY : sample statistics every 5 minutes of simulation time
        // calculate mean value, upper and lower bounds for response time global
        FILE *transient = fopen("transient.csv", "w+");
        fprintf(transient, "Minutes, Mean, Upper, Lower\n");
        int minSize = (int) INFINITY;
        for (int i = 0; i < ITERATIONS; i++){
            if (statistics[i].sampleArraySize < minSize){
                minSize = statistics[i].sampleArraySize;
            }
        }

        double means[minSize];
        double upper[minSize];
        double lower[minSize];

        double dataset[ITERATIONS];
        for (int k = 0; k < minSize; k++){
            for (int j = 0; j < ITERATIONS; j++){
                // build a dataset for each sample time
                dataset[j] = statistics[j].samplesResponseTime[k];
            }
            int sampleTime = k*5;   // 5 minutes
            results = welford(confidence, dataset, ITERATIONS);
            means[k] = results[0];
            upper[k] = results[0] + results[1];
            lower[k] = results[0] - results[1];
            free(results);

            fprintf(transient, "%d, %6.6f, %6.6f, %6.6f\n", sampleTime, means[k], upper[k], lower[k]);
        }

        fclose(transient);      
    }
    else{
        // INFINITE HORIZON

        // the method of autocorrelation < 0.2 has been used to have almost independent batches
        int batchNumber = 64;   // k
        int batchSize = 10000;   // b
        int lag = 1;            // j
        double confidence = 0.95;
        printf("Infinite Horizon Simulation\n\n");
        printf("One long run made of %d batches of %d jobs each\n", batchNumber, batchSize);


        stats *simResults = infiniteHorizonSimulation(batchNumber, batchSize, "simulation_stats.csv");
        char *filename = "infinite_horizon.csv";

        FILE *f = fopen(filename, "w+");
        fprintf(f, "Statistic, Analytical result, Experimental result\n");
        char *actualValue = malloc(60 * sizeof(char));
        char *statName = malloc(100 * sizeof(char));

        int numCenters = 4;
        if (IMPROVEMENT)
            numCenters = 5;


        printf("\n\nRecovered Statistics\n\n");
        // global response times
        double *dataPoints = malloc(batchNumber * sizeof(double));
        for(int i = 0; i < batchNumber; i++){
            dataPoints[i] = simResults[i].globalResponseTime;
        }
        double autoCorrelation = autocorrelation(dataPoints, batchNumber, lag);
        double *results;
        results = welford(confidence, dataPoints, batchNumber);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(f, "Global Response Time", actualValue);
        printf("Global response time : %6.6f +/- %6.6f sec\n", results[0], results[1]);
        free(dataPoints);
        free(results);

        // premium global response time
        dataPoints = malloc(batchNumber * sizeof(double));
        for(int i = 0; i < batchNumber; i++){
            dataPoints[i] = simResults[i].globalPremiumResponseTime;
        }
        autoCorrelation = autocorrelation(dataPoints, batchNumber, lag);
        results = welford(confidence, dataPoints, batchNumber);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(f, "Global Premium Response Time", actualValue);
        printf("Global Premium response time : %6.6f +/- %6.6f sec\n", results[0], results[1]);
        free(dataPoints);
        free(results);

        // normal global response time
        dataPoints = malloc(batchNumber * sizeof(double));
        for(int i = 0; i < batchNumber; i++){
            dataPoints[i] = simResults[i].globalNormalResponseTime;
        }
        autoCorrelation = autocorrelation(dataPoints, batchNumber, lag);
        results = welford(confidence, dataPoints, batchNumber);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(f, "Global Normal Response Time", actualValue);
        printf("Global Normal response time : %6.6f +/- %6.6f sec\n", results[0], results[1]);
        free(dataPoints);
        free(results);

        // Rho
        for (int j = 0; j < numCenters; j++){
            dataPoints = malloc(batchNumber * sizeof(double));
            for (int i = 0; i < batchNumber; i++){
                dataPoints[i] = simResults[i].ro[j];
            }
            results = welford(confidence, dataPoints, batchNumber);
            printf("Rho in the center %d : %6.6f +/- %6.6f jobs\n", j + 1, results[0], results[1]);
            sprintf(statName, "Rho in the %s center", centerNames[j]);
            sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
            writeCSVLine(f, statName, actualValue);
            free(results);
            free(dataPoints);
        }

        // bypass percentage
        dataPoints = malloc(batchNumber * sizeof(double));
        for(int i = 0; i < batchNumber; i++){
            dataPoints[i] = simResults[i].bypassPercentage;
        }
        results = welford(confidence, dataPoints, batchNumber);
        printf("Bypass percentage: %6.6f +/- %6.6f sec\n", results[0], results[1]);
        free(dataPoints);
        free(results);

        fclose(f);
    }
}

