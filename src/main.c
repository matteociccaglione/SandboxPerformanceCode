#include "events_queue.h"
#include "../lib/rngs.h"
#include "../lib/rvgs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stats.h"
#include "../lib/rvms.h"

typedef struct __event_list
{
    arrival *arrivals;
    termination *terminations;
} event_list;
#define EXIT_FAILURE 0
#define START 0.0
#define MEAN_INTERARRIVAL_STREAM 0
#define DIGEST_SERVICE_TIME_STREAM 1
#define USER_PROBABILITY_STREAM 2
#define MALWARE_PROBABILITY_STREAM 3
#define DIGEST_MATCHING_PROBABILITY_STREAM 4
#define MEAN_SERVICE_TIME_PREMIUM_STREAM 5
#define MEAN_SERVICE_TIME_NORMAL_STREAM 6
#define MEAN_SERVICE_TIME_RELIABLE_STREAM 7
#define MEAN_SERVICE_TIME_ML_STREAM 8
#define ML_RESULT_STREAM 9
double simulationTime = START;

/**
 * @brief This function is used to generate an arrival to the entire system:
    the arrival is only received by the first center (digest calculation), since the
    arrivals to the other centers are some of the terminations of other centers.
 *
 * @return arrival* A pointer to an arrival event struct
 */
arrival *getArrival()
{
    arrival *event = malloc(sizeof(arrival));
    SelectStream(MEAN_INTERARRIVAL_STREAM);
    double inter = Exponential(MEAN_INTERARRIVAL_TIME); // generate interarrival time

    event->time = inter + simulationTime; // time of the arrival = simulation time + interarrival time
    event->center = CENTER_DIGEST;        // the arrival can only be to the digest calculation center
    SelectStream(USER_PROBABILITY_STREAM);
    event->job.userType = Bernoulli(PROBABILITY_PREMIUM); // generate randomly if the job is from a Premium or Normal user
    SelectStream(MALWARE_PROBABILITY_STREAM);
    event->job.type = Bernoulli(PROBABILITY_MALWARE); // generate randomly if the job is a malware or not
    SelectStream(DIGEST_SERVICE_TIME_STREAM);
    event->job.serviceTime = Exponential(DIGEST_MEAN_SERVICE_TIME); // generate the service time for the digest calculation
    return event;
}

/**
 * @brief This function is used to add a new event (arrival or termination)
 * to the event list of the simulation. There are two separate lists, one for the arrivals
 * and one for the terminations, ordered by time of the event.
 *
 * @param ev Pointer to the event list struct
 * @param node Pointer to an arrival event or to a termination event struct
 * @param type 0 for arrivals, 1 for terminations
 */
void insertList(event_list *ev, void *node, int type)
{
    // arrivals
    if (type == 0)
    {
        arrival *ar = (arrival *)node;
        arrival *head = ev->arrivals;
        arrival *prev = NULL;
        if (head == NULL)
        {
            head = ar;
            ar->next = NULL;
            ev->arrivals = head;
            return;
        }
        while (head != NULL)
        {
            if (head->time > ar->time)
            {
                ar->next = head;
                if (prev == NULL)
                {
                    ev->arrivals = ar;
                }
                else
                {
                    prev->next = ar;
                }
                break;
            }
            else
            {
                prev = head;
                head = head->next;
            }
        }
        if (head == NULL)
        {
            prev->next = ar;
            ar->next = NULL;
        }
    }
    // terminations
    else
    {
        termination *ar = (termination *)node;
        termination *head = ev->terminations;
        termination *prev = NULL;
        if (head == NULL)
        {
            head = ar;
            ar->next = NULL;
            ev->terminations = head;
            return;
        }
        while (head != NULL)
        {
            if (head->time > ar->time)
            {
                ar->next = head;
                if (prev == NULL)
                {
                    ev->terminations = ar;
                }
                else
                {
                    prev->next = ar;
                }
                break;
            }
            else
            {
                prev = head;
                head = head->next;
            }
        }
        if (head == NULL)
        {
            prev->next = ar;
            ar->next = NULL;
        }
    }
}

/**
 * @brief This function returns the minimum value between two doubles.
 *
 * @param val1 First number
 * @param val2 Second number
 * @return double The minimum between the two parameters
 */
double min(double val1, double val2)
{
    if (val1 < val2)
        return val1;
    return val2;
}

/**
 * @brief This function is used to find the free server in a center to which assign a new job.
 * The chosen policy is "IN ORDER" (Lowest numbered idle server).
 * @param servers pointer to an array of servers' status (busy = 1, idle = 0)
 * @param n size of the array
 * @return int The index of the chosen server in the array
 */
int findFreeServer(int *servers, int n)
{
    int i = 0;
    for (i = 0; i < n; i++)
    {
        if (servers[i] == 0)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief This function is used to determine if the next event to be processed during the simulation
 * is an arrival or a termination event. The return value is used to distinguish from which of the two
 * queues of events the event should be extracted (the event extracted is always the head of the list,
 * so the event with lowest happening time).
 *
 * If the arrival event and the termination event have the same happening time, the termination event is preferred
 * as the next event.
 *
 * @param ev The event_list struct containing the lists of arrivals and terminations
 * @return int 0 if arrival, 1 if termination
 */
int nextEvent(event_list ev)
{
    if (ev.terminations == NULL && ev.arrivals != NULL)
    {
        // return arrival
        return 0;
    }
    if (ev.arrivals == NULL && ev.terminations != NULL)
    {
        // return termination
        return 1;
    }
    if (ev.arrivals->time < ev.terminations->time)
    {
        // return arrival
        return 0;
    }
    // return termination
    return 1;
}

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

// Definition of functions to handle the events to the four centers of the system
void handleDigestArrival(digestCenter *digestCenter, event_list *ev);
void handleNormalArrival(normalAnalysisCenter *center, event_list *ev);
void handlePremiumArrival(premiumAnalysisCenter *center, event_list *ev);
void handleReliableArrival(reliableAnalysisCenter *center, event_list *ev);
void handleMachineLearningArrival(machineLearningCenter *mlCenter, event_list *ev);
void handleDigestTermination(digestCenter *digestCenter, event_list *ev);
void handleNormalTermination(normalAnalysisCenter *center, event_list *ev, digestCenter *digestCenter);
void handlePremiumTermination(premiumAnalysisCenter *center, event_list *ev, digestCenter *digestCenter);
void handleReliableTermination(reliableAnalysisCenter *center, event_list *ev, digestCenter *digestCenter);
void handleMachineLearningTermination(machineLearningCenter *mlCenter, event_list *ev);
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
    digestCenter.service_time = 0.0;
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
    initializeServerArray(premiumAnalysisCenter.service_time, premiumAnalysisCenter.servers, N_PREMIUM);
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
    initializeServerArray(normalAnalysisCenter.service_time, normalAnalysisCenter.servers, N_NORMAL);
    normalAnalysisCenter.queue = NULL;
    normalAnalysisCenter.interarrivalTime = 0.0;
    normalAnalysisCenter.lastArrivalTime = 0.0;
    for (int pippo = 0; pippo < N_NORMAL; pippo++)
    {
        normalAnalysisCenter.indexes[pippo] = 0;
    }
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
    initializeServerArray(reliableAnalysisCenter.service_time, reliableAnalysisCenter.servers, N_RELIABLE);
    reliableAnalysisCenter.queueNormal = NULL;
    reliableAnalysisCenter.queuePremium = NULL;
    reliableAnalysisCenter.lastEventTimePremium = 0.0;
    int j = 0;
    for (j = 0; j < N_RELIABLE; j++)
    {
        reliableAnalysisCenter.service_time_premium[j] = 0.0;
        reliableAnalysisCenter.service_time_normal[j] = 0.0;
    }
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
        handleDigestArrival(digestCenter, &events);
        break;
    case CENTER_NORMAL:
        normalAnalysisCenter->area += (events.arrivals->time - normalAnalysisCenter->lastEventTime) * normalAnalysisCenter->jobs;
        normalAnalysisCenter->serviceArea += (events.arrivals->time - normalAnalysisCenter->lastEventTime) * (normalAnalysisCenter->jobs - normalAnalysisCenter->jobsInQueue);
        normalAnalysisCenter->queueArea += (events.arrivals->time - normalAnalysisCenter->lastEventTime) * normalAnalysisCenter->jobsInQueue;
        handleNormalArrival(normalAnalysisCenter, &events);
        break;
    case CENTER_PREMIUM:
        premiumAnalysisCenter->area += (events.arrivals->time - premiumAnalysisCenter->lastEventTime) * premiumAnalysisCenter->jobs;
        premiumAnalysisCenter->serviceArea += (events.arrivals->time - premiumAnalysisCenter->lastEventTime) * (premiumAnalysisCenter->jobs - premiumAnalysisCenter->jobsInQueue);
        premiumAnalysisCenter->queueArea += (events.arrivals->time - premiumAnalysisCenter->lastEventTime) * (premiumAnalysisCenter->jobsInQueue);
        handlePremiumArrival(premiumAnalysisCenter, &events);
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
        /*
    if(events.arrivals->job.userType==PREMIUM){
        // update areas for jobs submitted by premium users
        reliableAnalysisCenter.areaPremium+=(events.arrivals->time-reliableAnalysisCenter.lastEventTimePremium)*reliableAnalysisCenter.premiumJobs;
        reliableAnalysisCenter.serviceAreaPremium+=(events.arrivals->time-reliableAnalysisCenter.lastEventTimePremium)*(reliableAnalysisCenter.premiumJobs-reliableAnalysisCenter.jobsInQueuePremium);
        reliableAnalysisCenter.queueAreaPremium+=(events.arrivals->time-reliableAnalysisCenter.lastEventTimePremium)*(reliableAnalysisCenter.jobsInQueuePremium);
    }else{
        // update areas for jobs sumbitted by normal users
        reliableAnalysisCenter.areaNormal+=(events.arrivals->time-reliableAnalysisCenter.lastEventTimeNormal)*reliableAnalysisCenter.normalJobs;
        reliableAnalysisCenter.queueAreaNormal+=(events.arrivals->time - reliableAnalysisCenter.lastEventTimeNormal)*reliableAnalysisCenter.jobsInQueueNormal;
        reliableAnalysisCenter.serviceAreaNormal+=(events.arrivals->time - reliableAnalysisCenter.lastEventTimeNormal)*(reliableAnalysisCenter.normalJobs-reliableAnalysisCenter.jobsInQueueNormal);
    }*/
        handleReliableArrival(reliableAnalysisCenter, &events);
        break;
    case CENTER_ML:
        if (mlCenter->jobs < N_ML)
        {
            mlCenter->area += (events.arrivals->time - mlCenter->lastEventTime) * mlCenter->jobs;
            mlCenter->serviceArea += (events.arrivals->time - mlCenter->lastEventTime) * mlCenter->jobs;
        }
        handleMachineLearningArrival(mlCenter, &events);
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
        handleDigestTermination(digestCenter, &events);
        break;
    case CENTER_NORMAL:
        normalAnalysisCenter->area += (events.terminations->time - normalAnalysisCenter->lastEventTime) * normalAnalysisCenter->jobs;
        normalAnalysisCenter->serviceArea += (events.terminations->time - normalAnalysisCenter->lastEventTime) * (normalAnalysisCenter->jobs - normalAnalysisCenter->jobsInQueue);
        normalAnalysisCenter->queueArea += (events.terminations->time - normalAnalysisCenter->lastEventTime) * normalAnalysisCenter->jobsInQueue;
        handleNormalTermination(normalAnalysisCenter, &events, digestCenter);
        break;
    case CENTER_PREMIUM:
        premiumAnalysisCenter->area += (events.terminations->time - premiumAnalysisCenter->lastEventTime) * premiumAnalysisCenter->jobs;
        premiumAnalysisCenter->serviceArea += (events.terminations->time - premiumAnalysisCenter->lastEventTime) * (premiumAnalysisCenter->jobs - premiumAnalysisCenter->jobsInQueue);
        premiumAnalysisCenter->queueArea += (events.terminations->time - premiumAnalysisCenter->lastEventTime) * (premiumAnalysisCenter->jobsInQueue);
        handlePremiumTermination(premiumAnalysisCenter, &events, digestCenter);
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
        /*
        if(events.terminations->job.userType==PREMIUM){
            // update areas for jobs submitted by premium users
            reliableAnalysisCenter.areaPremium+=(events.terminations->time-reliableAnalysisCenter.lastEventTimePremium)*reliableAnalysisCenter.premiumJobs;
            reliableAnalysisCenter.serviceAreaPremium+=(events.terminations->time-reliableAnalysisCenter.lastEventTimePremium)*(reliableAnalysisCenter.premiumJobs-reliableAnalysisCenter.jobsInQueuePremium);
            reliableAnalysisCenter.queueAreaPremium+=(events.terminations->time-reliableAnalysisCenter.lastEventTimePremium)*(reliableAnalysisCenter.jobsInQueuePremium);
        }
        else{
            //update areas for jobs submitted by normal users
            reliableAnalysisCenter.areaNormal+=(events.terminations->time-reliableAnalysisCenter.lastEventTimeNormal)*reliableAnalysisCenter.normalJobs;
            reliableAnalysisCenter.queueAreaNormal+=(events.terminations->time - reliableAnalysisCenter.lastEventTimeNormal)*reliableAnalysisCenter.jobsInQueueNormal;
            reliableAnalysisCenter.serviceAreaNormal+=(events.terminations->time - reliableAnalysisCenter.lastEventTimeNormal)*(reliableAnalysisCenter.normalJobs-reliableAnalysisCenter.jobsInQueueNormal);
        }
        */
        handleReliableTermination(reliableAnalysisCenter, &events, digestCenter);
        break;
    case CENTER_ML:
        mlCenter->area += (events.terminations->time - mlCenter->lastEventTime) * mlCenter->jobs;
        mlCenter->serviceArea += (events.terminations->time - mlCenter->lastEventTime) * mlCenter->jobs;
        handleMachineLearningTermination(mlCenter, &events);
    }
    return events;
}

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
    double meanServiceTime = 0.0;

    double means[N_NORMAL];
    int servers_used = 0;
    for (int k = 0; k < N_NORMAL; k++)
    {
        if (normalAnalysisCenter.indexes[k] == 0)
        {
            means[k] = 0.0;
        }
        else
        {
            means[k] = normalAnalysisCenter.service_time[k] / normalAnalysisCenter.indexes[k];
            servers_used++;
        }
    }
    for (int k = 0; k < N_NORMAL; k++)
    {
        meanServiceTime += means[k];
    }
    meanServiceTime = meanServiceTime / servers_used;
    double rho = meanServiceTime / ((normalAnalysisCenter.interarrivalTime * N_NORMAL) / normalAnalysisCenter.index);


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
    double reliablePremiumresponseTime = reliableAnalysisCenter.areaPremium / reliableAnalysisCenter.premiumIndex;
    
    delayArea = reliableAnalysisCenter.queueArea;
    double delayAreaPremium = reliableAnalysisCenter.queueAreaPremium;
    double delayAreaNormal = reliableAnalysisCenter.queueAreaNormal;
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
    if (reliableAnalysisCenter.premiumIndex == 0){
        reliablePremiumresponseTime = 0.0;
    }
    
    
    


    // ML performances

    // Global performances
    // printf("\nGlobal performances\n");
    double globalWaitTime = (digestCenter.queueArea + normalAnalysisCenter.queueArea + premiumAnalysisCenter.queueArea + reliableAnalysisCenter.queueArea) / digestCenter.index;
    double globalPremiumWaitTime = (digestCenter.queueArea + premiumAnalysisCenter.queueArea + reliableAnalysisCenter.queueAreaPremium) / digestCenter.indexPremium;
    double globalNormalWaitTime = (digestCenter.queueArea + normalAnalysisCenter.queueArea + reliableAnalysisCenter.queueAreaNormal) / (digestCenter.index - digestCenter.indexPremium);
    double globalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / digestCenter.index + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.index + reliableResponseTime * ((double)(premiumAnalysisCenter.numberOfTimeouts + normalAnalysisCenter.numberOfTimeouts) / digestCenter.index);


    double globalPremiumResponseTime = digestResponseTime + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.indexPremium + reliableResponseTime * (double)premiumAnalysisCenter.numberOfTimeouts / digestCenter.indexPremium;
    // double globalPremiumResponseTime = (digestCenter.area + premiumAnalysisCenter.area ) / digestCenter.index + reliableAnalysisCenter.areaPremium/premiumAnalysisCenter.index;
    double globalNormalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / (digestCenter.index - digestCenter.indexPremium) + reliableResponseTime * (double)normalAnalysisCenter.numberOfTimeouts / (digestCenter.index - digestCenter.indexPremium);
    // double globalNormalResponseTime = (digestCenter.area + normalAnalysisCenter.area) / (digestCenter.index)  + reliableAnalysisCenter.areaNormal/normalAnalysisCenter.index;
    //  printf("Global waiting time : %6.6f\nGlobal premium waiting time : %6.6f\nGlobal normal waiting time : %6.6f\n",globalWaitTime,globalPremiumWaitTime,globalNormalWaitTime);
    if (IMPROVEMENT)
    {
        // printf("%d\n",mlCenter.index);
        double mlResponseTime = mlCenter.area / mlCenter.index;
        globalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / digestCenter.index + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.index + reliableResponseTime * ((double)(premiumAnalysisCenter.numberOfTimeouts + normalAnalysisCenter.numberOfTimeouts) / digestCenter.index) + mlResponseTime * ((double)mlCenter.index / digestCenter.index);
        globalPremiumResponseTime = digestResponseTime + premiumResponseTime * (double)premiumAnalysisCenter.index / digestCenter.indexPremium + reliableResponseTime * (double)premiumAnalysisCenter.numberOfTimeouts / digestCenter.indexPremium + mlResponseTime * ((double)mlCenter.indexPremium / digestCenter.indexPremium);
        globalNormalResponseTime = digestResponseTime + normalResponseTime * (double)normalAnalysisCenter.index / (digestCenter.index - digestCenter.indexPremium) + reliableResponseTime * (double)normalAnalysisCenter.numberOfTimeouts / (digestCenter.index - digestCenter.indexPremium) + mlResponseTime * ((double)(mlCenter.index - mlCenter.indexPremium) / (digestCenter.index - digestCenter.indexPremium));
    }
    // printf("Global response time : %6.6f\nGlobal premium response time : %6.6f\nGlobal normal response time : %6.6f\n",globalResponseTime,globalPremiumResponseTime,globalNormalResponseTime);
    double percentageFailure = (double)reliableAnalysisCenter.numberOfTimeouts / digestCenter.index;
    // printf("Global failure percentage : %6.6f\n", percentageFailure);
    double check = globalPremiumResponseTime * ((double)digestCenter.indexPremium / digestCenter.index) + globalNormalResponseTime * ((double)(digestCenter.index - digestCenter.indexPremium) / digestCenter.index);
    // printf("%6.6f\n", check);
    statistics.globalResponseTime = globalResponseTime;
    statistics.globalPremiumResponseTime = globalPremiumResponseTime;
    statistics.globalFailurePercentage = percentageFailure;
    statistics.globalNormalResponseTime = globalNormalResponseTime;

    statistics.ro[4] = (mlCenter.serviceArea / mlCenter.index) / (N_ML * mlCenter.interarrivalTime / mlCenter.index);
    statistics.numOfBypass = mlCenter.numOfBypass;
    statistics.responseTime[4] = mlCenter.area / mlCenter.index;
    statistics.serviceTime[4] = mlCenter.serviceArea / mlCenter.index;
    statistics.avgNumberOFJobs[4] = mlCenter.area / mlCenter.interarrivalTime;
    statistics.interarrivalTime[4] = mlCenter.interarrivalTime / mlCenter.index;

    // printf("Verify:\n");
    // TODO: uncomment the following line
    //verify(&digestCenter, &normalAnalysisCenter, &premiumAnalysisCenter, &reliableAnalysisCenter, &mlCenter);
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
    insertList(&events, getArrival(), 0); // generate the first arrival and put it in the list of events
    stats *allStatistics = malloc(sizeof(stats) * batchNumber);
    int digestArrivals = 0;
    int normalArrivals = 0;
    int premiumArrivals = 0;
    int reliableArrivals = 0;
    while (nBatch < batchNumber)
    {
        if (nextEvent(events) == 0)
        {
            //jobsInBatch++;
            
            if (events.arrivals->center == CENTER_DIGEST)
            {
                jobsInBatch++;
                digestArrivals++;
            }else if (events.arrivals->center == CENTER_NORMAL){
                normalArrivals++;
            }else if (events.arrivals->center == CENTER_PREMIUM){
                premiumArrivals++;
            }else{
                reliableArrivals++;
            }
          
            
            // distinguish the center that has to process the arrival event
            events = handleArrival(&digestCenter, &normalAnalysisCenter, &premiumAnalysisCenter, &reliableAnalysisCenter, &mlCenter, events);

            //if ((jobsInBatch % batchSize == 0 && nBatch != batchNumber - 1) || (jobsInBatch % batchSize == 0 && isEmptyList(events) && nBatch == batchNumber - 1))
            //if ((jobsInBatch % batchSize == 0 && nBatch != batchNumber - 1))
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
                nBatch++;
                batchTime = simulationTime;
                jobsInBatch = 0;

                printf("D: %d\tN: %d\tP: %d\tR: %d\n", digestArrivals, normalArrivals, premiumArrivals, reliableArrivals);
                digestArrivals = 0;
                normalArrivals = 0;
                premiumArrivals = 0;
                reliableArrivals = 0;
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
    insertList(&events, getArrival(), 0); // generate the first arrival and put it in the list of events

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

/* use 0.95 for 95% confidence */

double *welford(double confidence, double *statistics, int size)
{
    long n = 0; /* counts data points */
    double sum = 0.0;
    double mean = 0.0;
    double data;
    double stdev;
    double u, t, w;
    double diff;

    for (int i = 0; i < size; i++)
    {                         /* use Welford's one-pass method */
        data = statistics[i]; /* to calculate the sample mean  */
        n++;                  /* and standard deviation        */
        diff = data - mean;
        sum += diff * diff * (n - 1.0) / n;
        mean += diff / n;
    }
    stdev = sqrt(sum / n);

    if (n > 1)
    {
        u = 1.0 - 0.5 * (1.0 - confidence); /* interval parameter  */
        t = idfStudent(n - 1, u);           /* critical value of t */
        w = t * stdev / sqrt(n - 1);        /* interval half width */
        // printf("\nbased upon %ld data points", n);
        // printf(" and with %d%% confidence\n", (int) (100.0 * confidence + 0.5));
        // printf("the expected value is in the interval");
        // printf("%10.2f +/- %6.2f\n", mean, w);
        double *array = malloc(sizeof(double) * 2);
        array[0] = round(1000000 * mean) / 1000000.0;
        array[1] = round(1000000 * w) / 1000000.0;
        return array;
    }
    else
        printf("ERROR - insufficient data\n");
    return (0);
}

/**
 * @brief This function calculate the autocorrelation with lag 1
 * over a dataset of data points.
 * 
 * @param statistics array of data points
 * @param size number of datapoints
 * @param j autocorrelation lag (set 1)
 * @return double autocorrelation value (C_j/C_0)
 */
double autocorrelation(double *statistics, int size, int j)
{
    int i = 0;
    double sum = 0.0;
    double n;
    int p = 0;
    double *cosum = malloc(sizeof(double) * (j + 1));
    double *hold = malloc(sizeof(double) * (j + 1));
    double mean = 0.0;
    while (i < j + 1)
    {                             /* initialize the hold array with */
        double x = statistics[i]; /* the first K + 1 data values    */
        sum += x;
        hold[i] = x;
        i++;
    }

    while (i < size)
    {
        for (int k = 0; k < j + 1; k++)
            cosum[k] += hold[p] * hold[(p + k) % (j + 1)];
        double x = statistics[i];
        sum += x;
        hold[p] = x;
        p = (p + 1) % (j + 1);
        i++;
    }
    n = i;

    while (i < n + j + 1)
    { /* empty the circular array */
        for (int k = 0; k < j + 1; k++){
            cosum[k] += hold[p] * hold[(p + k) % (j + 1)];
        }
        hold[p] = 0.0;
        p = (p + 1) % (j + 1);
        i++;
    }

    mean = sum / n;
    for (int k = 0; k <= j; k++)
        cosum[k] = (cosum[k] / (n - k)) - (mean * mean);

    // return C_j / C_0
    return cosum[j] / cosum[0];
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
    avgStats averageAmongRuns;
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


        // calculate mean value, upper and lower bounds for response time global
        FILE *transient = fopen("transient.csv", "w+");
        fprintf(transient, "Minutes, Mean, Upper, Lower\n");
        int minSize = INFINITY;
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
        
        




        // init struct
        averageAmongRuns.numJobs = 0.0;
        averageAmongRuns.numNormalJobs = 0.0;
        averageAmongRuns.numPremiumJobs = 0.0;
        averageAmongRuns.numDigestMatching = 0.0;
        for (int j = 0; j < 4; j++)
        {
            averageAmongRuns.responseTime[j] = 0.0;
            averageAmongRuns.waitTime[j] = 0.0;
            averageAmongRuns.serviceTime[j] = 0.0;
            averageAmongRuns.interarrivalTime[j] = 0.0;
            averageAmongRuns.avgNumberOFJobs[j] = 0.0;
            if (j != 0)
            {
                averageAmongRuns.numOfTimeouts[j - 1] = 0.0;
            }
        }

        // fill struct
        for (int i = 0; i < ITERATIONS; i++)
        {
            averageAmongRuns.numJobs += statistics[i].numJobs;
            averageAmongRuns.numNormalJobs += statistics[i].numNormalJobs;
            averageAmongRuns.numPremiumJobs += statistics[i].numPremiumJobs;
            averageAmongRuns.numDigestMatching += statistics[i].numDigestMatching;
            for (int j = 0; j < 4; j++)
            {
                averageAmongRuns.responseTime[j] += statistics[i].responseTime[j];
                averageAmongRuns.waitTime[j] += statistics[i].waitTime[j];
                averageAmongRuns.serviceTime[j] += statistics[i].serviceTime[j];
                averageAmongRuns.interarrivalTime[j] += statistics[i].interarrivalTime[j];
                averageAmongRuns.avgNumberOFJobs[j] += statistics[i].avgNumberOFJobs[j];
                if (j != 0)
                {
                    averageAmongRuns.numOfTimeouts[j - 1] += statistics[i].numOfTimeouts[j - 1];
                }
            }
        }
        averageAmongRuns.numJobs /= (double)ITERATIONS;
        averageAmongRuns.numNormalJobs /= (double)ITERATIONS;
        averageAmongRuns.numPremiumJobs /= (double)ITERATIONS;
        averageAmongRuns.numDigestMatching /= (double)ITERATIONS;
        for (int j = 0; j < 4; j++)
        {
            averageAmongRuns.responseTime[j] /= (double)ITERATIONS;
            averageAmongRuns.waitTime[j] /= (double)ITERATIONS;
            averageAmongRuns.serviceTime[j] /= (double)ITERATIONS;
            averageAmongRuns.interarrivalTime[j] /= (double)ITERATIONS;
            averageAmongRuns.avgNumberOFJobs[j] /= (double)ITERATIONS;
            if (j != 0)
            {
                averageAmongRuns.numOfTimeouts[j - 1] /= (double)ITERATIONS;
            }
        }
    }
    else
    {
        // INFINITE HORIZON
        // the method of autocorrelation < 0.2 has been used to have almost independent batches
        int batchNumber = 64;   // k
        int batchSize = 10000;   // b
        int lag = 1;            // j
        double confidence = 0.95;
        stats *simResults = infiniteHorizonSimulation(batchNumber, batchSize, "simulation_stats.csv");
        char *filename = "infinite_horizon.csv";

        FILE *f = fopen(filename, "w+");
        fprintf(f, "Statistic, Analytical result, Experimental result\n");
        char *actualValue = malloc(60 * sizeof(char));

        // autocorrelation of global response times
        double *dataPoints = malloc(batchNumber * sizeof(double));
        for(int i = 0; i < batchNumber; i++){
            dataPoints[i] = simResults[i].globalResponseTime;
        }
        double autoCorrelation = autocorrelation(dataPoints, batchNumber, lag);
        printf("\n\nAutocorrelation for global response time is %6.3f\n", autoCorrelation);
        double *results;
        results = welford(confidence, dataPoints, batchNumber);
        printf("Confidence interval for E(Ts) : %6.6f +/- %6.6f\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(f, "Global Response Time", actualValue);
        free(dataPoints);
        free(results);

        // premium global response time
        dataPoints = malloc(batchNumber * sizeof(double));
        for(int i = 0; i < batchNumber; i++){
            dataPoints[i] = simResults[i].globalPremiumResponseTime;
        }
        autoCorrelation = autocorrelation(dataPoints, batchNumber, lag);
        printf("Autocorrelation for global premium response time is %6.3f\n", autoCorrelation);
        results = welford(confidence, dataPoints, batchNumber);
        printf("Confidence interval for E(Ts_premium) : %6.6f +/- %6.6f\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(f, "Global Premium Response Time", actualValue);
        free(dataPoints);
        free(results);

        // normal global response time
        dataPoints = malloc(batchNumber * sizeof(double));
        for(int i = 0; i < batchNumber; i++){
            dataPoints[i] = simResults[i].globalNormalResponseTime;
        }
        autoCorrelation = autocorrelation(dataPoints, batchNumber, lag);
        printf("Autocorrelation for global normal response time is %6.3f\n", autoCorrelation);
        results = welford(confidence, dataPoints, batchNumber);
        printf("Confidence interval for E(Ts_normal) : %6.6f +/- %6.6f\n", results[0], results[1]);
        sprintf(actualValue, "%6.6f +/- %6.6f", results[0], results[1]);
        writeCSVLine(f, "Global Normal Response Time", actualValue);
        free(dataPoints);
        free(results);
        
        fclose(f);
    }
}

void handleDigestArrival(digestCenter *digestCenter, event_list *ev)
{
    arrival *ar = ev->arrivals;
    // digestCenter->area += (ar->time - digestCenter->lastEventTime) * digestCenter->jobs;
    digestCenter->jobs++;
    simulationTime = ar->time;
    double lastEventTime = digestCenter->lastEventTime;
    digestCenter->lastEventTime = ar->time;
    ev->arrivals = ar->next;
    if (digestCenter->jobs == 1)
    {
        digestCenter->service_time += ar->job.serviceTime;
        termination *term = malloc(sizeof(termination));
        term->time = ar->job.serviceTime + simulationTime;
        term->center = CENTER_DIGEST;
        term->job = ar->job;
        insertList(ev, term, 1);
    }
    else
    {
        // digestCenter->queueArea += (ar->time - lastEventTime) * digestCenter->jobsInQueue;
        digestCenter->jobsInQueue++;
        job_queue *node = malloc(sizeof(job_queue));
        node->job = ar->job;
        insertQueue(&(digestCenter->queue), node);
    }
    // Generate a new arrival and insert into the list of events, only if the simuation time is not over
    if (simulationTime < OBSERVATION_PERIOD || INFINITE_HORIZON)
    {
        arrival *newArrival = getArrival();
        digestCenter->interarrivalTime += newArrival->time - simulationTime;
        insertList(ev, newArrival, 0);
    }

    free(ar);
}

void handleDigestTermination(digestCenter *digestCenter, event_list *ev)
{
    termination *term = ev->terminations;
    // digestCenter->area += (term->time - digestCenter->lastEventTime) * digestCenter->jobs;
    digestCenter->jobs--;
    digestCenter->index++;
    if (term->job.userType == PREMIUM)
    {
        digestCenter->indexPremium++;
    }
    simulationTime = term->time;
    double lastEventTime = digestCenter->lastEventTime;
    digestCenter->lastEventTime = term->time;
    ev->terminations = term->next;

    SelectStream(DIGEST_MATCHING_PROBABILITY_STREAM);
    if (!Bernoulli(digestCenter->probabilityOfMatching))
    {
        // Digest not matching: create new arrival
        if (IMPROVEMENT)
        {
            SelectStream(MEAN_SERVICE_TIME_ML_STREAM);
            arrival *ar = malloc(sizeof(arrival));
            ar->job.type = term->job.type;
            ar->job.userType = term->job.userType;
            ar->job.serviceTime = Exponential(ML_MEAN_SERVICE_TIME);

            ar->time = simulationTime;
            ar->center = CENTER_ML;
            insertList(ev, ar, 0);
        }
        else
        {
            arrival *ar = malloc(sizeof(arrival));
            if (ar == NULL)
            {
                printf("Memory leak\n");
                exit(0);
            }
            switch (term->job.userType)
            {
            case PREMIUM:
                ar->center = CENTER_PREMIUM;
                SelectStream(MEAN_SERVICE_TIME_PREMIUM_STREAM);
                ar->job.serviceTime = Exponential(PREMIUM_MEAN_SERVICE_TIME);
                ar->job.type = term->job.type;
                ar->job.userType = term->job.userType;
                break;
            case NORMAL:
                ar->center = CENTER_NORMAL;
                SelectStream(MEAN_SERVICE_TIME_NORMAL_STREAM);
                ar->job.serviceTime = Exponential(NORMAL_MEAN_SERVICE_TIME);
                ar->job.type = term->job.type;
                ar->job.userType = term->job.userType;
                break;
            }
            ar->time = simulationTime;
            insertList(ev, ar, 0);
        }
    }
    else
    {
        digestCenter->digestMatching++;
    }
    if (digestCenter->jobs >= 1)
    {
        // digestCenter->queueArea += (term->time - lastEventTime) * digestCenter->jobsInQueue;
        digestCenter->jobsInQueue--;
        job_queue *j_q = popQueue(&(digestCenter->queue));
        Job job = j_q->job;
        free(j_q);
        digestCenter->service_time += job.serviceTime;
        termination *ter = malloc(sizeof(termination));
        ter->time = job.serviceTime + simulationTime;
        ter->center = CENTER_DIGEST;
        ter->job = job;
        insertList(ev, ter, 1);
    }
    free(term);
}

void handleNormalArrival(normalAnalysisCenter *center, event_list *ev)
{

    arrival *ar = ev->arrivals;
    ar->job.arrivalTime = ar->time;
    double jobs = center->jobs;
    center->jobs++;
    double current = simulationTime;
    simulationTime = ar->time;
    if (center->lastArrivalTime != 0.0)
        center->interarrivalTime += ar->time - center->lastArrivalTime;
    center->lastArrivalTime = simulationTime;
    double lastEventTime = center->lastEventTime;
    center->lastEventTime = simulationTime;
    ev->arrivals = ar->next;
    if (center->jobs <= N_NORMAL)
    {
        int server_index = findFreeServer(center->servers, N_NORMAL);
        center->servers[server_index] = 1;
        // The actual service time can be less than the generated service time due to timeout expiration
        center->service_time[server_index] += min(ar->job.serviceTime, TIMEOUT);
        center->indexes[server_index]++;
        termination *term = malloc(sizeof(termination));
        term->time = min(ar->job.serviceTime, TIMEOUT) + simulationTime;
        term->center = CENTER_NORMAL;
        term->job = ar->job;
        term->server_index = server_index;
        insertList(ev, term, 1);
    }
    else
    {
        center->jobsInQueue++;
        job_queue *node = malloc(sizeof(job_queue));
        node->job = ar->job;
        insertQueue(&(center->queue), node);
        if (center->jobsInQueue != sizeQueue(center->queue))
        {
            printf("FATAL ERROR\n");
            exit(0);
        }
    }
    free(ar);
}

void handleNormalTermination(normalAnalysisCenter *center, event_list *ev, digestCenter *digestCenter)
{
    termination *ter = ev->terminations;
    center->jobs--;
    center->index++;
    simulationTime = ter->time;
    double lastEventTime = center->lastEventTime;
    center->lastEventTime = simulationTime;
    ev->terminations = ter->next;
    center->meanResponseTime += ter->time - ter->job.arrivalTime;
    if (ter->job.serviceTime > TIMEOUT)
    {
        center->numberOfTimeouts++;
        arrival *ar = malloc(sizeof(arrival));
        ar->center = CENTER_RELIABLE;
        ar->time = simulationTime;
        ar->job = ter->job;
        // Service time is decreased by a factor, because reliable machines are faster than normal machines
        double factor = (double)PREMIUM_MEAN_SERVICE_TIME / NORMAL_MEAN_SERVICE_TIME;
        SelectStream(MEAN_SERVICE_TIME_RELIABLE_STREAM);
        ar->job.serviceTime = Exponential(RELIABLE_MEAN_SERVICE_TIME);
        insertList(ev, ar, 0);
    }
    else if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB)
    {
        digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;
    }
    center->servers[ter->server_index] = 0;
    if (center->jobs >= N_NORMAL)
    {
        center->jobsInQueue--;
        int server_index = findFreeServer(center->servers, N_NORMAL);
        center->servers[server_index] = 1;
        job_queue *j_q = popQueue(&center->queue);
        if (center->jobsInQueue != sizeQueue(center->queue))
        {
            printf("FATAL ERROR\n");
            exit(0);
        }
        Job job = j_q->job;
        center->meanQueueTime += simulationTime - job.arrivalTime;
        free(j_q);
        // The actual service time can be less than the generated service time due to timeout expiration
        center->service_time[server_index] += min(job.serviceTime, TIMEOUT);
        center->indexes[server_index]++;
        termination *termi = malloc(sizeof(termination));
        termi->time = min(job.serviceTime, TIMEOUT) + simulationTime;
        termi->center = CENTER_NORMAL;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
}

void handlePremiumArrival(premiumAnalysisCenter *center, event_list *ev)
{

    arrival *ar = ev->arrivals;
    // center->area += (ar->time - center->lastEventTime) * center->jobs;
    center->jobs++;
    simulationTime = ar->time;
    if (center->lastArrivalTime != 0.0)
    {
        center->interarrivalTime += ar->time - center->lastArrivalTime;
    }
    center->lastArrivalTime = simulationTime;
    double lastEventTime = center->lastEventTime;
    center->lastEventTime = simulationTime;
    ev->arrivals = ar->next;

    if (center->jobs <= N_PREMIUM)
    {
        int server_index = findFreeServer(center->servers, N_PREMIUM);
        center->servers[server_index] = 1;
        // The actual service time can be less than the generated service time due to timeout expiration
        center->service_time[server_index] += min(ar->job.serviceTime, TIMEOUT);
        termination *term = malloc(sizeof(termination));
        if (term == NULL)
        {
            printf("Memory leak\n");
            exit(0);
        }
        term->time = min(ar->job.serviceTime, TIMEOUT) + simulationTime;
        term->center = CENTER_PREMIUM;
        term->job = ar->job;
        term->server_index = server_index;

        insertList(ev, term, 1);
    }
    else
    {
        // center->queueArea += (ar->time - lastEventTime) * center->jobsInQueue;
        center->jobsInQueue++;
        job_queue *node = malloc(sizeof(job_queue));

        if (node == NULL)
        {
            printf("Memory leak\n");
            exit(0);
        }
        node->job = ar->job;

        insertQueue(&(center->queue), node);
        if (center->jobsInQueue != sizeQueue(center->queue))
        {
            printf("FATAL ERROR\n");
            exit(0);
        }
    }
    free(ar);
}

void handlePremiumTermination(premiumAnalysisCenter *center, event_list *ev, digestCenter *digestCenter)
{
    termination *ter = ev->terminations;
    // center->area += (ter->time - center->lastEventTime) * center->jobs;
    center->jobs--;
    center->index++;
    simulationTime = ter->time;
    double lastEventTime = center->lastEventTime;
    center->lastEventTime = simulationTime;
    ev->terminations = ter->next;
    if (ter->job.serviceTime > TIMEOUT)
    {
        center->numberOfTimeouts++;
        arrival *ar = malloc(sizeof(arrival));
        ar->center = CENTER_RELIABLE;
        ar->time = simulationTime;
        ar->job = ter->job;
        SelectStream(MEAN_SERVICE_TIME_RELIABLE_STREAM);
        ar->job.serviceTime = Exponential(RELIABLE_MEAN_SERVICE_TIME);
        insertList(ev, ar, 0);
    }
    else if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB)
    {
        digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;
    }
    center->servers[ter->server_index] = 0;
    if (center->jobs >= N_PREMIUM)
    {
        // center->queueArea += (ter->time - lastEventTime) * center->jobsInQueue;
        center->jobsInQueue--;
        int server_index = findFreeServer(center->servers, N_PREMIUM);
        center->servers[server_index] = 1;
        job_queue *j_q = popQueue(&(center->queue));
        if (center->jobsInQueue != sizeQueue(center->queue))
        {
            printf("FATAL ERROR\n");
            exit(0);
        }
        Job job = j_q->job;
        free(j_q);
        // The actual service time can be less than the generated service time due to timeout expiration
        center->service_time[server_index] += min(job.serviceTime, TIMEOUT);
        termination *termi = malloc(sizeof(termination));
        termi->time = min(job.serviceTime, TIMEOUT) + simulationTime;
        termi->center = CENTER_PREMIUM;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
}

void handleReliableArrival(reliableAnalysisCenter *center, event_list *ev)
{
    arrival *ar = ev->arrivals;
    // center->area += (ar->time - center->lastEventTime) * center->jobs;
    UserType type = ar->job.userType;
    double lastEventTimeNormal = center->lastEventTimeNormal;
    double lastEventTimePremium = center->lastEventTimePremium;
    double lastEventTime = center->lastEventTime;
    if (type == PREMIUM)
    {
        // center->areaPremium += (ar->time - center->lastEventTimePremium) * center->premiumJobs;
        center->lastEventTimePremium = ar->time;
        center->premiumJobs++;
    }
    else
    {
        // center->areaNormal += (ar->time - center->lastEventTimeNormal) * center->normalJobs;
        center->lastEventTimeNormal = ar->time;
        center->normalJobs++;
    }
    center->jobs++;
    simulationTime = ar->time;
    if (center->lastArrivalTime != 0.0)
        center->interarrivalTime += ar->time - center->lastArrivalTime;
    center->lastArrivalTime = simulationTime;
    center->lastEventTime = simulationTime;
    ev->arrivals = ar->next;
    if (center->jobs <= N_RELIABLE)
    {
        int server_index = findFreeServer(center->servers, N_RELIABLE);
        center->servers[server_index] = 1;
        // The actual service time can be less than the generated service time due to timeout expiration
        // center->service_time[server_index] += min(ar->job.serviceTime, TIMEOUT_RELIABLE);
        if (type == PREMIUM)
        {
            center->service_time_premium[server_index] += min(ar->job.serviceTime, TIMEOUT_RELIABLE);
        }
        termination *term = malloc(sizeof(termination));
        term->time = min(ar->job.serviceTime, TIMEOUT_RELIABLE) + simulationTime;
        // term->time = ar->job.serviceTime + simulationTime;
        term->center = CENTER_RELIABLE;
        term->job = ar->job;
        term->server_index = server_index;
        insertList(ev, term, 1);
    }
    else
    {
        job_queue *j_q;
        job_queue *node = malloc(sizeof(job_queue));
        if (node == NULL)
        {
            printf("Memory leak\n");
            exit(0);
        }
        node->job = ar->job;
        // center->queueArea += (ar->time - lastEventTime) * (center->jobsInQueueNormal + center->jobsInQueuePremium);
        if (type == PREMIUM)
        {
            // center->queueAreaPremium += (ar->time - lastEventTimePremium) * center->jobsInQueuePremium;
            center->jobsInQueuePremium++;
            insertQueue(&(center->queuePremium), node);
        }
        else
        {
            // center->queueAreaNormal += (ar->time - lastEventTimeNormal) * center->jobsInQueueNormal;
            center->jobsInQueueNormal++;
            insertQueue(&(center->queueNormal), node);
        }
    }
    free(ar);
}

void handleReliableTermination(reliableAnalysisCenter *center, event_list *ev, digestCenter *digestCenter)
{
    termination *ter = ev->terminations;
    // center->area += (ter->time - center->lastEventTime) * center->jobs;
    UserType type = ter->job.userType;
    double lastEventTimeNormal = center->lastEventTimeNormal;
    double lastEventTimePremium = center->lastEventTimePremium;
    double lastEventTime = center->lastEventTime;
    if (type == PREMIUM)
    {
        // center->areaPremium += (ter->time - center->lastEventTimePremium) * center->premiumJobs;
        center->lastEventTimePremium = ter->time;
        center->premiumJobs--;
        center->premiumIndex++;
    }
    else
    {
        // center->areaNormal += (ter->time - center->lastEventTimeNormal) * center->normalJobs;
        center->lastEventTimeNormal = ter->time;
        center->normalJobs--;
        center->normalIndex++;
    }
    center->jobs--;
    center->index++;
    simulationTime = ter->time;
    center->lastEventTime = simulationTime;
    ev->terminations = ter->next;

    if (ter->job.serviceTime > TIMEOUT_RELIABLE)
    {
        center->numberOfTimeouts++;
    }
    else
    {
        if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB)
        {
            digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;
        }
        center->jobAnalyzed++;
    }

    center->servers[ter->server_index] = 0;
    if (center->jobs >= N_RELIABLE)
    {
        int server_index = findFreeServer(center->servers, N_RELIABLE);
        center->servers[server_index] = 1;
        Job job;

        // center->queueArea += (ter->time - lastEventTime) * (center->jobsInQueueNormal + center->jobsInQueuePremium);
        if (center->queuePremium == NULL)
        {
            // job of the normal queue can be processed
            // center->queueAreaNormal += (ter->time - lastEventTimeNormal) * center->jobsInQueueNormal;
            center->jobsInQueueNormal--;
            job_queue *j_q = popQueue(&(center->queueNormal));
            job = j_q->job;
            free(j_q);
        }
        else
        {
            // center->queueAreaPremium += (ter->time - lastEventTimePremium) * center->jobsInQueuePremium;
            center->jobsInQueuePremium--;
            job_queue *j_q = popQueue(&(center->queuePremium));
            job = j_q->job;
            free(j_q);
        }

        // The actual service time can be less than the generated service time due to timeout expiration
        center->service_time[server_index] += min(job.serviceTime, TIMEOUT_RELIABLE);
        if (type == PREMIUM)
        {
            center->service_time_premium[server_index] += min(ter->job.serviceTime, TIMEOUT_RELIABLE);
        }
        termination *termi = malloc(sizeof(termination));
        termi->time = min(job.serviceTime, TIMEOUT_RELIABLE) + simulationTime;
        // termi->time = job.serviceTime + simulationTime;
        termi->center = CENTER_RELIABLE;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
}

void handleMachineLearningArrival(machineLearningCenter *mlCenter, event_list *ev)
{
    arrival *ar = ev->arrivals;
    simulationTime = ar->time;
    ev->arrivals = ar->next;
    // printf("ML Service time = %6.6f\n",ar->job.serviceTime);

    if (mlCenter->jobs < N_ML)
    {
        if (mlCenter->lastArrivalTime != 0.0)
            mlCenter->interarrivalTime += ar->time - mlCenter->lastArrivalTime;
        mlCenter->lastArrivalTime = simulationTime;
        mlCenter->jobs++;
        mlCenter->lastEventTime = simulationTime;
        // We can handle this job
        termination *terr = malloc(sizeof(termination));
        terr->center = CENTER_ML;
        terr->time = simulationTime + ar->job.serviceTime;
        terr->job = ar->job;
        insertList(ev, terr, 1);
    }
    else
    {
        mlCenter->numOfBypass++;
        // Job is moved to the correct center
        if (ar->job.userType == PREMIUM)
        {
            arrival *arr = malloc(sizeof(arrival));
            arr->center = CENTER_PREMIUM;
            arr->time = simulationTime;
            arr->job = ar->job;
            SelectStream(MEAN_SERVICE_TIME_PREMIUM_STREAM);
            arr->job.serviceTime = Exponential(PREMIUM_MEAN_SERVICE_TIME);
            insertList(ev, arr, 0);
        }
        else
        {
            arrival *arr = malloc(sizeof(arrival));
            arr->center = CENTER_NORMAL;
            arr->time = simulationTime;
            arr->job = ar->job;
            SelectStream(MEAN_SERVICE_TIME_NORMAL_STREAM);
            arr->job.serviceTime = Exponential(NORMAL_MEAN_SERVICE_TIME);
            insertList(ev, arr, 0);
        }
    }
    free(ar);
}

void handleMachineLearningTermination(machineLearningCenter *mlCenter, event_list *ev)
{
    termination *ter = ev->terminations;
    simulationTime = ter->time;
    mlCenter->lastEventTime = simulationTime;
    ev->terminations = ter->next;
    mlCenter->jobs--;
    mlCenter->index++;
    if (ter->job.userType == PREMIUM)
    {
        mlCenter->indexPremium++;
    }
    SelectStream(ML_RESULT_STREAM);
    if (Bernoulli(PROB_POSITIVE_ML))
    {
        // We have a positive result that goes out of the system
        mlCenter->mlSuccess++;
    }
    else
    {
        // Negative result
        if (ter->job.userType == PREMIUM)
        {
            arrival *arr = malloc(sizeof(arrival));
            arr->center = CENTER_PREMIUM;
            arr->time = simulationTime;
            arr->job = ter->job;
            SelectStream(MEAN_SERVICE_TIME_PREMIUM_STREAM);
            arr->job.serviceTime = Exponential(PREMIUM_MEAN_SERVICE_TIME);
            insertList(ev, arr, 0);
        }
        else
        {
            arrival *arr = malloc(sizeof(arrival));
            arr->center = CENTER_NORMAL;
            arr->time = simulationTime;
            arr->job = ter->job;
            SelectStream(MEAN_SERVICE_TIME_NORMAL_STREAM);
            arr->job.serviceTime = Exponential(NORMAL_MEAN_SERVICE_TIME);
            insertList(ev, arr, 0);
        }
    }
    free(ter);
}

/**
 * @brief This function is used to do the VERIFICATION of the system.
 * By carrying out consistency checks between the measured values. This function must be followed after each finite horizon simulation run.
 * If a consistency check skips the function it stops the simulation showing the error presented.
 *  The checks done are:
 *          - Check that service time = wait time (queue) + service time, for each center and for the two separate queues of the reliable center
 *          - Check
 *
 * @param digestCenter
 * @param normalCenter
 * @param premiumCenter
 * @param reliableCenter
 */
void verify(digestCenter *digestCenter, normalAnalysisCenter *normalCenter, premiumAnalysisCenter *premiumCenter, reliableAnalysisCenter *reliableCenter, machineLearningCenter *mlCenter)
{

    double responseTime = digestCenter->area / digestCenter->index;
    double waitTime = digestCenter->queueArea / digestCenter->index;
    double serviceTime = digestCenter->serviceArea / digestCenter->index;
    if (round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000 == 0)
    {
        printf("Verify for digest calculation center: \n");
        printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }
    responseTime = normalCenter->area / normalCenter->index;
    waitTime = normalCenter->queueArea / normalCenter->index;
    serviceTime = normalCenter->serviceArea / normalCenter->index;
    if (round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000 == 0)
    {
        printf("Verify for normal center: \n");
        printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }
    responseTime = premiumCenter->area / premiumCenter->index;
    waitTime = premiumCenter->queueArea / premiumCenter->index;
    serviceTime = premiumCenter->serviceArea / premiumCenter->index;
    if (round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000 == 0)
    {
        printf("Verify for premium center: \n");
        printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }
    responseTime = reliableCenter->area / reliableCenter->index;
    waitTime = reliableCenter->queueArea / reliableCenter->index;
    serviceTime = reliableCenter->serviceArea / reliableCenter->index;
    if (round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000 == 0)
    {
        printf("Verify for reliable center: \n");
        printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }
    responseTime = reliableCenter->areaPremium / reliableCenter->premiumIndex;
    waitTime = reliableCenter->queueAreaPremium / reliableCenter->premiumIndex;
    serviceTime = reliableCenter->serviceAreaPremium / reliableCenter->premiumIndex;
    if (round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000 == 0)
    {
        printf("Verify for reliable center (Premium class): \n");
        printf("E(Ts1) = E(Tq1) + E(s1) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }
    responseTime = reliableCenter->areaNormal / reliableCenter->normalIndex;
    waitTime = reliableCenter->queueAreaNormal / reliableCenter->normalIndex;
    serviceTime = reliableCenter->serviceAreaNormal / reliableCenter->normalIndex;

    if (round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000 == 0)
    {
        printf("Verify for reliable center (Normal class): \n");
        printf("E(Ts2) = E(Tq2) + E(s2) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }

    // Verify routing probabilities

    double probabilityPremium = round(100 * (double)digestCenter->indexPremium / digestCenter->index) / 100;
    double probabilityNormal = round(100 * ((double)digestCenter->index - digestCenter->indexPremium) / digestCenter->index) / 100;

    if ((probabilityPremium == PROBABILITY_PREMIUM) == 0)
    {
        printf("Verify that P(Job is submitted by a Premium user) = %6.2f\n", PROBABILITY_PREMIUM);
        printf("Probability computed = %6.2f\n", probabilityPremium);
        printf("Condition is satisfied: %d\n", probabilityPremium == PROBABILITY_PREMIUM);
        exit(EXIT_FAILURE);
    }
    if ((probabilityNormal == round(100 * (1 - PROBABILITY_PREMIUM)) / 100) == 0)
    {
        printf("Verify that P(Job is submitted by a Normal user) = %6.2f\n", 1 - PROBABILITY_PREMIUM);
        printf("Probability computed = %6.2f\n", probabilityNormal);
        printf("Condition is satisfied: %d\n", probabilityNormal == round(100 * (1 - PROBABILITY_PREMIUM)) / 100);
        exit(EXIT_FAILURE);
    }
    double probabilityDigestMatching = round(100 * (double)digestCenter->digestMatching / digestCenter->index) / 100;
    if ((probabilityDigestMatching == FINAL_DIGEST_MATCHING_PROB) == 0)
    {
        printf("Verify that P(Job matches digest) = %6.2f\n", FINAL_DIGEST_MATCHING_PROB);
        printf("Probability computed = %6.2f\n", probabilityDigestMatching);
        printf("Condition is satisfied: %d\n", probabilityDigestMatching == FINAL_DIGEST_MATCHING_PROB);
        exit(EXIT_FAILURE);
    }
    double probabilityOfTimeoutPremium = round(100 * ((double)premiumCenter->numberOfTimeouts / premiumCenter->index)) / 100;
    double probabilityOfTimeoutNormal = round(100 * ((double)normalCenter->numberOfTimeouts / normalCenter->index)) / 100;
    double probabilityOfTimeoutReliable = round(100 * ((double)reliableCenter->numberOfTimeouts / reliableCenter->index)) / 100;

    double p1 = (double)premiumCenter->numberOfTimeouts / (reliableCenter->index);
    // printf("P1 = %6.6f\n", p1);
    // printf("E(Tq) = p1*E(tq1)+p2*E(Tq2) = %6.6f : %d\n", round(1000000 * (p1 * (reliableCenter->queueAreaPremium / reliableCenter->premiumIndex) + (1 - p1) * (reliableCenter->queueAreaNormal / reliableCenter->normalIndex))) / 1000000, round(1000000 * (p1 * (reliableCenter->queueAreaPremium / reliableCenter->premiumIndex) + (1 - p1) * (reliableCenter->queueAreaNormal / reliableCenter->normalIndex))) / 1000000 == round(1000000 * reliableCenter->queueArea / (reliableCenter->index)) / 1000000);

    double expectedTimeoutPremium = round(100 * exp(-(double)TIMEOUT / PREMIUM_MEAN_SERVICE_TIME)) / 100;
    double expectedTimeoutNormal = round(100 * exp(-(double)TIMEOUT / NORMAL_MEAN_SERVICE_TIME)) / 100;
    double expectedTimeoutReliable = round(100 * exp(-(double)TIMEOUT_RELIABLE / PREMIUM_MEAN_SERVICE_TIME)) / 100;
    if (((probabilityOfTimeoutPremium == round(100 * (expectedTimeoutPremium + 0.01)) / 100) || (probabilityOfTimeoutPremium == round(100 * (expectedTimeoutPremium - 0.01)) / 100) || (probabilityOfTimeoutPremium == expectedTimeoutPremium)) == 0)
    {
        printf("Verify that P(Job is timed out| Job is in the premium center) = %6.2f\n", expectedTimeoutPremium);
        printf("Probability computed = %6.2f\n", probabilityOfTimeoutPremium);
        printf("Condition is satisfied: %d\n", ((probabilityOfTimeoutPremium == round(100 * (expectedTimeoutPremium + 0.01)) / 100) || (probabilityOfTimeoutPremium == round(100 * (expectedTimeoutPremium - 0.01)) / 100) || (probabilityOfTimeoutPremium == expectedTimeoutPremium)));
        exit(EXIT_FAILURE);
    }
    if (((probabilityOfTimeoutNormal == round(100 * (expectedTimeoutNormal + 0.01)) / 100) || (probabilityOfTimeoutNormal == round(100 * (expectedTimeoutNormal - 0.01)) / 100) || (probabilityOfTimeoutNormal == expectedTimeoutNormal)) == 0)
    {
        printf("Verify that P(Job is timed out| Job is in the normal center) = %6.2f\n", expectedTimeoutNormal);
        printf("Probability computed = %6.2f\n", probabilityOfTimeoutNormal);
        printf("Condition is satisfied: %d\n", (probabilityOfTimeoutNormal == expectedTimeoutNormal + 0.01) || (probabilityOfTimeoutNormal == expectedTimeoutNormal - 0.01) || (probabilityOfTimeoutNormal == expectedTimeoutNormal));
        exit(EXIT_FAILURE);
    }
    /*
        printf("Verify that P(Job is timed out| Job is in the reliable center) = %6.2f\n",expectedTimeoutReliable);
        printf("Probability computed = %6.2f\n",probabilityOfTimeoutReliable);
        printf("Condition is satisfied: %d\n",probabilityOfTimeoutReliable==expectedTimeoutReliable);

        */
    // Verify that number of jobs in input is equals to number of jobs analyzed + number of jobs timed out
    int numberOfInput = digestCenter->index;
    int numberOfProcessed = premiumCenter->index - premiumCenter->numberOfTimeouts + normalCenter->index - normalCenter->numberOfTimeouts + digestCenter->digestMatching + reliableCenter->jobAnalyzed;
    if (!IMPROVEMENT)
    {
        if ((numberOfProcessed + reliableCenter->numberOfTimeouts == numberOfInput) == 0)
        {
            printf("Verify that number of jobs in input is equals to number of jobs analyzed + number of jobs timed out\n");
            printf("#job in input = #processed+#timedout -> %d = %d+%d\n", numberOfInput, numberOfProcessed, reliableCenter->numberOfTimeouts);
            printf("Condition is satisfied: %d\n", numberOfProcessed + reliableCenter->numberOfTimeouts == numberOfInput);
            exit(EXIT_FAILURE);
        }
    }

    double lambda = round(1000000 * (1 / digestCenter->interarrivalTime * digestCenter->index)) / 1000000;
    serviceTime = round(1000000 * (digestCenter->area / digestCenter->index)) / 1000000;
    double eN = round(1000000 * (double)digestCenter->area / digestCenter->interarrivalTime) / 1000000;
    double experimental = round(1000000 * (lambda * serviceTime) / 1000000);
    int condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
    if (!condition)
    {
        printf("Verify that little is valid for digest center\n");
        printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
        printf("Condition is not satisfied\n");
        exit(EXIT_FAILURE);
    }
    lambda = round(1000000 * (1 / normalCenter->interarrivalTime * normalCenter->index)) / 1000000;
    serviceTime = round(1000000 * (normalCenter->area / normalCenter->index)) / 1000000;
    eN = round(1000000 * (double)normalCenter->area / normalCenter->interarrivalTime) / 1000000;
    experimental = round(1000000 * (lambda * serviceTime) / 1000000);
    condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
    if (!condition)
    {
        printf("Verify that little is valid for normal center\n");
        printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
        printf("Condition is not satisfied\n");
        exit(EXIT_FAILURE);
    }
    lambda = round(1000000 * (1 / premiumCenter->interarrivalTime * premiumCenter->index)) / 1000000;
    serviceTime = round(1000000 * (premiumCenter->area / premiumCenter->index)) / 1000000;
    eN = round(1000000 * (double)premiumCenter->area / premiumCenter->interarrivalTime) / 1000000;
    experimental = round(1000000 * (lambda * serviceTime) / 1000000);
    condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
    if (!condition)
    {
        printf("Verify that little is valid for premium center\n");
        printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
        printf("Condition is not satisfied\n");
        exit(EXIT_FAILURE);
    }
    lambda = round(1000000 * (1 / reliableCenter->interarrivalTime * reliableCenter->index)) / 1000000;
    serviceTime = round(1000000 * (reliableCenter->area / reliableCenter->index)) / 1000000;
    eN = round(1000000 * (double)reliableCenter->area / reliableCenter->interarrivalTime) / 1000000;
    experimental = round(1000000 * (lambda * serviceTime) / 1000000);
    condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
    if (!condition)
    {
        printf("Verify that little is valid for reliable center\n");
        printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
        printf("Condition is not satisfied\n");
        exit(EXIT_FAILURE);
    }
    if (IMPROVEMENT)
    {
        lambda = round(1000000 * (1 / mlCenter->interarrivalTime * mlCenter->index)) / 1000000;
        serviceTime = round(1000000 * (mlCenter->area / mlCenter->index)) / 1000000;
        eN = round(1000000 * (double)mlCenter->area / mlCenter->interarrivalTime) / 1000000;
        experimental = round(1000000 * (lambda * serviceTime) / 1000000);
        condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
        if (!condition)
        {
            printf("Verify that little is valid for machine learning center\n");
            printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
            printf("Condition is not satisfied\n");
            exit(EXIT_FAILURE);
        }
    }
}
