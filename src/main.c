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

#define START 0.0
#define MEAN_INTERARRIVAL_STREAM 0
#define DIGEST_SERVICE_TIME_STREAM 1
#define USER_PROBABILITY_STREAM 2
#define MALWARE_PROBABILITY_STREAM 3
#define DIGEST_MATCHING_PROBABILITY_STREAM 4
#define MEAN_SERVICE_TIME_PREMIUM_STREAM 5
#define MEAN_SERVICE_TIME_NORMAL_STREAM 6
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
    double inter = Exponential(MEAN_INTERARRIVAL_TIME);             // generate interarrival time
    
    event->time = inter + simulationTime;                           // time of the arrival = simulation time + interarrival time
    event->center = CENTER_DIGEST;                                  // the arrival can only be to the digest calculation center
    SelectStream(USER_PROBABILITY_STREAM);
    event->job.userType = Bernoulli(PROBABILITY_PREMIUM);           // generate randomly if the job is from a Premium or Normal user
    SelectStream(MALWARE_PROBABILITY_STREAM);
    event->job.type = Bernoulli(PROBABILITY_MALWARE);               // generate randomly if the job is a malware or not
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
    else{
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
double min(double val1, double val2){
    if(val1<val2)
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
    if (ev.terminations == NULL && ev.arrivals!=NULL){
        // return arrival
        return 0;
    }
    if(ev.arrivals==NULL && ev.terminations!=NULL){
        //return termination
        return 1;
    }
    if (ev.arrivals->time < ev.terminations->time){
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
void handleDigestTermination( digestCenter *digestCenter, event_list *ev);
void handleNormalTermination(normalAnalysisCenter *center, event_list *ev, digestCenter *digestCenter);
void handlePremiumTermination(premiumAnalysisCenter *center, event_list *ev, digestCenter *digestCenter);
void handleReliableTermination(reliableAnalysisCenter *center, event_list *ev, digestCenter *digestCenter);
void verify(digestCenter *digestCenter, normalAnalysisCenter *normalCenter, premiumAnalysisCenter *premiumCenter, reliableAnalysisCenter *reliableCenter);

/**
 * @brief This function is used to do a single simulation run of the system.
 * 
 * @param runNumber The progressive ID of the simulation run
 * @param filename Filename of the file where to save statistics
 * @return stats Returns a struct containing recovered statistics through the simulation run
 */
stats oneTimeSimulation(int runNumber, char *filename){

    // Initial setup
    simulationTime=START;                                                   // set the initial value of the simulation clock
    event_list events;                                                      // struct that contains the event lists of the simulation
    digestCenter digestCenter;                                              // struct containing info on the Digest Center during the simulation
    normalAnalysisCenter normalAnalysisCenter;                              // struct containing info on the Normal Analysis Center during the simulation
    premiumAnalysisCenter premiumAnalysisCenter;                            // struct containing info on the Premium Analysis Center during the simulation
    reliableAnalysisCenter reliableAnalysisCenter;                          // struct containing info on the Reliable Analysis Center during the simulation

    // Digest center initialization
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
    digestCenter.interarrivalTime=0.0;
    digestCenter.serviceArea=0.0;

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
    normalAnalysisCenter.interarrivalTime=0.0;
    normalAnalysisCenter.lastArrivalTime = 0.0;
    for(int pippo=0; pippo < N_NORMAL; pippo++){
        normalAnalysisCenter.indexes[pippo]=0;
    }
    normalAnalysisCenter.serviceArea=0.0;

    // Premium center initialization
    premiumAnalysisCenter.area = 0.0;
    premiumAnalysisCenter.queueArea = 0.0;
    premiumAnalysisCenter.numberOfTimeouts = 0;
    premiumAnalysisCenter.lastEventTime = 0.0;
    premiumAnalysisCenter.index = 0;
    premiumAnalysisCenter.jobs = 0;
    premiumAnalysisCenter.jobsInQueue = 0;
    initializeServerArray(premiumAnalysisCenter.service_time, premiumAnalysisCenter.servers, N_PREMIUM);
    premiumAnalysisCenter.queue = NULL;
    premiumAnalysisCenter.interarrivalTime=0.0;
    premiumAnalysisCenter.lastArrivalTime=0.0;
    premiumAnalysisCenter.serviceArea=0.0;

    // Reliable center initialization
    reliableAnalysisCenter.area = 0.0;
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
    reliableAnalysisCenter.lastEventTimePremium=0.0;
    int j = 0;
    for(j=0; j<N_RELIABLE;j++){
        reliableAnalysisCenter.service_time_premium[j]=0.0;
        reliableAnalysisCenter.service_time_normal[j]=0.0;
    }
    reliableAnalysisCenter.interarrivalTime=0.0;
    reliableAnalysisCenter.lastArrivalTime=0.0;
    reliableAnalysisCenter.lastEventTimeNormal=0.0;
    reliableAnalysisCenter.normalJobs=0;
    reliableAnalysisCenter.normalIndex=0;
    reliableAnalysisCenter.serviceArea=0.0;
    reliableAnalysisCenter.serviceAreaNormal=0.0;
    reliableAnalysisCenter.serviceAreaPremium=0.0;
    reliableAnalysisCenter.jobAnalyzed=0;

    // Initialize event lists
    events.arrivals = NULL;                                                 // list of arrivals
    events.terminations = NULL;                                             // list of terminations
    PlantSeeds(123456789+runNumber);                                         // seeds for RNGS
    insertList(&events, getArrival(), 0);                                   // generate the first arrival and put it in the list of events
    
    // Run the simulation until the simulation time is greater or equal of the defined observation period.
    // If so, continue the simulation until the lists of events are completely processed and become empty
    while (simulationTime < OBSERVATION_PERIOD || !(isEmptyList(events))){

        // next event is an arrival
        if (nextEvent(events) == 0){
            // distinguish the center that has to process the arrival event 
            switch (events.arrivals->center){
                // Before calling the appropriate function to handle the event, update areas used to calculate statistics
                // Areas to be updated : time-integrated number of jobs in center, time-integrated number of jobs in queue, time-integrated number of jobs in service
            case CENTER_DIGEST:
                digestCenter.area+=(events.arrivals->time-digestCenter.lastEventTime)*digestCenter.jobs;
                digestCenter.serviceArea+=(events.arrivals->time-digestCenter.lastEventTime)*(digestCenter.jobs-digestCenter.jobsInQueue);
                digestCenter.queueArea+=(events.arrivals->time-digestCenter.lastEventTime)*digestCenter.jobsInQueue;
                handleDigestArrival(&digestCenter, &events);
                break;
            case CENTER_NORMAL:
                normalAnalysisCenter.area+=(events.arrivals->time-normalAnalysisCenter.lastEventTime)*normalAnalysisCenter.jobs;
                normalAnalysisCenter.serviceArea+=(events.arrivals->time-normalAnalysisCenter.lastEventTime)*(normalAnalysisCenter.jobs-normalAnalysisCenter.jobsInQueue);
                normalAnalysisCenter.queueArea+=(events.arrivals->time-normalAnalysisCenter.lastEventTime)*normalAnalysisCenter.jobsInQueue;
                handleNormalArrival(&normalAnalysisCenter, &events);
                break;
            case CENTER_PREMIUM:
                premiumAnalysisCenter.area+=(events.arrivals->time-premiumAnalysisCenter.lastEventTime)*premiumAnalysisCenter.jobs;
                premiumAnalysisCenter.serviceArea+=(events.arrivals->time-premiumAnalysisCenter.lastEventTime)*(premiumAnalysisCenter.jobs-premiumAnalysisCenter.jobsInQueue);
                premiumAnalysisCenter.queueArea+=(events.arrivals->time-premiumAnalysisCenter.lastEventTime)*(premiumAnalysisCenter.jobsInQueue);
                handlePremiumArrival(&premiumAnalysisCenter, &events);
                break;
            case CENTER_RELIABLE:
                reliableAnalysisCenter.area+=(events.arrivals->time-reliableAnalysisCenter.lastEventTime)*reliableAnalysisCenter.jobs;
                reliableAnalysisCenter.serviceArea+=(events.arrivals->time-reliableAnalysisCenter.lastEventTime)*(reliableAnalysisCenter.jobs-(reliableAnalysisCenter.jobsInQueueNormal+reliableAnalysisCenter.jobsInQueuePremium));
                reliableAnalysisCenter.queueArea+=(events.arrivals->time-reliableAnalysisCenter.lastEventTime)*(reliableAnalysisCenter.jobsInQueueNormal+reliableAnalysisCenter.jobsInQueuePremium);
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
                }
                handleReliableArrival(&reliableAnalysisCenter, &events);
            }
        }
        else{
            // Next event is a termination
            switch (events.terminations->center){
                // Before calling the appropriate function to handle the event, update areas used to calculate statistics
                // Areas to be updated : time-integrated number of jobs in center, time-integrated number of jobs in queue, time-integrated number of jobs in service
            case CENTER_DIGEST:
                digestCenter.area+=(events.terminations->time-digestCenter.lastEventTime)*digestCenter.jobs;
                digestCenter.serviceArea+=(events.terminations->time-digestCenter.lastEventTime)*(digestCenter.jobs-digestCenter.jobsInQueue);
                digestCenter.queueArea+=(events.terminations->time-digestCenter.lastEventTime)*digestCenter.jobsInQueue;
                handleDigestTermination( &digestCenter, &events);
                break;
            case CENTER_NORMAL:
                normalAnalysisCenter.area+=(events.terminations->time-normalAnalysisCenter.lastEventTime)*normalAnalysisCenter.jobs;
                normalAnalysisCenter.serviceArea+=(events.terminations->time-normalAnalysisCenter.lastEventTime)*(normalAnalysisCenter.jobs-normalAnalysisCenter.jobsInQueue);
                normalAnalysisCenter.queueArea+=(events.terminations->time-normalAnalysisCenter.lastEventTime)*normalAnalysisCenter.jobsInQueue;
                handleNormalTermination(&normalAnalysisCenter, &events, &digestCenter);
                break;
            case CENTER_PREMIUM:
                premiumAnalysisCenter.area+=(events.terminations->time-premiumAnalysisCenter.lastEventTime)*premiumAnalysisCenter.jobs;
                premiumAnalysisCenter.serviceArea+=(events.terminations->time-premiumAnalysisCenter.lastEventTime)*(premiumAnalysisCenter.jobs-premiumAnalysisCenter.jobsInQueue);
                premiumAnalysisCenter.queueArea+=(events.terminations->time-premiumAnalysisCenter.lastEventTime)*(premiumAnalysisCenter.jobsInQueue);
                handlePremiumTermination(&premiumAnalysisCenter, &events, &digestCenter);
                break;
            case CENTER_RELIABLE:
                reliableAnalysisCenter.area+=(events.terminations->time-reliableAnalysisCenter.lastEventTime)*reliableAnalysisCenter.jobs;
                reliableAnalysisCenter.serviceArea+=(events.terminations->time-reliableAnalysisCenter.lastEventTime)*(reliableAnalysisCenter.jobs-(reliableAnalysisCenter.jobsInQueueNormal+reliableAnalysisCenter.jobsInQueuePremium));
                reliableAnalysisCenter.queueArea+=(events.terminations->time-reliableAnalysisCenter.lastEventTime)*(reliableAnalysisCenter.jobsInQueueNormal+reliableAnalysisCenter.jobsInQueuePremium);
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
                handleReliableTermination(&reliableAnalysisCenter, &events, &digestCenter);
            }
        }
    }

    // struct used to contain statistics
    stats statistics;
    statistics.realSimulationTime=simulationTime;                                       // final value of the simulation clock
    statistics.numJobs = digestCenter.index;                                            // jobs processed by the entire system (all of them pass through the digest center)  
    statistics.numNormalJobs = digestCenter.index - digestCenter.indexPremium;          // jobs processed and submitted by normal users
    statistics.numPremiumJobs = digestCenter.indexPremium;                              // jobs processed and submitted by premium users

    // Output file for simulation statistics
    FILE* file = fopen(filename,"a+");
    fprintf(file,"%d,",runNumber);
    //printf("Real simulation time: %6.2f\n\n", simulationTime);
    // Digest center
    double digestResponseTime = digestCenter.area / digestCenter.index;
    /*
    printf("Area: %6.2f\n",digestCenter.area);
    printf("Area is negativa: %d\n",digestCenter.area<0.0);
    printf("\nCenter 1 : Digest center\n");
    printf("Number of processed jobs : %d\n", digestCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", digestCenter.area / simulationTime);
    printf("Average response time : %6.2f\n", digestResponseTime);
    */
    double digestRo = (digestCenter.serviceArea/digestCenter.index)/(digestCenter.interarrivalTime/digestCenter.index);
    //printf("Utilization : %6.2f\n",digestRo);

    double delayArea = digestCenter.queueArea;
    //printf("Average wait time : %6.2f\n", delayArea / digestCenter.index);
    //printf("Job with a matching digest: %d (percentage : %6.2f)\n",
     //digestCenter.digestMatching, (double)digestCenter.digestMatching / digestCenter.index);
    statistics.numDigestMatching=digestCenter.digestMatching;
    statistics.responseTime[0]=digestResponseTime;
    statistics.waitTime[0] = delayArea/digestCenter.index;
    statistics.serviceTime[0] = digestCenter.serviceArea/digestCenter.index;
    statistics.interarrivalTime[0] = digestCenter.interarrivalTime/digestCenter.index;
    statistics.avgNumberOFJobs[0] = digestCenter.area/simulationTime;
    
    // Normal analysis center
    double normalResponseTime = normalAnalysisCenter.area / normalAnalysisCenter.index;
    /*
    printf("\nCenter 2 : Normal analysis center\n");
    printf("Number of processed jobs : %d\n", normalAnalysisCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", normalAnalysisCenter.area / simulationTime);
    printf("Average response time : %6.6f\n", normalResponseTime);
*/
    delayArea = normalAnalysisCenter.area-normalAnalysisCenter.serviceArea;
/*
    for (int i = 0; i < N_NORMAL; i++)
    {
        delayArea -= normalAnalysisCenter.service_time[i];
    }
    */
    double meanServiceTime = 0.0;
    for (int k = 0; k < N_NORMAL; k++)
    {
        meanServiceTime += normalAnalysisCenter.service_time[k];
    }
    meanServiceTime = meanServiceTime / normalAnalysisCenter.index;

    meanServiceTime=0.0;
    double means[N_NORMAL];
    int servers_used = 0;
    for (int k = 0; k < N_NORMAL; k++){
        if(normalAnalysisCenter.indexes[k]==0){
            means[k]=0.0;
            
        }
        else{
            means[k] = normalAnalysisCenter.service_time[k]/normalAnalysisCenter.indexes[k];
            servers_used++;
        }
    }
    for (int k = 0; k < N_NORMAL; k++){
        meanServiceTime+=means[k];
    }
    meanServiceTime = meanServiceTime/servers_used;
    double rho =  meanServiceTime/((normalAnalysisCenter.interarrivalTime*N_NORMAL)/normalAnalysisCenter.index);
    /*
    printf("Mean service time with area: %6.6f\n",normalAnalysisCenter.serviceArea/normalAnalysisCenter.index);
    printf("Utilization : %6.2f\n", rho);
    printf("Average service time : %6.6f\n", meanServiceTime);
    printf("Average interarrival time: %6.6f\n",normalAnalysisCenter.interarrivalTime/normalAnalysisCenter.index);
    printf("Average wait time : %6.6f\n", delayArea / normalAnalysisCenter.index);
    printf("Verificato = %6.6f = %6.6f+%6.6f = %6.6f\n",normalResponseTime, delayArea/normalAnalysisCenter.index,meanServiceTime,delayArea/normalAnalysisCenter.index+meanServiceTime);
    printf("Number of termination due to timeout expiration: %d (percentage : %6.6f)\n",
     normalAnalysisCenter.numberOfTimeouts, (double)normalAnalysisCenter.numberOfTimeouts / normalAnalysisCenter.index);
    printf("Tempo di risposta medio corretto = %6.6f\n Tempo di attesa medio corretto = %6.6f\n",normalAnalysisCenter.meanResponseTime/normalAnalysisCenter.index,normalAnalysisCenter.meanQueueTime/normalAnalysisCenter.index);
        printf("Verificato = %6.6f = %6.20f+%6.6f = %d\n",normalAnalysisCenter.meanResponseTime/normalAnalysisCenter.index,normalAnalysisCenter.meanQueueTime/normalAnalysisCenter.index,meanServiceTime,normalAnalysisCenter.meanQueueTime/normalAnalysisCenter.index+meanServiceTime == normalAnalysisCenter.meanResponseTime/normalAnalysisCenter.index);
    */
   statistics.responseTime[1]=normalResponseTime;
    statistics.waitTime[1] = delayArea/normalAnalysisCenter.index;
    statistics.serviceTime[1] = normalAnalysisCenter.serviceArea/normalAnalysisCenter.index;
    statistics.interarrivalTime[1] = normalAnalysisCenter.interarrivalTime/normalAnalysisCenter.index;
    statistics.avgNumberOFJobs[1] = normalAnalysisCenter.area/simulationTime;
    statistics.numOfTimeouts[0] = normalAnalysisCenter.numberOfTimeouts;
    // Premium analysis center
    double premiumResponseTime = premiumAnalysisCenter.area / premiumAnalysisCenter.index;
    /*
    printf("\nCenter 3 : Premium analysis center\n");
    printf("Number of processed jobs : %d\n", premiumAnalysisCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", premiumAnalysisCenter.area / simulationTime);
    printf("Average response time : %6.2f\n", premiumResponseTime);
    */
    delayArea = premiumAnalysisCenter.queueArea;
    meanServiceTime=premiumAnalysisCenter.serviceArea;
    double premiumRho = meanServiceTime/(premiumAnalysisCenter.interarrivalTime*N_PREMIUM);
    /*
    printf("Utilization : %6.2f\n",premiumRho);
    printf("Average service time: %6.6f\n", meanServiceTime/premiumAnalysisCenter.index);
    printf("Average wait time : %6.2f\n", delayArea / premiumAnalysisCenter.index);
    printf("Number of termination due to timeout expiration: %d (percentage : %6.6f)\n",
     premiumAnalysisCenter.numberOfTimeouts, (double)premiumAnalysisCenter.numberOfTimeouts / premiumAnalysisCenter.index);
     */
    statistics.responseTime[2]=premiumResponseTime;
    statistics.waitTime[2] = delayArea/premiumAnalysisCenter.index;
    statistics.serviceTime[2] = premiumAnalysisCenter.serviceArea/premiumAnalysisCenter.index;
    statistics.interarrivalTime[2] = premiumAnalysisCenter.interarrivalTime/premiumAnalysisCenter.index;
    statistics.avgNumberOFJobs[2] = premiumAnalysisCenter.area/simulationTime;
    statistics.numOfTimeouts[1] = premiumAnalysisCenter.numberOfTimeouts;
    // Reliable analysis center
    double reliableResponseTime = reliableAnalysisCenter.area / reliableAnalysisCenter.index;
    double reliablePremiumresponseTime = reliableAnalysisCenter.areaPremium / reliableAnalysisCenter.premiumIndex;
    /*
    printf("\nCenter 4 : Reliable analysis center\n");
    printf("Number of processed jobs : %d\n", reliableAnalysisCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", reliableAnalysisCenter.area / simulationTime);
    printf("Mean number of PREMIUM jobs in the center : %6.2f\n", reliableAnalysisCenter.areaPremium / simulationTime);
    printf("Average PREMIUM class response time : %6.2f\n", reliablePremiumresponseTime);
    printf("Average response time : %6.2f\n", reliableResponseTime);
    printf("Average NORMAL response time: %6.2f\n",reliableAnalysisCenter.areaNormal/reliableAnalysisCenter.normalIndex);
    */
    delayArea = reliableAnalysisCenter.queueArea;
    double delayAreaPremium = reliableAnalysisCenter.queueAreaPremium;
    double delayAreaNormal = reliableAnalysisCenter.queueAreaNormal;
    //printf("Area premium = %6.2f\n",delayAreaPremium);
    meanServiceTime=reliableAnalysisCenter.serviceArea;


    double reliableRho = meanServiceTime/(reliableAnalysisCenter.interarrivalTime*N_RELIABLE);
    /*
    printf("Mean service time: %6.6f\n",meanServiceTime/reliableAnalysisCenter.index);
    printf("Utilization: %6.2f\n",reliableRho);
    printf("Average PREMIUM class wait time : %6.2f\n", delayAreaPremium / reliableAnalysisCenter.premiumIndex);
    printf("Average NORMAL class wait time : %6.2f\n", delayAreaNormal / reliableAnalysisCenter.normalIndex);
    printf("Average wait time : %6.2f\n", delayArea / reliableAnalysisCenter.index);
    printf("Number of termination due to timeout expiration: %d (percentage : %6.6f)\n",
     reliableAnalysisCenter.numberOfTimeouts, (double)reliableAnalysisCenter.numberOfTimeouts / reliableAnalysisCenter.index);
     */
    statistics.responseTime[3]=reliableResponseTime;
    statistics.waitTime[3] = delayArea/reliableAnalysisCenter.index;
    statistics.serviceTime[3] = reliableAnalysisCenter.serviceArea/reliableAnalysisCenter.index;
    statistics.interarrivalTime[3] = reliableAnalysisCenter.interarrivalTime/reliableAnalysisCenter.index;
    statistics.avgNumberOFJobs[3] = reliableAnalysisCenter.area/simulationTime;
    statistics.numOfTimeouts[2] = reliableAnalysisCenter.numberOfTimeouts;
    // Global performances
    //printf("\nGlobal performances\n");
    double globalWaitTime = (digestCenter.queueArea + normalAnalysisCenter.queueArea + premiumAnalysisCenter.queueArea + reliableAnalysisCenter.queueArea)/digestCenter.index;
    double globalPremiumWaitTime = (digestCenter.queueArea + premiumAnalysisCenter.queueArea + reliableAnalysisCenter.queueAreaPremium) / digestCenter.indexPremium;
    double globalNormalWaitTime = (digestCenter.queueArea +normalAnalysisCenter.queueArea+reliableAnalysisCenter.queueAreaNormal)/(digestCenter.index-digestCenter.indexPremium);
    double globalResponseTime = (digestCenter.area + normalAnalysisCenter.area + premiumAnalysisCenter.area + reliableAnalysisCenter.area) / digestCenter.index;
    double globalPremiumResponseTime = (digestCenter.area  + premiumAnalysisCenter.area + reliableAnalysisCenter.areaPremium) / digestCenter.indexPremium;
    double globalNormalResponseTime = (digestCenter.area +normalAnalysisCenter.area+reliableAnalysisCenter.areaNormal)/(digestCenter.index-digestCenter.indexPremium);
    //printf("Global waiting time : %6.6f\nGlobal premium waiting time : %6.6f\nGlobal normal waiting time : %6.6f\n",globalWaitTime,globalPremiumWaitTime,globalNormalWaitTime);
    //printf("Global response time : %6.6f\nGlobal premium response time : %6.6f\nGlobal normal response time : %6.6f\n",globalResponseTime,globalPremiumResponseTime,globalNormalResponseTime);
    double percentageFailure = (double) reliableAnalysisCenter.numberOfTimeouts / digestCenter.index;
    //printf("Global failure percentage : %6.6f\n", percentageFailure);

    //printf("Verify:\n");
    //verify(&digestCenter,&normalAnalysisCenter,&premiumAnalysisCenter,&reliableAnalysisCenter);
    fprintf(file,"%d,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f,%6.6f\n",statistics.numDigestMatching,
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
    statistics.numOfTimeouts[2]);
    fclose(file);
    return statistics;
}


/* use 0.95 for 95% confidence */

double* welford(double confidence, double *statistics, int size){
  long   n    = 0;                     /* counts data points */
  double sum  = 0.0;
  double mean = 0.0;
  double data;
  double stdev;
  double u, t, w;
  double diff;

  for(int i = 0; i<size; i++) {                 /* use Welford's one-pass method */
    data = statistics[i];               /* to calculate the sample mean  */
    n++;                                 /* and standard deviation        */
    diff  = data - mean;
    sum  += diff * diff * (n - 1.0) / n;
    mean += diff / n;
  }
  stdev  = sqrt(sum / n);

  if (n > 1) {
    u = 1.0 - 0.5 * (1.0 - confidence);              /* interval parameter  */
    t = idfStudent(n - 1, u);                 /* critical value of t */
    w = t * stdev / sqrt(n - 1);              /* interval half width */
    //printf("\nbased upon %ld data points", n);
    //printf(" and with %d%% confidence\n", (int) (100.0 * confidence + 0.5));
    //printf("the expected value is in the interval");
    //printf("%10.2f +/- %6.2f\n", mean, w);
    double *array = malloc(sizeof(double)*2);
    array[0] = round(1000000 * mean)/ 1000000.0;
    array[1] = round(1000000 * w) / 1000000.0;
    return array;
  }
  else
    printf("ERROR - insufficient data\n");
  return (0);
}


/**
 * @brief The main function. It starts the simulation.
 * 
 * @return int 
 */
int main(){
    FILE* f = fopen("simulation_stats.csv","w+");
    stats statistics[ITERATIONS];
    avgStats averageAmongRuns;
    fprintf(f,"#RUN,Digest Matching, Service time Digest, Service time Normal, Service time Premium, Service time Reliable,Response time Digest, Response time Normal, Response time Premium, Response time Reliable, Wait time Digest, Wait time Normal, Wait time Premium, Wait time Reliable,Interarrival time Digest, Interarrival time Normal, Interarrival time Premium, Interarrival time Reliable, Avg num of jobs Digest, Avg num of jobs Normal, Avg num of jobs Premium, Avg num of jobs Reliable, Num of timeouts Normal, Num of timeouts Premium, Num of timeouts Reliable\n");
    fclose(f);
    if (FINITE_HORIZON){
        printf ("Finite Horizon simulation with %d runs\n\n", ITERATIONS);
        for (int i = 0; i < ITERATIONS; i++)
        {
            statistics[i] = oneTimeSimulation(i,"simulation_stats.csv");
            if ((i+1)%10 == 0 || i+1 == ITERATIONS){
                printf("Run %d DONE\n", i+1);
            }
        }

        // Calculate mean values of the statistics recovered through the ITERATIONS runs
        double confidence = 0.95;
        double array[ITERATIONS];
        double *results;
        double **confidenceIntervals = malloc(sizeof(double*) * 27);

        printf("\n\nConfidence intervals:\n\n");
        // num jobs
        for (int i = 0; i < ITERATIONS; i++){
            array[i] = (double) statistics[i].numJobs;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[0] = results;
        printf("Num jobs processed : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        free(results);

        // num normal jobs
        for (int i = 0; i < ITERATIONS; i++){
            array[i] = (double) statistics[i].numNormalJobs;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[1] = results;
        printf("Num normal jobs processed : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        free(results);

        // num premium jobs
        for (int i = 0; i < ITERATIONS; i++){
            array[i] = (double) statistics[i].numPremiumJobs;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[2] = results;
        printf("Num premium jobs processed : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        free(results);

        // num digest Matching
        for (int i = 0; i < ITERATIONS; i++){
            array[i] = (double) statistics[i].numDigestMatching;
        }
        results = welford(confidence, array, ITERATIONS);
        confidenceIntervals[3] = results;
        printf("Num digest matching : %6.6f +/- %6.6f jobs\n", results[0], results[1]);
        free(results);
        
        // response times
        for(int j = 0; j<4; j++){
            for (int i = 0; i < ITERATIONS; i++){
                array[i] = (double) statistics[i].responseTime[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Response time center %d : %6.6f +/- %6.6f sec\n",j+1, results[0], results[1]);
            free(results);
        }

        // wait times
        for(int j = 0; j<4; j++){
            for (int i = 0; i < ITERATIONS; i++){
                array[i] = (double) statistics[i].waitTime[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Wait time center %d : %6.6f +/- %6.6f sec\n",j+1, results[0], results[1]);
            free(results);
        }

        // service times
        for(int j = 0; j<4; j++){
            for (int i = 0; i < ITERATIONS; i++){
                array[i] = (double) statistics[i].serviceTime[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Service time center %d : %6.6f +/- %6.6f sec\n",j+1, results[0], results[1]);
            free(results);
        }

        // interarrival times
        for(int j = 0; j<4; j++){
            for (int i = 0; i < ITERATIONS; i++){
                array[i] = (double) statistics[i].interarrivalTime[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Inter-arrival time center %d : %6.6f +/- %6.6f sec\n",j+1, results[0], results[1]);
            free(results);
        }

        // average number of jobs in center
        for(int j = 0; j<4; j++){
            for (int i = 0; i < ITERATIONS; i++){
                array[i] = (double) statistics[i].avgNumberOFJobs[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Average number of jobs in the center %d : %6.6f +/- %6.6f jobs\n",j+1, results[0], results[1]);
            free(results);
        }

        // timeouts
        for(int j = 0; j<3; j++){
            for (int i = 0; i < ITERATIONS; i++){
                array[i] = (double) statistics[i].numOfTimeouts[j];
            }
            results = welford(confidence, array, ITERATIONS);
            confidenceIntervals[3] = results;
            printf("Average number of timeouts in the center %d : %6.6f +/- %6.6f jobs\n",j+1, results[0], results[1]);
            free(results);
        }
        
        //init struct
        averageAmongRuns.numJobs = 0.0;
        averageAmongRuns.numNormalJobs = 0.0;
        averageAmongRuns.numPremiumJobs = 0.0;
        averageAmongRuns.numDigestMatching = 0.0;
        for(int j = 0; j<4; j++){
            averageAmongRuns.responseTime[j] = 0.0;
            averageAmongRuns.waitTime[j] = 0.0;
            averageAmongRuns.serviceTime[j] = 0.0;
            averageAmongRuns.interarrivalTime[j] = 0.0;
            averageAmongRuns.avgNumberOFJobs[j] = 0.0;
            if (j!= 0){
                averageAmongRuns.numOfTimeouts[j-1] = 0.0;
            }
        }

        //fill struct
        for (int i=0; i < ITERATIONS; i++){
            averageAmongRuns.numJobs += statistics[i].numJobs;
            averageAmongRuns.numNormalJobs += statistics[i].numNormalJobs;
            averageAmongRuns.numPremiumJobs += statistics[i].numPremiumJobs;
            averageAmongRuns.numDigestMatching += statistics[i].numDigestMatching;
            for(int j = 0; j<4; j++){
                averageAmongRuns.responseTime[j] += statistics[i].responseTime[j];
                averageAmongRuns.waitTime[j] += statistics[i].waitTime[j];
                averageAmongRuns.serviceTime[j] += statistics[i].serviceTime[j];
                averageAmongRuns.interarrivalTime[j] += statistics[i].interarrivalTime[j];
                averageAmongRuns.avgNumberOFJobs[j] += statistics[i].avgNumberOFJobs[j];
                if (j!= 0){
                    averageAmongRuns.numOfTimeouts[j-1] += statistics[i].numOfTimeouts[j-1];
                }
            }
        }
        averageAmongRuns.numJobs /= (double) ITERATIONS;
        averageAmongRuns.numNormalJobs /= (double) ITERATIONS;
        averageAmongRuns.numPremiumJobs /= (double) ITERATIONS;
        averageAmongRuns.numDigestMatching /= (double) ITERATIONS;
        for(int j = 0; j<4; j++){
            averageAmongRuns.responseTime[j] /= (double) ITERATIONS;
            averageAmongRuns.waitTime[j] /= (double) ITERATIONS;
            averageAmongRuns.serviceTime[j] /= (double) ITERATIONS;
            averageAmongRuns.interarrivalTime[j] /= (double) ITERATIONS;
            averageAmongRuns.avgNumberOFJobs[j] /= (double) ITERATIONS;
            if (j!= 0){
                averageAmongRuns.numOfTimeouts[j-1] /= (double) ITERATIONS;
            }
        }
    }
    
}



void handleDigestArrival( digestCenter *digestCenter, event_list *ev)
{
    arrival* ar = ev->arrivals;
    //digestCenter->area += (ar->time - digestCenter->lastEventTime) * digestCenter->jobs;
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
        //digestCenter->queueArea += (ar->time - lastEventTime) * digestCenter->jobsInQueue;
        digestCenter->jobsInQueue++;
        job_queue *node = malloc(sizeof(job_queue));
        node->job = ar->job;
        insertQueue(&(digestCenter->queue), node);
    }
    // Generate a new arrival and insert into the list of events, only if the simuation time is not over
    if (simulationTime < OBSERVATION_PERIOD){
        arrival *newArrival = getArrival();
        digestCenter->interarrivalTime+=newArrival->time-simulationTime;
        insertList(ev, newArrival, 0);
    }

    free(ar);
}

void handleDigestTermination(digestCenter *digestCenter, event_list *ev)
{
    termination* term = ev->terminations;
    //digestCenter->area += (term->time - digestCenter->lastEventTime) * digestCenter->jobs;
    digestCenter->jobs--;
    digestCenter->index++;
    if (term->job.userType == PREMIUM){
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
        arrival *ar = malloc(sizeof(arrival));
        if(ar==NULL){
            printf("Memory leak\n");
            exit(0);
        }
        switch (term->job.userType){
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
    else
    {
        digestCenter->digestMatching++;
    }
    if (digestCenter->jobs >= 1)
    {
        //digestCenter->queueArea += (term->time - lastEventTime) * digestCenter->jobsInQueue;
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
    if(center->lastArrivalTime!=0.0)
        center->interarrivalTime+=ar->time-center->lastArrivalTime;
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
        if(center->jobsInQueue!=sizeQueue(center->queue)){
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
    center->meanResponseTime+=ter->time-ter->job.arrivalTime;
    if (ter->job.serviceTime > TIMEOUT)
    {
        center->numberOfTimeouts++;
        arrival *ar = malloc(sizeof(arrival));
        ar->center = CENTER_RELIABLE;
        ar->time = simulationTime;
        ar->job = ter->job;
        // Service time is decreased by a factor, because reliable machines are faster than normal machines
        double factor = (double) PREMIUM_MEAN_SERVICE_TIME/NORMAL_MEAN_SERVICE_TIME;
        ar->job.serviceTime = ar->job.serviceTime * factor;
        insertList(ev, ar, 0);
    }else if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB){
        digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;
    }
    center->servers[ter->server_index] = 0;
    if (center->jobs >= N_NORMAL)
    {
        center->jobsInQueue--;
        int server_index = findFreeServer(center->servers, N_NORMAL);
        center->servers[server_index] = 1;
        job_queue *j_q = popQueue(&center->queue);
        if(center->jobsInQueue!=sizeQueue(center->queue)){
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
    //center->area += (ar->time - center->lastEventTime) * center->jobs;
    center->jobs++;
    simulationTime = ar->time;
    if(center->lastArrivalTime!=0.0){
        center->interarrivalTime+=ar->time-center->lastArrivalTime;
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
        if(term==NULL){
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
        //center->queueArea += (ar->time - lastEventTime) * center->jobsInQueue;
        center->jobsInQueue++;   
        job_queue *node = malloc(sizeof(job_queue));

        if(node==NULL){
            printf("Memory leak\n");
            exit(0);
        }
        node->job = ar->job;

        insertQueue(&(center->queue), node);
        if(center->jobsInQueue!=sizeQueue(center->queue)){
            printf("FATAL ERROR\n");
            exit(0);
        }

    }
    free(ar);
}

void handlePremiumTermination(premiumAnalysisCenter *center, event_list *ev, digestCenter *digestCenter)
{
    termination *ter = ev->terminations;
    //center->area += (ter->time - center->lastEventTime) * center->jobs;
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
        // Service time is the same because the machines are the same.
        ar->job.serviceTime = ar->job.serviceTime;
        insertList(ev, ar, 0);
    }else if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB){
        digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;
    }
    center->servers[ter->server_index] = 0;
    if (center->jobs >= N_PREMIUM)
    {
        //center->queueArea += (ter->time - lastEventTime) * center->jobsInQueue;
        center->jobsInQueue--;
        int server_index = findFreeServer(center->servers, N_PREMIUM);
        center->servers[server_index] = 1;
        job_queue *j_q = popQueue(&(center->queue));
        if(center->jobsInQueue!=sizeQueue(center->queue)){
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
    //center->area += (ar->time - center->lastEventTime) * center->jobs;
    UserType type = ar->job.userType;
    double lastEventTimeNormal = center->lastEventTimeNormal;
    double lastEventTimePremium = center ->lastEventTimePremium;
    double lastEventTime = center->lastEventTime;
    if (type == PREMIUM)
    {
        //center->areaPremium += (ar->time - center->lastEventTimePremium) * center->premiumJobs;
        center->lastEventTimePremium = ar->time;
        center->premiumJobs++;
    }
    else{
        //center->areaNormal += (ar->time - center->lastEventTimeNormal) * center->normalJobs;
        center->lastEventTimeNormal=ar->time;
        center->normalJobs++;
    }
    center->jobs++;
    simulationTime = ar->time;
    if(center->lastArrivalTime!=0.0)
        center->interarrivalTime+=ar->time-center->lastArrivalTime;
    center->lastArrivalTime = simulationTime;
    center->lastEventTime = simulationTime;
    ev->arrivals = ar->next;
    if (center->jobs <= N_RELIABLE)
    {
        int server_index = findFreeServer(center->servers, N_RELIABLE);
        center->servers[server_index] = 1;
        // The actual service time can be less than the generated service time due to timeout expiration
        center->service_time[server_index] += min(ar->job.serviceTime, TIMEOUT_RELIABLE);
        if(type==PREMIUM){
            center->service_time_premium[server_index] += min(ar->job.serviceTime, TIMEOUT_RELIABLE);
        }
        termination *term = malloc(sizeof(termination));
        term->time = min(ar->job.serviceTime, TIMEOUT_RELIABLE) + simulationTime;
        term->center = CENTER_RELIABLE;
        term->job = ar->job;
        term->server_index = server_index;
        insertList(ev, term, 1);
    }
    else
    {
        job_queue *j_q;
        job_queue *node = malloc(sizeof(job_queue));
        if(node == NULL){
            printf("Memory leak\n");
            exit(0);
        }
        node->job = ar->job;
        //center->queueArea += (ar->time - lastEventTime) * (center->jobsInQueueNormal + center->jobsInQueuePremium);
        if (type == PREMIUM)
        {
            //center->queueAreaPremium += (ar->time - lastEventTimePremium) * center->jobsInQueuePremium;
            center->jobsInQueuePremium++;
            insertQueue(&(center->queuePremium), node);
        }
        else
        {
            //center->queueAreaNormal += (ar->time - lastEventTimeNormal) * center->jobsInQueueNormal;
            center->jobsInQueueNormal++;
            insertQueue(&(center->queueNormal),node);
        }
        
    }
    free(ar);
}

void handleReliableTermination(reliableAnalysisCenter *center, event_list *ev, digestCenter *digestCenter)
{
    termination *ter = ev->terminations;
    //center->area += (ter->time - center->lastEventTime) * center->jobs;
    UserType type = ter->job.userType;
    double lastEventTimeNormal = center->lastEventTimeNormal;
    double lastEventTimePremium = center ->lastEventTimePremium;
    double lastEventTime = center->lastEventTime;
    if (type == PREMIUM)
    {
        //center->areaPremium += (ter->time - center->lastEventTimePremium) * center->premiumJobs;
        center->lastEventTimePremium = ter->time;
        center->premiumJobs--;
        center->premiumIndex++;
    }
    else{
        //center->areaNormal += (ter->time - center->lastEventTimeNormal) * center->normalJobs;
        center->lastEventTimeNormal=ter->time;
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
    }else{
        if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB){
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

        //center->queueArea += (ter->time - lastEventTime) * (center->jobsInQueueNormal + center->jobsInQueuePremium);
        if (center->queuePremium == NULL)
        {
            // job of the normal queue can be processed
            //center->queueAreaNormal += (ter->time - lastEventTimeNormal) * center->jobsInQueueNormal;
            center->jobsInQueueNormal--;
            job_queue *j_q = popQueue(&(center->queueNormal));
            job = j_q->job;
            free(j_q);
        }
        else
        {
            //center->queueAreaPremium += (ter->time - lastEventTimePremium) * center->jobsInQueuePremium;
            center->jobsInQueuePremium--;
            job_queue *j_q = popQueue(&(center->queuePremium));
            job = j_q->job;
            free(j_q);
        }

        // The actual service time can be less than the generated service time due to timeout expiration
        center->service_time[server_index] += min(job.serviceTime, TIMEOUT_RELIABLE);
        if(type==PREMIUM){
            center->service_time_premium[server_index] += min(ter->job.serviceTime, TIMEOUT_RELIABLE);
        }
        termination *termi = malloc(sizeof(termination));
        termi->time = min(job.serviceTime, TIMEOUT_RELIABLE) + simulationTime;
        termi->center = CENTER_RELIABLE;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
}

/**
 * @brief This function is used to do the VERIFICATION of the system. The checks done are:
 *          - Check that service time = wait time (queue) + service time, for each center and for the two separate queues of the reliable center
 *          - Check 
 * 
 * @param digestCenter 
 * @param normalCenter 
 * @param premiumCenter 
 * @param reliableCenter 
 */
void verify(digestCenter *digestCenter, normalAnalysisCenter *normalCenter, premiumAnalysisCenter *premiumCenter, reliableAnalysisCenter *reliableCenter){

    double responseTime = digestCenter->area / digestCenter->index;
    double waitTime = digestCenter->queueArea / digestCenter->index;
    double serviceTime = digestCenter->serviceArea / digestCenter->index;
    printf("Verify for digest calculation center: \n");
    printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n",waitTime,serviceTime,responseTime);
    printf("Condition is satisfied: %d\n",round(1000000*responseTime)/1000000==round(1000000*(waitTime+serviceTime))/1000000);
    responseTime = normalCenter->area / normalCenter->index;
    waitTime = normalCenter->queueArea / normalCenter->index;
    serviceTime = normalCenter->serviceArea / normalCenter->index;
    printf("Verify for normal center: \n");
    printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n",waitTime,serviceTime,responseTime);
    printf("Condition is satisfied: %d\n",round(1000000*responseTime)/1000000==round(1000000*(waitTime+serviceTime))/1000000);
    responseTime = premiumCenter->area / premiumCenter->index;
    waitTime = premiumCenter->queueArea / premiumCenter->index;
    serviceTime = premiumCenter->serviceArea / premiumCenter->index;
    printf("Verify for premium center: \n");
    printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n",waitTime,serviceTime,responseTime);
    printf("Condition is satisfied: %d\n",round(1000000*responseTime)/1000000==round(1000000*(waitTime+serviceTime))/1000000);
    responseTime = reliableCenter->area / reliableCenter->index;
    waitTime = reliableCenter->queueArea / reliableCenter->index;
    serviceTime = reliableCenter->serviceArea / reliableCenter->index;
    printf("Verify for reliable center: \n");
    printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n",waitTime,serviceTime,responseTime);
    printf("Condition is satisfied: %d\n",round(1000000*responseTime)/1000000==round(1000000*(waitTime+serviceTime))/1000000);
    responseTime = reliableCenter->areaPremium / reliableCenter->premiumIndex;
    waitTime = reliableCenter->queueAreaPremium / reliableCenter->premiumIndex;
    serviceTime = reliableCenter->serviceAreaPremium / reliableCenter->premiumIndex;
    printf("Verify for reliable center (Premium class): \n");
    printf("E(Ts1) = E(Tq1) + E(s1) -> %6.6f + %6.6f = %6.6f\n",waitTime,serviceTime,responseTime);
    printf("Condition is satisfied: %d\n",round(1000000*responseTime)/1000000==round(1000000*(waitTime+serviceTime))/1000000);
    responseTime = reliableCenter->areaNormal / reliableCenter->normalIndex;
    waitTime = reliableCenter->queueAreaNormal / reliableCenter->normalIndex;
    serviceTime = reliableCenter->serviceAreaNormal / reliableCenter->normalIndex;
    printf("Verify for reliable center (Normal class): \n");
    printf("E(Ts2) = E(Tq2) + E(s2) -> %6.6f + %6.6f = %6.6f\n",waitTime,serviceTime,responseTime);
    printf("Condition is satisfied: %d\n",round(1000000*responseTime)/1000000==round(1000000*(waitTime+serviceTime))/1000000);

    //Verify routing probabilities

    double probabilityPremium = round(100*(double)digestCenter->indexPremium/digestCenter->index)/100;
    double probabilityNormal = round(100*((double)digestCenter->index-digestCenter->indexPremium)/digestCenter->index)/100;

    printf("Verify that P(Job is submitted by a Premium user) = %6.2f\n",PROBABILITY_PREMIUM);
    printf("Probability computed = %6.2f\n",probabilityPremium);
    printf("Condition is satisfied: %d\n",probabilityPremium==PROBABILITY_PREMIUM);
        printf("Verify that P(Job is submitted by a Normal user) = %6.2f\n",1-PROBABILITY_PREMIUM);
    printf("Probability computed = %6.2f\n",probabilityNormal);
    printf("Condition is satisfied: %d\n",probabilityNormal==round(100*(1-PROBABILITY_PREMIUM))/100);

    double probabilityDigestMatching = round(100*(double)digestCenter->digestMatching/digestCenter->index)/100;

    printf("Verify that P(Job matches digest) = %6.2f\n",FINAL_DIGEST_MATCHING_PROB);
    printf("Probability computed = %6.2f\n",probabilityDigestMatching);
    printf("Condition is satisfied: %d\n",probabilityDigestMatching==FINAL_DIGEST_MATCHING_PROB);

    double probabilityOfTimeoutPremium = round(100*((double)premiumCenter->numberOfTimeouts/premiumCenter->index))/100;
    double probabilityOfTimeoutNormal = round(100*((double)normalCenter->numberOfTimeouts/normalCenter->index))/100;
    double probabilityOfTimeoutReliable = round(100*((double)reliableCenter->numberOfTimeouts/reliableCenter->index))/100;

    double expectedTimeoutPremium = round(100*exp(-(double)TIMEOUT/PREMIUM_MEAN_SERVICE_TIME))/100;
    double expectedTimeoutNormal = round(100*exp(-(double)TIMEOUT/NORMAL_MEAN_SERVICE_TIME))/100;
    double expectedTimeoutReliable = round(100*exp(-(double)TIMEOUT_RELIABLE/PREMIUM_MEAN_SERVICE_TIME))/100;

    printf("Verify that P(Job is timed out| Job is in the premium center) = %6.2f\n",expectedTimeoutPremium);
    printf("Probability computed = %6.2f\n",probabilityOfTimeoutPremium);
    printf("Condition is satisfied: %d\n",probabilityOfTimeoutPremium==expectedTimeoutPremium);

    printf("Verify that P(Job is timed out| Job is in the normal center) = %6.2f\n",expectedTimeoutNormal);
    printf("Probability computed = %6.2f\n",probabilityOfTimeoutNormal);
    printf("Condition is satisfied: %d\n",probabilityOfTimeoutNormal==expectedTimeoutNormal);

/*
    printf("Verify that P(Job is timed out| Job is in the reliable center) = %6.2f\n",expectedTimeoutReliable);
    printf("Probability computed = %6.2f\n",probabilityOfTimeoutReliable);
    printf("Condition is satisfied: %d\n",probabilityOfTimeoutReliable==expectedTimeoutReliable);
   
    */
    //Verify that number of jobs in input is equals to number of jobs analyzed + number of jobs timed out
    int numberOfInput = digestCenter->index;
    int numberOfProcessed = premiumCenter->index-premiumCenter->numberOfTimeouts+normalCenter->index-normalCenter->numberOfTimeouts+digestCenter->digestMatching+reliableCenter->jobAnalyzed;
    printf("Verify that number of jobs in input is equals to number of jobs analyzed + number of jobs timed out\n");
    printf("#job in input = #processed+#timedout -> %d = %d+%d\n",numberOfInput,numberOfProcessed,reliableCenter->numberOfTimeouts);
    printf("Condition is satisfied: %d\n",numberOfProcessed+reliableCenter->numberOfTimeouts==numberOfInput);
}
