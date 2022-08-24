/**
 * @file handle_events.c
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-08-23
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file contains the implementation of the functions used to handle the simulation events for each center.
 * It also implements other support functions.
 */

#include "handle_events.h"
#include "config.h"


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



/**
 * @brief Process an arrival event to the Digest Center.
 * 
 * @param digestCenter Pointer to digestCenter struct
 * @param ev Pointer to the event list of the simulation
 * @param simulationTime Actual simulation time
 * @return double Returns the updated simulation time
 */
double handleDigestArrival(digestCenter *digestCenter, event_list *ev, double simulationTime)
{
    arrival *ar = ev->arrivals;                                         // take the arrival event from the list
    digestCenter->jobs++;                                               // increase the number of jobs in the center
    simulationTime = ar->time;                                          // update simulation time
    digestCenter->lastEventTime = ar->time;                             // update the time of the last event happened in the center
    ev->arrivals = ar->next;                                            // remove the processed arrival from the list of events

    // SSQ
    // server is not busy, let's execute the job
    if (digestCenter->jobs == 1)
    {
        // generate the termination event
        termination *term = malloc(sizeof(termination));
        term->time = ar->job.serviceTime + simulationTime;              // compute termination time
        term->center = CENTER_DIGEST;
        term->job = ar->job;
        insertList(ev, term, 1);                                        // insert termination event in the event list
    }
    else
    {
        // server is busy: put the job in the queue
        digestCenter->jobsInQueue++;
        job_queue *node = malloc(sizeof(job_queue));
        node->job = ar->job;
        insertQueue(&(digestCenter->queue), node);
    }

    // Generate a new arrival and insert into the list of events, only if the simulation time is not over
    if (simulationTime < OBSERVATION_PERIOD || INFINITE_HORIZON)
    {
        arrival *newArrival = getArrival(simulationTime);
        digestCenter->interarrivalTime += newArrival->time - simulationTime;
        insertList(ev, newArrival, 0);
    }

    free(ar);
    return simulationTime;
}


/**
 * @brief Process a termination event at the Digest Center. Here, a job can have 
 * a matching digest and so its analysis is over and it can leave the system. Otherwise, in original system,
 * it have to be processed from the Normal Analysis Center or from the Premium Analysis Center.
 * In the improved system, it is sent to the ML Center.
 * 
 * @param digestCenter Pointer to digestCenter struct
 * @param ev Pointer to event list 
 * @param simulationTime Actual simulation time
 * @return double Returns the updated simulation time
 */
double handleDigestTermination(digestCenter *digestCenter, event_list *ev, double simulationTime)
{
    termination *term = ev->terminations;                               // get the termination event
    digestCenter->jobs--;                                               // decrease number of jobs in the center
    digestCenter->index++;                                              // increase number of processed jobs
    if (term->job.userType == PREMIUM)
    {
        digestCenter->indexPremium++;                                   // increase number of jobs submitted by a Premium user
    }
    simulationTime = term->time;                                        // update simulation time
    digestCenter->lastEventTime = term->time;                           // update last event time
    ev->terminations = term->next;                                      // remove event from the simulation event list

    SelectStream(DIGEST_MATCHING_PROBABILITY_STREAM);                   // randomly choose if the digest has matched or not
    if (!Bernoulli(digestCenter->probabilityOfMatching))
    {
        // Digest not matching: create new arrival
        if (IMPROVEMENT)
        {
            // new arrival to the ML Center
            SelectStream(MEAN_SERVICE_TIME_ML_STREAM);
            arrival *ar = malloc(sizeof(arrival));
            ar->job.type = term->job.type;
            ar->job.userType = term->job.userType;
            ar->job.serviceTime = Exponential(ML_MEAN_SERVICE_TIME);    // generate service time

            ar->time = simulationTime;
            ar->center = CENTER_ML;
            insertList(ev, ar, 0);
        }
        else
        {   
            // new arrival to Normal Analysis Center or to Premium Analysis Center
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
                ar->job.serviceTime = Exponential(PREMIUM_MEAN_SERVICE_TIME);   // generate service time
                ar->job.type = term->job.type;
                ar->job.userType = term->job.userType;
                break;
            case NORMAL:
                ar->center = CENTER_NORMAL;
                SelectStream(MEAN_SERVICE_TIME_NORMAL_STREAM);
                ar->job.serviceTime = Exponential(NORMAL_MEAN_SERVICE_TIME);    // generate service time
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
        // digest has matched
        digestCenter->digestMatching++;                                  // increase number of jobs whose digest has matched 
    }

    // if the queue is not empty, let's execute the next job
    if (digestCenter->jobs >= 1)
    {
        digestCenter->jobsInQueue--;                                    // decrease the number of jobs in the queue
        job_queue *j_q = popQueue(&(digestCenter->queue));              // get the next job in the queue (FIFO policy)
        Job job = j_q->job;
        free(j_q);
        termination *ter = malloc(sizeof(termination));                 // generate a termination event for this job
        ter->time = job.serviceTime + simulationTime;
        ter->center = CENTER_DIGEST;
        ter->job = job;
        insertList(ev, ter, 1);
    }
    free(term);
    return simulationTime;
}



/**
 * @brief Process an arrival event to the Normal Analysis Center.
 * 
 * @param center Pointer to normalAnalysisCenter struct
 * @param ev Pointer to the event list of the simulation
 * @param simulationTime Actual simulation time
 * @return double Return the updated simulation time
 */
double handleNormalArrival(normalAnalysisCenter *center, event_list *ev, double simulationTime)
{

    arrival *ar = ev->arrivals;                                         // get the arrival event
    ar->job.arrivalTime = ar->time;
    center->jobs++;                                                     // increase the number of jobs in the center
    simulationTime = ar->time;
    if (center->lastArrivalTime != 0.0)
        center->interarrivalTime += ar->time - center->lastArrivalTime; // update interarrival time sum
    center->lastArrivalTime = simulationTime;                           // update the last arrival time in the center
    center->lastEventTime = simulationTime;                             // update last event time
    ev->arrivals = ar->next;                                            // remove the event from the event list

    // if there is at least one idle server, the incoming job can be executed
    if (center->jobs <= N_NORMAL)
    {
        int server_index = findFreeServer(center->servers, N_NORMAL);   // Find a free server (IN ORDER policy)
        center->servers[server_index] = 1;                              // Set the server as busy
        termination *term = malloc(sizeof(termination));                // Generate termination event
        term->time = min(ar->job.serviceTime, TIMEOUT) + simulationTime;// Compute termination time; take timeout into account
        term->center = CENTER_NORMAL;
        term->job = ar->job;
        term->server_index = server_index;
        insertList(ev, term, 1);
    }
    // All servers are busy; incoming job is put in the queue
    else
    {
        center->jobsInQueue++;                                          // increase the number of jobs in the queue
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
    return simulationTime;
}


/**
 * @brief Process a termination event at the Normal Analysis Center. If the execution finished with a timeout expiration,
 * the job has to be processed by the Reliable Analysis Center. Otherwise, it exits the system.
 * 
 * @param center Pointer to normalAnalysisCenter struct
 * @param ev Pointer to event list
 * @param digestCenter Pointer to digestCenter struct
 * @param simulationTime Actual simulation time
 * @return double Returns the updated simulation time
 */
double handleNormalTermination(normalAnalysisCenter *center, event_list *ev, digestCenter *digestCenter, double simulationTime)
{
    termination *ter = ev->terminations;                                // get the termination event
    center->jobs--;                                                     // decrease number of jobs in the center
    center->index++;                                                    // increase number of processed jobs
    simulationTime = ter->time;                                         // update simulation time
    center->lastEventTime = simulationTime;                             // update last event time
    ev->terminations = ter->next;                                       // remove the vent from the list of events

    // if the timeout expired, the job should be sent to the reliable center
    if (ter->job.serviceTime > TIMEOUT)
    {
        center->numberOfTimeouts++;                                     // increase number of timeouts
        arrival *ar = malloc(sizeof(arrival));                          // create new arrival event at reliable center
        ar->center = CENTER_RELIABLE;
        ar->time = simulationTime;
        ar->job = ter->job;

        SelectStream(MEAN_SERVICE_TIME_RELIABLE_STREAM);
        ar->job.serviceTime = Exponential(RELIABLE_MEAN_SERVICE_TIME);  // generate service time
        insertList(ev, ar, 0);
    }
    else if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB)
    {
        digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;   // increase matching probability, if job exits the system and limit has not been reached
    }
    center->servers[ter->server_index] = 0;                             // server becomes idle

    // if there are jobs in queue, serve other jobs
    if (center->jobs >= N_NORMAL)
    {
        center->jobsInQueue--;                                          // decrease number of jobs in queue
        int server_index = findFreeServer(center->servers, N_NORMAL);   // find an idle server
        center->servers[server_index] = 1;                              // server becomes busy
        job_queue *j_q = popQueue(&center->queue);                      // remove job from queue
        if (center->jobsInQueue != sizeQueue(center->queue))
        {
            printf("FATAL ERROR\n");
            exit(0);
        }
        Job job = j_q->job;
        free(j_q);
        termination *termi = malloc(sizeof(termination));               // generate termination event
        termi->time = min(job.serviceTime, TIMEOUT) + simulationTime;   // compute termination time
        termi->center = CENTER_NORMAL;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
    return simulationTime;
}


/**
 * @brief Process an arrival event at the Premium Analysis Center.
 * 
 * @param center Pointer to premiumAnalysisCenter struct
 * @param ev Pointer to event list
 * @param simulationTime Actual simulation time
 * @return double Returns the updated simulation time
 */
double handlePremiumArrival(premiumAnalysisCenter *center, event_list *ev, double simulationTime)
{

    arrival *ar = ev->arrivals;                                         // get the arrival event
    center->jobs++;                                                     // increase the number of jobs in the center
    simulationTime = ar->time;                                          // update simulation time
    if (center->lastArrivalTime != 0.0)
    {
        center->interarrivalTime += ar->time - center->lastArrivalTime; // update interarrival time sum
    }
    center->lastArrivalTime = simulationTime;                           // update last arrival time
    center->lastEventTime = simulationTime;                             // update last event time
    ev->arrivals = ar->next;                                            // remove the event from the event list

    // if there are idle servers, let's serve the job
    if (center->jobs <= N_PREMIUM)
    {
        int server_index = findFreeServer(center->servers, N_PREMIUM);  // find an idle server
        center->servers[server_index] = 1;                              // make the server busy
        termination *term = malloc(sizeof(termination));                // generate termination event
        if (term == NULL)
        {
            printf("Memory leak\n");
            exit(0);
        }
        term->time = min(ar->job.serviceTime, TIMEOUT) + simulationTime;// compute termination time taking timeout into account
        term->center = CENTER_PREMIUM;
        term->job = ar->job;
        term->server_index = server_index;

        insertList(ev, term, 1);
    }
    // all servers are busy; put the arrived job in the queue
    else
    {
        center->jobsInQueue++;                                          // increase the number of jobs in the queue
        job_queue *node = malloc(sizeof(job_queue));

        if (node == NULL)
        {
            printf("Memory leak\n");
            exit(0);
        }
        node->job = ar->job;

        insertQueue(&(center->queue), node);                            // put the job in the queue
        if (center->jobsInQueue != sizeQueue(center->queue))
        {
            printf("FATAL ERROR\n");
            exit(0);
        }
    }
    free(ar);
    return simulationTime;
}


/**
 * @brief Process a termination event at the Premium Analysis Center. If the service finished due to timeout expiration,
 * the job is sent to the Reliable Analysis Center; otherwise, it exits the system.
 * 
 * @param center Pointer to premiumAnalysisCenter struct
 * @param ev Pointer to event list
 * @param digestCenter Pointer to digestCenter struct
 * @param simulationTime Actual simulation time
 * @return double Returns the updated simulation time
 */
double handlePremiumTermination(premiumAnalysisCenter *center, event_list *ev, digestCenter *digestCenter, double simulationTime)
{
    termination *ter = ev->terminations;                                // get the termination event
    center->jobs--;                                                     // decrease the number of jobs in the center
    center->index++;                                                    // increase the number of processed jobs
    simulationTime = ter->time;                                         // update simulation time
    center->lastEventTime = simulationTime;                             // update last event time
    ev->terminations = ter->next;                                       // remove the event from the event list

    // if the service finished due to timeout expiration, let's create a new arrival event at the Reliable Analysis Center 
    if (ter->job.serviceTime > TIMEOUT)
    {
        center->numberOfTimeouts++;                                     // increase the number of timeouts
        arrival *ar = malloc(sizeof(arrival));                          // generate new arrival event
        ar->center = CENTER_RELIABLE;
        ar->time = simulationTime;
        ar->job = ter->job;
        SelectStream(MEAN_SERVICE_TIME_RELIABLE_STREAM);
        ar->job.serviceTime = Exponential(RELIABLE_MEAN_SERVICE_TIME);  // generate new service time
        insertList(ev, ar, 0);
    }
    else if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB)
    {
        digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;   // increase digest matching probability if the job exits the system and if the limit has not been already reached
    }
    center->servers[ter->server_index] = 0;                             // make the server idle

    // if there are jobs waiting in the queue, let's serve the next one
    if (center->jobs >= N_PREMIUM)
    {
        center->jobsInQueue--;                                          // decrease the number of jobs in the queue
        int server_index = findFreeServer(center->servers, N_PREMIUM);  // find an idle server
        center->servers[server_index] = 1;                              // make the server busy
        job_queue *j_q = popQueue(&(center->queue));                    // remove the job from the queue
        if (center->jobsInQueue != sizeQueue(center->queue))
        {
            printf("FATAL ERROR\n");
            exit(0);
        }
        Job job = j_q->job;
        free(j_q);
        termination *termi = malloc(sizeof(termination));               // generate a new termination event
        termi->time = min(job.serviceTime, TIMEOUT) + simulationTime;   // compute termination time, taking timeout into account
        termi->center = CENTER_PREMIUM;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
    return simulationTime;
}


/**
 * @brief Process an arrival event at the Reliable Analysis Center.
 * Here there can be arrivals of jobs submitted by normal users and of jobs submitted by premium users.
 * Since there are two different priority queues, the two situations must be handled differently.
 * This center is a SINGLE SERVER center with two queues in the original system, but is MULTI SERVER center
 * with two queues in the improved system.
 * So, the center has been modeled as a multi-server center, but with a configured number of servers equal to 1
 * it becomes a single server center.
 * 
 * @param center Pointer to reliableAnalysisCenter struct
 * @param ev Pointer to event list
 * @param simulationTime Actual simulation time
 * @return double Returns the updated simulation time
 */
double handleReliableArrival(reliableAnalysisCenter *center, event_list *ev, double simulationTime)
{
    arrival *ar = ev->arrivals;                                         // get the arrival event
    UserType type = ar->job.userType;                                   // get the user type of the submitted job

    // if the job has been submitted by a premium user
    if (type == PREMIUM)
    {
        center->lastEventTimePremium = ar->time;                        // update last event time of premium jobs
        center->premiumJobs++;                                          // increase number of premium jobs in the center
    }
    // else if the job has been submitted by a normal user
    else
    {
        center->lastEventTimeNormal = ar->time;                         // update last event time of normal jobs
        center->normalJobs++;                                           // increase the number of normal jobs in the center
    }
    center->jobs++;                                                     // increase the number of total jobs in the center
    simulationTime = ar->time;                                          // update simulation time
    if (center->lastArrivalTime != 0.0)
        center->interarrivalTime += ar->time - center->lastArrivalTime; // update interarrival time sum
    center->lastArrivalTime = simulationTime;                           // update last arrival time
    center->lastEventTime = simulationTime;                             // update last event time
    ev->arrivals = ar->next;                                            // remove the arrival event from the event list

    // if there are idle servers, let's serve the arrived job
    if (center->jobs <= N_RELIABLE)
    {
        int server_index = findFreeServer(center->servers, N_RELIABLE); // find an idle server
        center->servers[server_index] = 1;                              // make the server busy
        termination *term = malloc(sizeof(termination));                // generate termination event
        term->time = min(ar->job.serviceTime, TIMEOUT_RELIABLE) + simulationTime;   // compute termination time, taking timeout into account
        term->center = CENTER_RELIABLE;
        term->job = ar->job;
        term->server_index = server_index;
        insertList(ev, term, 1);
    }
    // servers are busy; let's put the arrived job in one of the two queues
    else
    {
        job_queue *node = malloc(sizeof(job_queue));
        if (node == NULL)
        {
            printf("Memory leak\n");
            exit(0);
        }
        node->job = ar->job;
        
        // if the job is from a premium user, let's put it in the high priority queue
        if (type == PREMIUM)
        {
            center->jobsInQueuePremium++;                               // increase the number of jobs in the high priority queue
            insertQueue(&(center->queuePremium), node);                 // insert the job in the queue
        }
        // else the job is from a normal user and so it must be put in the low priority queue
        else
        {
            center->jobsInQueueNormal++;                                // increase the number of jobs in the low priority queue
            insertQueue(&(center->queueNormal), node);                  // insert the job in the queue
        }
    }
    free(ar);
    return simulationTime;
}


/**
 * @brief Process a termination event at the Reliable Analysis Center. The service can finish due to a timeout expiration or not,
 * but, in both cases, the job will leave the system.
 * When the server becomes idle, if any, jobs of the high priority queue will enter the service; otherwise, if any, jobs of the 
 * low priority queue can enter the service. There is no preemption mechanism.
 * 
 * @param center Pointer to reliableAnalysisCenter struct
 * @param ev Pointer to the event list
 * @param digestCenter Pointer to digestCenter struct
 * @param simulationTime Actual simulation time
 * @return double Returns the updated simulation time
 */
double handleReliableTermination(reliableAnalysisCenter *center, event_list *ev, digestCenter *digestCenter, double simulationTime)
{
    termination *ter = ev->terminations;                                // get the termination event
    UserType type = ter->job.userType;                                  // get the user type of the submitted job

    // if the job was submitted by a premium user
    if (type == PREMIUM)
    {
        center->lastEventTimePremium = ter->time;                       // update last event time of premium jobs
        center->premiumJobs--;                                          // decrease the number of premium jobs in the center
        center->premiumIndex++;                                         // increase the number of processed premium jobs
    }
    // else if the job was submitted by a normal user
    else
    {
        center->lastEventTimeNormal = ter->time;                        // update last event time of normal jobs
        center->normalJobs--;                                           // decrease the number of normal jobs in the center
        center->normalIndex++;                                          // increase the number of processed normal jobs
    }   
    center->jobs--;                                                     // decrease the number of jobs in the center    
    center->index++;                                                    // increase the number of processed jobs
    simulationTime = ter->time;                                         // update simulation time
    center->lastEventTime = simulationTime;                             // update last event time
    ev->terminations = ter->next;                                       // remove the termination event from the event list

    // if the service finished due to a timeout expiration, let's count a failure
    if (ter->job.serviceTime > TIMEOUT_RELIABLE)
    {
        center->numberOfTimeouts++;                                     // increase the number of jobs timed out
    }
    else
    {
        if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB)
        {
            digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;   // increase the digest matching probability if the job exited the system and the limit has not been already reached
        }
        center->jobAnalyzed++;                                          // increase the number of jobs analyzed without timeout expiration
    }

    center->servers[ter->server_index] = 0;                             // make the server idle

    // if there are jobs in the queues, let's serve the next one
    if (center->jobs >= N_RELIABLE)
    {
        int server_index = findFreeServer(center->servers, N_RELIABLE); // find an idle server
        center->servers[server_index] = 1;                              // make the server busy
        Job job;

        // if the high priority queue is empty, let's take the job from the low priority queue
        if (center->queuePremium == NULL)
        {
            center->jobsInQueueNormal--;                                // decrease the number of jobs in the low priority queue
            job_queue *j_q = popQueue(&(center->queueNormal));          // remove the job from the queue
            job = j_q->job;
            free(j_q);
        }
        // else serve a job from the high priority queue
        else
        {
            center->jobsInQueuePremium--;                               // decrease the number of jobs in the high priority queue
            job_queue *j_q = popQueue(&(center->queuePremium));         // remove the job from the queue
            job = j_q->job;
            free(j_q);
        }

        termination *termi = malloc(sizeof(termination));               // generate the new termination event
        termi->time = min(job.serviceTime, TIMEOUT_RELIABLE) + simulationTime;  // compute termination time, taking timeout into account
        termi->center = CENTER_RELIABLE;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
    return simulationTime;
}

/**
 * @brief Process an arrival event at the ML Center. This center is a muti-server center
 * WITHOUT THE QUEUE, so jobs can bypass the service at this center, if all servers are busy.
 * 
 * @param mlCenter Pointer to mlCenter struct
 * @param ev Pointer to the event list
 * @param simulationTime Actual simulation time
 * @return double Returns the updated simulation time
 */
double handleMachineLearningArrival(machineLearningCenter *mlCenter, event_list *ev, double simulationTime)
{
    arrival *ar = ev->arrivals;                                         // get the arrival event
    simulationTime = ar->time;                                          // update simulation time
    ev->arrivals = ar->next;                                            // remove the event from the event list


    // if there are idle servers, let's serve the arrived job
    if (mlCenter->jobs < N_ML)
    {
        if (mlCenter->lastArrivalTime != 0.0)
            mlCenter->interarrivalTime += ar->time - mlCenter->lastArrivalTime; // update interarrival time sum
        mlCenter->lastArrivalTime = simulationTime;                     // update last arrival time
        mlCenter->jobs++;                                               // increase the number of jobs in the center
        mlCenter->lastEventTime = simulationTime;                       // update last event time

        // We can serve this job
        termination *terr = malloc(sizeof(termination));                // generate new termination event
        terr->center = CENTER_ML;
        terr->time = simulationTime + ar->job.serviceTime;              // compute termination time
        terr->job = ar->job;
        insertList(ev, terr, 1);
    }
    // else, the job bypass this center and goes in the Normal Analysis Center or in the Premium Analysis Center
    else
    {
        mlCenter->numOfBypass++;                                        // increase the number of bypass

        // Job is moved to the correct center
        // If it was submitted by a premium user
        if (ar->job.userType == PREMIUM)
        {
            arrival *arr = malloc(sizeof(arrival));                     // generate new arrival at Premium Analysis Center
            arr->center = CENTER_PREMIUM;
            arr->time = simulationTime;
            arr->job = ar->job;
            SelectStream(MEAN_SERVICE_TIME_PREMIUM_STREAM);
            arr->job.serviceTime = Exponential(PREMIUM_MEAN_SERVICE_TIME);  // compute new service time
            insertList(ev, arr, 0);
        }
        // It was submitted by a normal user
        else
        {   
            arrival *arr = malloc(sizeof(arrival));                     // generate new arrival at Normal Analysis Center
            arr->center = CENTER_NORMAL;
            arr->time = simulationTime;
            arr->job = ar->job;
            SelectStream(MEAN_SERVICE_TIME_NORMAL_STREAM);
            arr->job.serviceTime = Exponential(NORMAL_MEAN_SERVICE_TIME);   // compute new service time
            insertList(ev, arr, 0);
        }
    }
    free(ar);
    return simulationTime;
}

/**
 * @brief Process a termination event at the ML Center. The Ml analysis can give two results: if the job is classified as malware, it
 * leaves the center and the prediction is confirmed. Otherwise, since a malware predicted as non malware can be very dangerous, let's always
 * dynamically analyze jobs predicted as non malware in the other centers. 
 * 
 * @param mlCenter Pointer to mlCenter struct
 * @param ev Pointer to the event list
 * @param digestCenter Pointer to digestCenter struct
 * @param simulationTime Actual simulation time
 * @return double Returns the updated simulation time
 */
double handleMachineLearningTermination(machineLearningCenter *mlCenter, event_list *ev, digestCenter *digestCenter, double simulationTime)
{
    termination *ter = ev->terminations;                                // get the termination event
    simulationTime = ter->time;                                         // update simulation time
    mlCenter->lastEventTime = simulationTime;                           // update last event time
    ev->terminations = ter->next;                                       // remove the event from the event list
    mlCenter->jobs--;                                                   // decrease the number of jobs in the center
    mlCenter->index++;                                                  // increase the number of processed jobs

    // if the job was sumbitted by a premium user
    if (ter->job.userType == PREMIUM)
    {
        mlCenter->indexPremium++;                                       // increase the number of processed jobs submitted by premium users
    }
    SelectStream(ML_RESULT_STREAM);

    if (Bernoulli(PROB_POSITIVE_ML))                                    // generate ML prediction result (malware or not malware)
    {
        // We have a positive result (malware) that goes out of the system
        mlCenter->mlSuccess++;                                          // increase the number of predicted malware
        if (digestCenter->probabilityOfMatching < FINAL_DIGEST_MATCHING_PROB){
            digestCenter->probabilityOfMatching += LINEAR_INCREASING_PROB_FACTOR;   // increase the digest matching probability, if the limit has not been already reached
        }
    }
    else
    {
        // Negative result (not malware)
        // if the job was sumbitted by a premium user, let's send it to the Premium Analysis Center
        if (ter->job.userType == PREMIUM)
        {
            arrival *arr = malloc(sizeof(arrival));                     // generate new arrival event
            arr->center = CENTER_PREMIUM;
            arr->time = simulationTime;
            arr->job = ter->job;
            SelectStream(MEAN_SERVICE_TIME_PREMIUM_STREAM);
            arr->job.serviceTime = Exponential(PREMIUM_MEAN_SERVICE_TIME);  // generate service time
            insertList(ev, arr, 0);
        }
        // else if the job was sumbitted by a normal user, let's send it to the Normal Analysis Center
        else
        {
            arrival *arr = malloc(sizeof(arrival));                     // generate new arrival event
            arr->center = CENTER_NORMAL;
            arr->time = simulationTime;
            arr->job = ter->job;
            SelectStream(MEAN_SERVICE_TIME_NORMAL_STREAM);
            arr->job.serviceTime = Exponential(NORMAL_MEAN_SERVICE_TIME);   // generate service time
            insertList(ev, arr, 0);
        }
    }
    free(ter);
    return simulationTime;
}
