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




double handleDigestArrival(digestCenter *digestCenter, event_list *ev, double simulationTime)
{
    arrival *ar = ev->arrivals;
    // digestCenter->area += (ar->time - digestCenter->lastEventTime) * digestCenter->jobs;
    digestCenter->jobs++;
    simulationTime = ar->time;
    digestCenter->lastEventTime = ar->time;
    ev->arrivals = ar->next;
    if (digestCenter->jobs == 1)
    {
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
        arrival *newArrival = getArrival(simulationTime);
        digestCenter->interarrivalTime += newArrival->time - simulationTime;
        insertList(ev, newArrival, 0);
    }

    free(ar);
    return simulationTime;
}

double handleDigestTermination(digestCenter *digestCenter, event_list *ev, double simulationTime)
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
        termination *ter = malloc(sizeof(termination));
        ter->time = job.serviceTime + simulationTime;
        ter->center = CENTER_DIGEST;
        ter->job = job;
        insertList(ev, ter, 1);
    }
    free(term);
    return simulationTime;
}

double handleNormalArrival(normalAnalysisCenter *center, event_list *ev, double simulationTime)
{

    arrival *ar = ev->arrivals;
    ar->job.arrivalTime = ar->time;
    center->jobs++;
    simulationTime = ar->time;
    if (center->lastArrivalTime != 0.0)
        center->interarrivalTime += ar->time - center->lastArrivalTime;
    center->lastArrivalTime = simulationTime;
    center->lastEventTime = simulationTime;
    ev->arrivals = ar->next;
    if (center->jobs <= N_NORMAL)
    {
        int server_index = findFreeServer(center->servers, N_NORMAL);
        center->servers[server_index] = 1;
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
    return simulationTime;
}

double handleNormalTermination(normalAnalysisCenter *center, event_list *ev, digestCenter *digestCenter, double simulationTime)
{
    termination *ter = ev->terminations;
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
        free(j_q);
        termination *termi = malloc(sizeof(termination));
        termi->time = min(job.serviceTime, TIMEOUT) + simulationTime;
        termi->center = CENTER_NORMAL;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
    return simulationTime;
}

double handlePremiumArrival(premiumAnalysisCenter *center, event_list *ev, double simulationTime)
{

    arrival *ar = ev->arrivals;
    center->jobs++;
    simulationTime = ar->time;
    if (center->lastArrivalTime != 0.0)
    {
        center->interarrivalTime += ar->time - center->lastArrivalTime;
    }
    center->lastArrivalTime = simulationTime;
    center->lastEventTime = simulationTime;
    ev->arrivals = ar->next;

    if (center->jobs <= N_PREMIUM)
    {
        int server_index = findFreeServer(center->servers, N_PREMIUM);
        center->servers[server_index] = 1;
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
    return simulationTime;
}

double handlePremiumTermination(premiumAnalysisCenter *center, event_list *ev, digestCenter *digestCenter, double simulationTime)
{
    termination *ter = ev->terminations;
    // center->area += (ter->time - center->lastEventTime) * center->jobs;
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
        termination *termi = malloc(sizeof(termination));
        termi->time = min(job.serviceTime, TIMEOUT) + simulationTime;
        termi->center = CENTER_PREMIUM;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
    return simulationTime;
}

double handleReliableArrival(reliableAnalysisCenter *center, event_list *ev, double simulationTime)
{
    arrival *ar = ev->arrivals;
    // center->area += (ar->time - center->lastEventTime) * center->jobs;
    UserType type = ar->job.userType;
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
    return simulationTime;
}

double handleReliableTermination(reliableAnalysisCenter *center, event_list *ev, digestCenter *digestCenter, double simulationTime)
{
    termination *ter = ev->terminations;
    // center->area += (ter->time - center->lastEventTime) * center->jobs;
    UserType type = ter->job.userType;
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

        termination *termi = malloc(sizeof(termination));
        termi->time = min(job.serviceTime, TIMEOUT_RELIABLE) + simulationTime;
        // termi->time = job.serviceTime + simulationTime;
        termi->center = CENTER_RELIABLE;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev, termi, 1);
    }
    free(ter);
    return simulationTime;
}

double handleMachineLearningArrival(machineLearningCenter *mlCenter, event_list *ev, double simulationTime)
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
    return simulationTime;
}

double handleMachineLearningTermination(machineLearningCenter *mlCenter, event_list *ev, double simulationTime)
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
    return simulationTime;
}
