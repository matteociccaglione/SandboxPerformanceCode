#include "events_queue.h"
//#include "job.h"
//#include "config.h"
//#include "centers.h"
#include "../lib/rngs.h"
#include "../lib/rvgs.h"
#include <stdio.h>
#include <stdlib.h>

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
arrival *getArrival()
{
    /*
    This function is used to generate an arrival to the entire system:
    The arrival is only received by the first center (digest calculation)
    */
    arrival *event = malloc(sizeof(arrival));
    SelectStream(MEAN_INTERARRIVAL_STREAM);
    double inter = Exponential(MEAN_INTERARRIVAL_TIME);
    
    event->time = inter + simulationTime;
    event->center = CENTER_DIGEST;
    SelectStream(USER_PROBABILITY_STREAM);
    event->job.userType = Bernoulli(PROBABILITY_PREMIUM);
    SelectStream(MALWARE_PROBABILITY_STREAM);
    event->job.type = Bernoulli(PROBABILITY_MALWARE);
    SelectStream(DIGEST_SERVICE_TIME_STREAM);
    event->job.serviceTime = Exponential(DIGEST_MEAN_SERVICE_TIME);
    return event;
}

void insertList(event_list *ev, void *node, int type)
{
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

double min(double val1, double val2){
    if(val1<val2)
        return val1;
    return val2;
}

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

int nextEvent(event_list ev)
{
    if (ev.terminations == NULL)
    {
        return 0;
    }
    if (ev.arrivals->time < ev.terminations->time)
    {
        return 0;
    }
    return 1;
}

int isEmptyList(event_list ev)
{
    if (ev.arrivals == NULL && ev.terminations == NULL)
        return 1;
    return 0;
}

void handleDigestArrival(digestCenter *digestCenter, event_list *ev);
void handleNormalArrival(normalAnalysisCenter *center, event_list *ev);
void handlePremiumArrival(premiumAnalysisCenter *center, event_list *ev);
void handleReliableArrival(reliableAnalysisCenter *center, event_list *ev);
void handleDigestTermination( digestCenter *digestCenter, event_list *ev);
void handleNormalTermination(normalAnalysisCenter *center, event_list *ev, digestCenter *digestCenter);
void handlePremiumTermination(premiumAnalysisCenter *center, event_list *ev, digestCenter *digestCenter);
void handleReliableTermination(reliableAnalysisCenter *center, event_list *ev, digestCenter *digestCenter);
void verify(digestCenter *digestCenter, normalAnalysisCenter *normalCenter, premiumAnalysisCenter *premiumCenter, reliableAnalysisCenter *reliableCenter);

Job **jobs;
int N = 0;
int main()
{
    event_list events;
    digestCenter digestCenter;
    normalAnalysisCenter normalAnalysisCenter;
    premiumAnalysisCenter premiumAnalysisCenter;
    reliableAnalysisCenter reliableAnalysisCenter;
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

    events.arrivals = NULL;
    events.terminations = NULL;
    PlantSeeds(123456789);
    insertList(&events, getArrival(), 0);
    jobs = malloc(sizeof(Job*)*28700);
    while (simulationTime < OBSERVATION_PERIOD && !(isEmptyList(events)))
    {
        
        if (nextEvent(events) == 0)
        {
            switch (events.arrivals->center)
            {
            case CENTER_DIGEST:
                //printf("digest arrival\n");
                digestCenter.area+=(events.arrivals->time-digestCenter.lastEventTime)*digestCenter.jobs;
                digestCenter.serviceArea+=(events.arrivals->time-digestCenter.lastEventTime)*(digestCenter.jobs-digestCenter.jobsInQueue);
                digestCenter.queueArea+=(events.arrivals->time-digestCenter.lastEventTime)*digestCenter.jobsInQueue;
                handleDigestArrival(&digestCenter, &events);
                break;
            case CENTER_NORMAL:
                //printf("normal arrival\n");
                normalAnalysisCenter.area+=(events.arrivals->time-normalAnalysisCenter.lastEventTime)*normalAnalysisCenter.jobs;
                normalAnalysisCenter.serviceArea+=(events.arrivals->time-normalAnalysisCenter.lastEventTime)*(normalAnalysisCenter.jobs-normalAnalysisCenter.jobsInQueue);
                normalAnalysisCenter.queueArea+=(events.arrivals->time-normalAnalysisCenter.lastEventTime)*normalAnalysisCenter.jobsInQueue;
                handleNormalArrival(&normalAnalysisCenter, &events);
                break;
            case CENTER_PREMIUM:
                //printf("premium arrival\n");
                premiumAnalysisCenter.area+=(events.arrivals->time-premiumAnalysisCenter.lastEventTime)*premiumAnalysisCenter.jobs;
                premiumAnalysisCenter.serviceArea+=(events.arrivals->time-premiumAnalysisCenter.lastEventTime)*(premiumAnalysisCenter.jobs-premiumAnalysisCenter.jobsInQueue);
                premiumAnalysisCenter.queueArea+=(events.arrivals->time-premiumAnalysisCenter.lastEventTime)*(premiumAnalysisCenter.jobsInQueue);
                handlePremiumArrival(&premiumAnalysisCenter, &events);
                break;
            case CENTER_RELIABLE:
                //printf("reliable arrival\n");
                reliableAnalysisCenter.area+=(events.arrivals->time-reliableAnalysisCenter.lastEventTime)*reliableAnalysisCenter.jobs;
                reliableAnalysisCenter.serviceArea+=(events.arrivals->time-reliableAnalysisCenter.lastEventTime)*(reliableAnalysisCenter.jobs-(reliableAnalysisCenter.jobsInQueueNormal+reliableAnalysisCenter.jobsInQueuePremium));
                reliableAnalysisCenter.queueArea+=(events.arrivals->time-reliableAnalysisCenter.lastEventTime)*(reliableAnalysisCenter.jobsInQueueNormal+reliableAnalysisCenter.jobsInQueuePremium);
                if(events.arrivals->job.userType==PREMIUM){
                    reliableAnalysisCenter.areaPremium+=(events.arrivals->time-reliableAnalysisCenter.lastEventTime)*reliableAnalysisCenter.premiumJobs;
                reliableAnalysisCenter.serviceAreaPremium+=(events.arrivals->time-reliableAnalysisCenter.lastEventTime)*(reliableAnalysisCenter.premiumJobs-reliableAnalysisCenter.jobsInQueuePremium);
                reliableAnalysisCenter.queueAreaPremium+=(events.arrivals->time-reliableAnalysisCenter.lastEventTime)*(reliableAnalysisCenter.jobsInQueuePremium);
                }
                handleReliableArrival(&reliableAnalysisCenter, &events);
            }
        }
        else
        {
            // Termination
            switch (events.terminations->center)
            {
            case CENTER_DIGEST:
                //printf("digest term\n");
                handleDigestTermination( &digestCenter, &events);
                break;
            case CENTER_NORMAL:
                //printf("normal term\n");
                handleNormalTermination(&normalAnalysisCenter, &events, &digestCenter);
                break;
            case CENTER_PREMIUM:
                //printf("premium term\n");
                handlePremiumTermination(&premiumAnalysisCenter, &events, &digestCenter);
                break;
            case CENTER_RELIABLE:
                //printf("reliable term\n");
                handleReliableTermination(&reliableAnalysisCenter, &events, &digestCenter);
            }
        }
    }

    // print out stats
    printf("Real simulation time: %6.2f\n\n", simulationTime);

    // Digest center
    double digestResponseTime = digestCenter.area / digestCenter.index;
    printf("Area: %6.2f\n",digestCenter.area);
    printf("Area is negativa: %d\n",digestCenter.area<0.0);
    printf("\nCenter 1 : Digest center\n");
    printf("Number of processed jobs : %d\n", digestCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", digestCenter.area / simulationTime);
    printf("Average response time : %6.2f\n", digestResponseTime);
    double digestRo = (digestCenter.service_time/digestCenter.index)/(digestCenter.interarrivalTime/digestCenter.index);
    printf("Utilization : %6.2f\n",digestRo);

    double delayArea = digestCenter.queueArea;
    printf("Average wait time : %6.2f\n", delayArea / digestCenter.index);
    printf("Job with a matching digest: %d (percentage : %6.2f)\n",
     digestCenter.digestMatching, (double)digestCenter.digestMatching / digestCenter.index);

    // Normal analysis center
    double normalResponseTime = normalAnalysisCenter.area / normalAnalysisCenter.index;
    printf("\nCenter 2 : Normal analysis center\n");
    printf("Number of processed jobs : %d\n", normalAnalysisCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", normalAnalysisCenter.area / simulationTime);
    printf("Average response time : %6.6f\n", normalResponseTime);

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
    // Premium analysis center
    double premiumResponseTime = premiumAnalysisCenter.area / premiumAnalysisCenter.index;
    printf("\nCenter 3 : Premium analysis center\n");
    printf("Number of processed jobs : %d\n", premiumAnalysisCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", premiumAnalysisCenter.area / simulationTime);
    printf("Average response time : %6.2f\n", premiumResponseTime);
    delayArea = premiumAnalysisCenter.queueArea;
    meanServiceTime=0.0;
    for (int i = 0; i < N_PREMIUM; i++)
    {
        //delayArea -= premiumAnalysisCenter.service_time[i];
        meanServiceTime+=premiumAnalysisCenter.service_time[i];
    }
    double premiumRho = meanServiceTime/(premiumAnalysisCenter.interarrivalTime*N_PREMIUM);
    printf("Utilization : %6.2f\n",premiumRho);
    printf("Average service time: %6.6f\n", meanServiceTime/premiumAnalysisCenter.index);
    printf("Average wait time : %6.2f\n", delayArea / premiumAnalysisCenter.index);
    printf("Number of termination due to timeout expiration: %d (percentage : %6.6f)\n",
     premiumAnalysisCenter.numberOfTimeouts, (double)premiumAnalysisCenter.numberOfTimeouts / premiumAnalysisCenter.index);

    // Reliable analysis center
    double reliableResponseTime = reliableAnalysisCenter.area / reliableAnalysisCenter.index;
    double reliablePremiumresponseTime = reliableAnalysisCenter.areaPremium / reliableAnalysisCenter.premiumIndex;
    printf("\nCenter 4 : Reliable analysis center\n");
    printf("Number of processed jobs : %d\n", reliableAnalysisCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", reliableAnalysisCenter.area / simulationTime);
    printf("Mean number of PREMIUM jobs in the center : %6.2f\n", reliableAnalysisCenter.areaPremium / simulationTime);
    printf("Average PREMIUM class response time : %6.2f\n", reliablePremiumresponseTime);
    printf("Average response time : %6.2f\n", reliableResponseTime);
    printf("Average NORMAL response time: %6.2f\n",reliableAnalysisCenter.areaNormal/reliableAnalysisCenter.normalIndex);
    delayArea = reliableAnalysisCenter.queueArea;
    double delayAreaPremium = reliableAnalysisCenter.queueAreaPremium;
    double delayAreaNormal = reliableAnalysisCenter.queueAreaNormal;
    printf("Area premium = %6.2f\n",delayAreaPremium);
    meanServiceTime=0.0;
    for (int i = 0; i < N_RELIABLE; i++)
    {
        //delayArea -= reliableAnalysisCenter.service_time[i];
        //delayAreaPremium -= reliableAnalysisCenter.service_time_premium[i];
        meanServiceTime+=reliableAnalysisCenter.service_time[i];
    }

    double reliableRho = meanServiceTime/(reliableAnalysisCenter.interarrivalTime*N_RELIABLE);
    printf("Utilization: %6.2f\n",reliableRho);
    printf("Average PREMIUM class wait time : %6.2f\n", delayAreaPremium / reliableAnalysisCenter.premiumIndex);
    printf("Average NORMAL class wait time : %6.2f\n", delayAreaNormal / reliableAnalysisCenter.normalIndex);
    printf("Average wait time : %6.2f\n", delayArea / reliableAnalysisCenter.index);
    printf("Number of termination due to timeout expiration: %d (percentage : %6.6f)\n",
     reliableAnalysisCenter.numberOfTimeouts, (double)reliableAnalysisCenter.numberOfTimeouts / reliableAnalysisCenter.index);

    // Global performances
    printf("\nGlobal performances\n");
    double globalWaitTime = (digestCenter.queueArea + normalAnalysisCenter.queueArea + premiumAnalysisCenter.queueArea + reliableAnalysisCenter.queueArea)/digestCenter.index;
    double globalPremiumWaitTime = (digestCenter.queueArea + premiumAnalysisCenter.queueArea + reliableAnalysisCenter.queueAreaPremium) / digestCenter.indexPremium;
    double globalNormalWaitTime = (digestCenter.queueArea +normalAnalysisCenter.queueArea+reliableAnalysisCenter.queueAreaNormal)/(digestCenter.index-digestCenter.indexPremium);
    double globalResponseTime = (digestCenter.area + normalAnalysisCenter.area + premiumAnalysisCenter.area + reliableAnalysisCenter.area) / digestCenter.index;
    double globalPremiumResponseTime = (digestCenter.area  + premiumAnalysisCenter.area + reliableAnalysisCenter.areaPremium) / digestCenter.indexPremium;
    double globalNormalResponseTime = (digestCenter.area +normalAnalysisCenter.area+reliableAnalysisCenter.areaNormal)/(digestCenter.index-digestCenter.indexPremium);
    printf("Global waiting time : %6.6f\nGlobal premium waiting time : %6.6f\nGlobal normal waiting time : %6.6f\n",globalWaitTime,globalPremiumWaitTime,globalNormalWaitTime);
    printf("Global response time : %6.6f\nGlobal premium response time : %6.6f\nGlobal normal response time : %6.6f\n",globalResponseTime,globalPremiumResponseTime,globalNormalResponseTime);
    double percentageFailure = (double) reliableAnalysisCenter.numberOfTimeouts / digestCenter.index;
    printf("Global failure percentage : %6.6f\n", percentageFailure);
}

void handleDigestArrival( digestCenter *digestCenter, event_list *ev)
{
    arrival* ar = ev->arrivals;
    digestCenter->area += (ar->time - digestCenter->lastEventTime) * digestCenter->jobs;
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
        digestCenter->queueArea += (ar->time - lastEventTime) * digestCenter->jobsInQueue;
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
    digestCenter->area += (term->time - digestCenter->lastEventTime) * digestCenter->jobs;
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
        digestCenter->queueArea += (term->time - lastEventTime) * digestCenter->jobsInQueue;
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
    ar->job.jobNumber = N;
    N++;
    printf("Job %d arrivato al tempo %6.6f\n",ar->job.jobNumber,ar->job.arrivalTime);
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
    }
    free(ar);
}

void handleNormalTermination(normalAnalysisCenter *center, event_list *ev, digestCenter *digestCenter)
{
    termination *ter = ev->terminations;
    printf("Job %d terminato al tempo %6.6f con Service time %6.6f\n",ter->job.jobNumber,ter->time,min(ter->job.serviceTime,TIMEOUT));
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
        // Service time is half because reliable machines are 2x faster than normal machines
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
        Job job = j_q->job;
        printf("Job %d uscito dalla coda al tempo %6.6f\n",job.jobNumber,simulationTime);
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
    center->area += (ar->time - center->lastEventTime) * center->jobs;
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
        center->queueArea += (ar->time - lastEventTime) * center->jobsInQueue;
        center->jobsInQueue++;   
        job_queue *node = malloc(sizeof(job_queue));

        if(node==NULL){
            printf("Memory leak\n");
            exit(0);
        }
        node->job = ar->job;

        insertQueue(&(center->queue), node);

    }
    free(ar);
}

void handlePremiumTermination(premiumAnalysisCenter *center, event_list *ev, digestCenter *digestCenter)
{
    termination *ter = ev->terminations;
    center->area += (ter->time - center->lastEventTime) * center->jobs;
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
        center->queueArea += (ter->time - lastEventTime) * center->jobsInQueue;
        center->jobsInQueue--;
        int server_index = findFreeServer(center->servers, N_PREMIUM);
        center->servers[server_index] = 1;
        job_queue *j_q = popQueue(&(center->queue));
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
    center->area += (ar->time - center->lastEventTime) * center->jobs;
    UserType type = ar->job.userType;
    double lastEventTimeNormal = center->lastEventTimeNormal;
    double lastEventTimePremium = center ->lastEventTimePremium;
    double lastEventTime = center->lastEventTime;
    if (type == PREMIUM)
    {
        center->areaPremium += (ar->time - center->lastEventTimePremium) * center->premiumJobs;
        center->lastEventTimePremium = ar->time;
        center->premiumJobs++;
    }
    else{
        center->areaNormal += (ar->time - center->lastEventTimeNormal) * center->normalJobs;
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
        center->queueArea += (ar->time - lastEventTime) * (center->jobsInQueueNormal + center->jobsInQueuePremium);
        if (type == PREMIUM)
        {
            center->queueAreaPremium += (ar->time - lastEventTimePremium) * center->jobsInQueuePremium;
            center->jobsInQueuePremium++;
            insertQueue(&(center->queuePremium), node);
        }
        else
        {
            center->queueAreaNormal += (ar->time - lastEventTimeNormal) * center->jobsInQueueNormal;
            center->jobsInQueueNormal++;
            insertQueue(&(center->queueNormal),node);
        }
        
    }
    free(ar);
}

void handleReliableTermination(reliableAnalysisCenter *center, event_list *ev, digestCenter *digestCenter)
{
    termination *ter = ev->terminations;
    center->area += (ter->time - center->lastEventTime) * center->jobs;
    UserType type = ter->job.userType;
    double lastEventTimeNormal = center->lastEventTimeNormal;
    double lastEventTimePremium = center ->lastEventTimePremium;
    double lastEventTime = center->lastEventTime;
    if (type == PREMIUM)
    {
        center->areaPremium += (ter->time - center->lastEventTimePremium) * center->premiumJobs;
        center->lastEventTimePremium = ter->time;
        center->premiumJobs--;
        center->premiumIndex++;
    }
    else{
        center->areaNormal += (ter->time - center->lastEventTimeNormal) * center->normalJobs;
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
    }else if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB){
        digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;   
    }
    center->servers[ter->server_index] = 0;

    if (center->jobs >= N_RELIABLE)
    {
        int server_index = findFreeServer(center->servers, N_RELIABLE);
        center->servers[server_index] = 1;
        Job job;

        center->queueArea += (ter->time - lastEventTime) * (center->jobsInQueueNormal + center->jobsInQueuePremium);
        if (center->queuePremium == NULL)
        {
            // job of the normal queue can be processed
            center->queueAreaNormal += (ter->time - lastEventTimeNormal) * center->jobsInQueueNormal;
            center->jobsInQueueNormal--;
            job_queue *j_q = popQueue(&(center->queueNormal));
            job = j_q->job;
            free(j_q);
        }
        else
        {
            center->queueAreaPremium += (ter->time - lastEventTimePremium) * center->jobsInQueuePremium;
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

void verify(digestCenter *digestCenter, normalAnalysisCenter *normalCenter, premiumAnalysisCenter *premiumCenter, reliableAnalysisCenter *reliableCenter){

    double responseTime = digestCenter->area / digestCenter->index;
    double waitTime = digestCenter->queueArea / digestCenter->index;
    double serviceTime = digestCenter->service_time / digestCenter->index;

}
