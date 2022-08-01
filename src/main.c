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

int main()
{
    event_list events;
    digestCenter digestCenter;
    normalAnalysisCenter normalAnalysisCenter;
    premiumAnalysisCenter premiumAnalysisCenter;
    reliableAnalysisCenter reliableAnalysisCenter;
    // Digest center initialization
    digestCenter.area = 0.0;
    digestCenter.digestMatching = 0;
    digestCenter.lastEventTime = 0.0;
    digestCenter.index = 0;
    digestCenter.indexPremium = 0;
    digestCenter.jobs = 0;
    digestCenter.service_time = 0.0;
    digestCenter.queue = NULL;
    digestCenter.probabilityOfMatching = INITIAL_DIGEST_MATCHING_PROB;

    // Normal center initialization
    normalAnalysisCenter.area = 0.0;
    normalAnalysisCenter.numberOfTimeouts = 0;
    normalAnalysisCenter.lastEventTime = 0.0;
    normalAnalysisCenter.index = 0;
    normalAnalysisCenter.jobs = 0;
    initializeServerArray(normalAnalysisCenter.service_time, normalAnalysisCenter.servers, N_NORMAL);
    normalAnalysisCenter.queue = NULL;

    // Premium center initialization
    premiumAnalysisCenter.area = 0.0;
    premiumAnalysisCenter.numberOfTimeouts = 0;
    premiumAnalysisCenter.lastEventTime = 0.0;
    premiumAnalysisCenter.index = 0;
    premiumAnalysisCenter.jobs = 0;
    initializeServerArray(premiumAnalysisCenter.service_time, premiumAnalysisCenter.servers, N_PREMIUM);
    premiumAnalysisCenter.queue = NULL;

    // Reliable center initialization
    reliableAnalysisCenter.area = 0.0;
    reliableAnalysisCenter.areaPremium = 0.0;
    reliableAnalysisCenter.jobs = 0;
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
    }

    events.arrivals = NULL;
    events.terminations = NULL;
    PlantSeeds(123456789);
    insertList(&events, getArrival(), 0);
    while (simulationTime < OBSERVATION_PERIOD && !(isEmptyList(events)))
    {
        
        if (nextEvent(events) == 0)
        {
            switch (events.arrivals->center)
            {
            case CENTER_DIGEST:
                //printf("digest arrival\n");
                handleDigestArrival(&digestCenter, &events);
                break;
            case CENTER_NORMAL:
                //printf("normal arrival\n");
                handleNormalArrival(&normalAnalysisCenter, &events);
                break;
            case CENTER_PREMIUM:
                //printf("premium arrival\n");
                handlePremiumArrival(&premiumAnalysisCenter, &events);
                break;
            case CENTER_RELIABLE:
                //printf("reliable arrival\n");
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
    printf("Average response time : %6.2f", digestResponseTime);

    double delayArea = digestCenter.area - digestCenter.service_time;
    printf("Average wait time : %6.2f", delayArea / digestCenter.index);
    printf("Job with a matching digest: %d (percentage : %6.2f)\n",
     digestCenter.digestMatching, (double)digestCenter.digestMatching / digestCenter.index);

    // Normal analysis center
    double normalResponseTime = normalAnalysisCenter.area / normalAnalysisCenter.index;
    printf("\nCenter 2 : Normal analysis center\n");
    printf("Number of processed jobs : %d\n", normalAnalysisCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", normalAnalysisCenter.area / simulationTime);
    printf("Average response time : %6.2f\n", normalResponseTime);

    delayArea = normalAnalysisCenter.area;
    for (int i = 0; i < N_NORMAL; i++)
    {
        delayArea -= normalAnalysisCenter.service_time[i];
    }
    double meanServiceTime = 0.0;
    for (int k = 0; k < N_NORMAL; k++)
    {
        meanServiceTime += normalAnalysisCenter.service_time[k];
    }
    meanServiceTime = meanServiceTime / normalAnalysisCenter.index;
    double lambda = (normalAnalysisCenter.area / simulationTime) / normalResponseTime;
    double rho = lambda * meanServiceTime;
    printf("Utilization : %6.2f\n", rho);
    printf("Average service time : %6.2f\n", meanServiceTime);
    printf("Average wait time : %6.2f\n", delayArea / normalAnalysisCenter.index);
    printf("Number of termination due to timeout expiration: %d (percentage : %6.2f)\n",
     normalAnalysisCenter.numberOfTimeouts, (double)normalAnalysisCenter.numberOfTimeouts / normalAnalysisCenter.index);

    // Premium analysis center
    double premiumResponseTime = premiumAnalysisCenter.area / premiumAnalysisCenter.index;
    printf("\nCenter 3 : Premium analysis center\n");
    printf("Number of processed jobs : %d\n", premiumAnalysisCenter.index);
    printf("Mean number of jobs in the center: %6.2f\n", premiumAnalysisCenter.area / simulationTime);
    printf("Average response time : %6.2f\n", premiumResponseTime);

    delayArea = premiumAnalysisCenter.area;
    for (int i = 0; i < N_PREMIUM; i++)
    {
        delayArea -= premiumAnalysisCenter.service_time[i];
    }

    printf("Average wait time : %6.2f\n", delayArea / premiumAnalysisCenter.index);
    printf("Number of termination due to timeout expiration: %d (percentage : %6.2f)\n",
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

    delayArea = reliableAnalysisCenter.area;
    double delayAreaPremium = reliableAnalysisCenter.areaPremium;
    printf("Area premium = %6.2f\n",delayAreaPremium);
    for (int i = 0; i < N_RELIABLE; i++)
    {
        delayArea -= reliableAnalysisCenter.service_time[i];
        delayAreaPremium -= reliableAnalysisCenter.service_time_premium[i];
        printf("Service time= %6.2f\n",reliableAnalysisCenter.service_time_premium[i]);
    }

    printf("Average PREMIUM class wait time : %6.2f\n", delayAreaPremium / reliableAnalysisCenter.premiumIndex);
    printf("Average wait time : %6.2f\n", delayArea / reliableAnalysisCenter.index);
    printf("Number of termination due to timeout expiration: %d (percentage : %6.2f)\n",
     reliableAnalysisCenter.numberOfTimeouts, (double)reliableAnalysisCenter.numberOfTimeouts / reliableAnalysisCenter.index);

    // Global performances
    double globalResponseTime = (digestCenter.area + normalAnalysisCenter.area + premiumAnalysisCenter.area + reliableAnalysisCenter.area) / digestCenter.index;
    double globalPremiumResponseTime = (digestCenter.area  + premiumAnalysisCenter.area + reliableAnalysisCenter.areaPremium) / digestCenter.indexPremium;
    printf("Global response time : %6.2f\nGlobal premium response time : %6.2f\n",globalResponseTime,globalPremiumResponseTime);
}

void handleDigestArrival( digestCenter *digestCenter, event_list *ev)
{
    arrival* ar = ev->arrivals;
    digestCenter->area += (ar->time - digestCenter->lastEventTime) * digestCenter->jobs;
    digestCenter->jobs++;
    simulationTime = ar->time;
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
        job_queue *node = malloc(sizeof(job_queue));
        node->job = ar->job;
        insertQueue(&(digestCenter->queue), node);
    }
    // Generate a new arrival and insert into the list of events, only if the simuation time is not over
    if (simulationTime < OBSERVATION_PERIOD)
        insertList(ev, getArrival(), 0);

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
    center->area += (ar->time - center->lastEventTime) * center->jobs;
    center->jobs++;
    simulationTime = ar->time;
    center->lastEventTime = simulationTime;
    ev->arrivals = ar->next;
    if (center->jobs <= N_NORMAL)
    {
        int server_index = findFreeServer(center->servers, N_NORMAL);
        center->servers[server_index] = 1;
        // The actual service time can be less than the generated service time due to timeout expiration
        center->service_time[server_index] += min(ar->job.serviceTime, TIMEOUT);
        termination *term = malloc(sizeof(termination));
        term->time = min(ar->job.serviceTime, TIMEOUT) + simulationTime;
        term->center = CENTER_NORMAL;
        term->job = ar->job;
        term->server_index = server_index;
        insertList(ev, term, 1);
    }
    else
    {
        job_queue *node = malloc(sizeof(job_queue));
        node->job = ar->job;
        insertQueue(&(center->queue), node);
    }
    free(ar);
}

void handleNormalTermination(normalAnalysisCenter *center, event_list *ev, digestCenter *digestCenter)
{
    termination *ter = ev->terminations;
    center->area += (ter->time - center->lastEventTime) * center->jobs;
    center->jobs--;
    center->index++;
    simulationTime = ter->time;
    center->lastEventTime = simulationTime;
    ev->terminations = ter->next;
    if (ter->job.serviceTime > TIMEOUT)
    {
        center->numberOfTimeouts++;
        arrival *ar = malloc(sizeof(arrival));
        ar->center = CENTER_RELIABLE;
        ar->time = simulationTime;
        ar->job = ter->job;
        // Service time is half because reliable machines are 2x faster than normal machines
        ar->job.serviceTime = ar->job.serviceTime / 2;
        insertList(ev, ar, 0);
    }else if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB){
        digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;
    }
    center->servers[ter->server_index] = 0;
    if (center->jobs >= N_NORMAL)
    {
        int server_index = findFreeServer(center->servers, N_NORMAL);
        center->servers[server_index] = 1;
        job_queue *j_q = popQueue(&center->queue);
        Job job = j_q->job;
        free(j_q);
        // The actual service time can be less than the generated service time due to timeout expiration
        center->service_time[server_index] += min(job.serviceTime, TIMEOUT);
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
    if (type == PREMIUM)
    {
        center->areaPremium += (ar->time - center->lastEventTimePremium) * center->premiumJobs;
        center->lastEventTimePremium = ar->time;
        center->premiumJobs++;
    }
    center->jobs++;
    simulationTime = ar->time;
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
        if (type == PREMIUM)
        {
            insertQueue(&(center->queuePremium), node);
        }
        else
        {
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
    if (type == PREMIUM)
    {
        center->areaPremium += (ter->time - center->lastEventTimePremium) * center->premiumJobs;
        center->lastEventTimePremium = ter->time;
        center->premiumJobs--;
        center->premiumIndex++;
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

        if (center->queuePremium == NULL)
        {
            // job of the normal queue can be processed

            job_queue *j_q = popQueue(&(center->queueNormal));
            job = j_q->job;
            free(j_q);
        }
        else
        {
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
