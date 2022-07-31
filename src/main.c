#include "events_queue.c"
#include "job.c"
#include "config.h"
#include "centers.c"
#include "../lib/rngs.h"
#include "../lib/rvgs.h"
#include <stdio.h>
#include<stdlib.h>
#include<math.h>

typedef struct __event_list{
    arrival *arrivals;
    termination *terminations;
}event_list;

#define START 0.0
#define MEAN_INTERARRIVAL_STREAM 0
#define DIGEST_SERVICE_TIME_STREAM 1
#define USER_PROBABILITY_STREAM 2
#define MALWARE_PROBABILITY_STREAM 3
#define DIGEST_MATCHING_PROBABILITY_STREAM 4
#define MEAN_SERVICE_TIME_PREMIUM_STREAM 5
#define MEAN_SERVICE_TIME_NORMAL_STREAM 6
double simulationTime = START;
arrival* getArrival(){
    /*
    This function is used to generate an arrival to the entire system: 
    The arrival is only received by the first center (digest calculation)
    */
   arrival* event = malloc(sizeof(arrival));
   SelectStream(MEAN_INTERARRIVAL_STREAM);
   event->time = Exponential(MEAN_INTERARRIVAL_TIME) + simulationTime;
   event -> center = DIGEST;
   SelectStream(USER_PROBABILITY_STREAM);
   event->job.userType = Bernoulli(PROBABILITY_PREMIUM);
   SelectStream(MALWARE_PROBABILITY_STREAM);
   event->job.type = Bernoulli(PROBABILITY_MALWARE);
   SelectStream(DIGEST_SERVICE_TIME_STREAM);
   event->job.serviceTime = Exponential(DIGEST_MEAN_SERVICE_TIME);
   return event;
}

void insertList(event_list ev, void* node, int type){
    if(type == 0){
        arrival* ar = (arrival*) node;
        arrival* head = ev.arrivals;
        arrival* prev = NULL;
        if(head==NULL){
            head=ar;
            ar->next = NULL;
            ev.arrivals = head;
            return;
        }
        while(head!=NULL){
            if(head->job.serviceTime<ar->job.serviceTime){
                ar->next = head;
                if(prev==NULL){
                    ev.arrivals = ar;
                }
                else{
                    prev->next = ar;
                }
                break;
            }
            else{
                prev = head;
                head = head->next;
            }
        }
        if(head == NULL){
            prev->next = ar;
            ar->next = NULL;
        }
    }
    else{
        termination* ar = (termination*) node;
        termination* head = ev.terminations;
        termination* prev = NULL;
        if(head==NULL){
            head=ar;
            ar->next = NULL;
            ev.terminations = head;
            return;
        }
        while(head!=NULL){
            if(head->job.serviceTime<ar->job.serviceTime){
                ar->next = head;
                if(prev==NULL){
                    ev.terminations = ar;
                }
                else{
                    prev->next = ar;
                }
                break;
            }
            else{
                prev = head;
                head = head->next;
            }
        }
        if(head == NULL){
            prev->next = ar;
            ar->next = NULL;
        }
    }
}

int findFreeServer(int* servers, int n){
    int i = 0;
    for (i=0; i < servers; i++){
        if(servers[i]==0){
            return i;
        }
    }
    return -1;
}

int nextEvent(event_list ev){
    if(ev.terminations==NULL){
        return 0;
    }
    if(ev.arrivals->time < ev.terminations->time){
        return 0;
    }
    return 1;
}
void handleDigestArrival(arrival* arrival,digestCenter digestCenter,event_list ev);
void handleNormalArrival(normalAnalysisCenter center, event_list ev);
void handlePremiumArrival(premiumAnalysisCenter center, event_list ev);
void handleReliableArrival(arrival* arrival);
void handleDigestTermination(termination* termination,digestCenter digestCenter, event_list ev);
void handleNormalTermination(normalAnalysisCenter center, event_list ev);
void handlePremiumTermination(premiumAnalysisCenter center, event_list ev);
void handleReliableTermination(termination* termination);

int main(){
    event_list events;
    digestCenter digestCenter;
    normalAnalysisCenter normalAnalysisCenter;
    premiumAnalysisCenter premiumAnalysisCenter;
    reliableAnalysisCenter reliableAnalysisCenter;
    //Digest center initialization
    digestCenter.area=0.0;
    digestCenter.digestMatching=0;
    digestCenter.lastEventTime=0.0;
    digestCenter.index=0;
    digestCenter.jobs=0;
    digestCenter.service_time=0.0;
    digestCenter.queue=NULL;
    digestCenter.probabilityOfMatching=INITIAL_DIGEST_MATCHING_PROB;

    //Normal center initialization
    normalAnalysisCenter.area=0.0;
    normalAnalysisCenter.numberOfTimeouts=0;
    normalAnalysisCenter.lastEventTime=0.0;
    normalAnalysisCenter.index=0;
    normalAnalysisCenter.jobs=0;
    initializeServerArray(normalAnalysisCenter.service_time,normalAnalysisCenter.servers,N_NORMAL);
    normalAnalysisCenter.queue = NULL;

    //Premoium center initialization
    premiumAnalysisCenter.area = 0.0;
    premiumAnalysisCenter.numberOfTimeouts=0;
    premiumAnalysisCenter.lastEventTime=0.0;
    premiumAnalysisCenter.index=0;
    premiumAnalysisCenter.jobs=0;
    initializeServerArray(premiumAnalysisCenter.service_time,premiumAnalysisCenter.servers,N_PREMIUM);
    premiumAnalysisCenter.queue=NULL;

    events.arrivals = NULL;
    events.terminations = NULL;
    PlantSeeds(123456789);
    insertList(events,getArrival(),0);
    while (simulationTime < OBSERVATION_PERIOD){
        if(nextEvent(events)==0){
            switch(events.arrivals->center){
                case DIGEST:
                    handleDigestArrival(events.arrivals,digestCenter,events);
                    break;
                case NORMAL:
                    handleNormalArrival(normalAnalysisCenter,events);
                    break;
                case PREMIUM:
                    handlePremiumArrival(premiumAnalysisCenter,events);
                    break;
                case RELIABLE:
                    handleReliableArrival(events.arrivals);

            }
        }
        else{
            //Termination
            switch(events.terminations->center){
                case DIGEST:
                    handleDigestTermination(events.terminations,digestCenter,events);
                    break;
                case NORMAL:
                    handleNormalTermination(normalAnalysisCenter,events);
                    break;
                case PREMIUM:
                    handlePremiumTermination(premiumAnalysisCenter,events);
                    break;
                case RELIABLE:
                    handleReliableTermination(events.terminations);

            }
        }
    }
}

void handleDigestArrival(arrival* arrival,digestCenter digestCenter, event_list ev){
    digestCenter.area += (arrival->time - digestCenter.lastEventTime)*digestCenter.jobs;
    digestCenter.jobs++;
    simulationTime = arrival->time;
    digestCenter.lastEventTime = arrival->time;
    ev.arrivals= arrival->next;
    if(digestCenter.jobs==1){
        digestCenter.service_time+=arrival->job.serviceTime;
        termination* termination = malloc(sizeof(termination));
        termination->time = arrival->job.serviceTime + simulationTime;
        termination->center=DIGEST;
        termination->job = arrival->job;
        insertList(ev,termination,1);
    }
    else{
        job_queue *node = malloc(sizeof(job_queue));
        node->job = arrival->job;
        insertQueue(&digestCenter.queue,node);
    }
    //Generate a new arrival and insert into the list of events
    insertList(ev,getArrival(),0);
    
    free(arrival);

}

void handleDigestTermination(termination* termination, digestCenter digestCenter, event_list ev){
    digestCenter.area += (termination->time - digestCenter.lastEventTime)*digestCenter.jobs;
    digestCenter.jobs--;
    digestCenter.index++;
    simulationTime = termination->time;
    digestCenter.lastEventTime = termination->time;
    ev.terminations = termination->next;
    SelectStream(DIGEST_MATCHING_PROBABILITY_STREAM);
    if(!Bernoulli(digestCenter.probabilityOfMatching)){
        //Digest not matching: create new arrival
        arrival* ar = malloc(sizeof(arrival));
        switch(termination->job.userType){
            case PREMIUM:
                ar->center=PREMIUM;
                SelectStream(MEAN_SERVICE_TIME_PREMIUM_STREAM);
                ar->job.serviceTime = Exponential(PREMIUM_MEAN_SERVICE_TIME);
                ar->job.type=termination->job.type;
                ar->job.userType = termination->job.userType;
                break;
            case NORMAL:
                ar->center = NORMAL;
                SelectStream(MEAN_SERVICE_TIME_NORMAL_STREAM);
                ar->job.serviceTime = Exponential(NORMAL_MEAN_SERVICE_TIME);
                ar->job.type=termination->job.type;
                ar->job.userType = termination->job.userType;
                break;
        }
        ar->time=simulationTime;
        insertList(ev,ar,0);
    }
    else{
        digestCenter.digestMatching++;
    }
    if(digestCenter.jobs>=1){
        job_queue *j_q = popQueue(&digestCenter.queue);
        job job = j_q->job;
        free(j_q);
        digestCenter.service_time+=job.serviceTime;
        termination* ter = malloc(sizeof(termination));
        ter->time = job.serviceTime + simulationTime;
        ter->center=DIGEST;
        ter->job = job;
        insertList(ev,ter,1);
    }
    free(termination);
}

void handleNormalArrival(normalAnalysisCenter center, event_list ev){
    arrival *ar = ev.arrivals;
    center.area+=(ar->time-center.lastEventTime)*center.jobs;
    center.jobs++;
    simulationTime = ar->time;
    center.lastEventTime = simulationTime;
    ev.arrivals = ar->next;
    if(center.jobs<=N_NORMAL){
        int server_index = findFreeServer(center.servers,N_NORMAL);
        center.servers[server_index] = 1;
        //The actual service time can be less than the generated service time due to timeout expiration
        center.service_time[server_index]+=min(ar->job.serviceTime,TIMEOUT);
        termination* termination = malloc(sizeof(termination));
        termination->time = ar->job.serviceTime + simulationTime;
        termination->center=NORMAL;
        termination->job = ar->job;
        termination->server_index = server_index;
        insertList(ev,termination,1);
    }
    else{
        job_queue *node = malloc(sizeof(job_queue));
        node->job = ar->job;
        insertQueue(&center.queue,node);
    }
    free(ar);
}

void handleNormalTermination(normalAnalysisCenter center, event_list ev){
    termination* ter = ev.terminations;
    center.area+=(ter->time-center.lastEventTime)*center.jobs;
    center.jobs--;
    center.index++;
    simulationTime = ter->time;
    center.lastEventTime=simulationTime;
    ev.terminations = ter->next;
    if(ter->job.serviceTime > TIMEOUT){
        center.numberOfTimeouts++;
        arrival* ar = malloc(sizeof(arrival));
        ar->center = RELIABLE;
        ar->time =simulationTime;
        ar->job = ter->job;
        //Service time is half because reliable machines are 2x faster than normal machines
        ar->job.serviceTime = ar->job.serviceTime/2;
        insertList(ev,ar,0);
    }
    center.servers[ter->server_index] = 0;
    if(center.jobs>=N_NORMAL){
        int server_index = findFreeServer(center.servers,N_NORMAL);
        center.servers[server_index] = 1;
        job_queue *j_q = popQueue(&center.queue);
        job job = j_q->job;
        free(j_q);
        //The actual service time can be less than the generated service time due to timeout expiration
        center.service_time[server_index]+=min(job.serviceTime,TIMEOUT);
        termination* termi = malloc(sizeof(termination));
        termi->time = job.serviceTime + simulationTime;
        termi->center=NORMAL;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev,termi,1);
    }
    free(ter);
}

void handlePremiumArrival(premiumAnalysisCenter center, event_list ev){
    arrival *ar = ev.arrivals;
    center.area+=(ar->time-center.lastEventTime)*center.jobs;
    center.jobs++;
    simulationTime = ar->time;
    center.lastEventTime = simulationTime;
    ev.arrivals = ar->next;
    if(center.jobs<=N_PREMIUM){
        int server_index = findFreeServer(center.servers,N_PREMIUM);
        center.servers[server_index] = 1;
        //The actual service time can be less than the generated service time due to timeout expiration
        center.service_time[server_index]+=min(ar->job.serviceTime,TIMEOUT);
        termination* termination = malloc(sizeof(termination));
        termination->time = ar->job.serviceTime + simulationTime;
        termination->center=PREMIUM;
        termination->job = ar->job;
        termination->server_index = server_index;
        insertList(ev,termination,1);
    }
    else{
        job_queue *node = malloc(sizeof(job_queue));
        node->job = ar->job;
        insertQueue(&center.queue,node);
    }
    free(ar);
}

void handlePremiumTermination(normalAnalysisCenter center, event_list ev){
    termination* ter = ev.terminations;
    center.area+=(ter->time-center.lastEventTime)*center.jobs;
    center.jobs--;
    center.index++;
    simulationTime = ter->time;
    center.lastEventTime=simulationTime;
    ev.terminations = ter->next;
    if(ter->job.serviceTime > TIMEOUT){
        center.numberOfTimeouts++;
        arrival* ar = malloc(sizeof(arrival));
        ar->center = RELIABLE;
        ar->time =simulationTime;
        ar->job = ter->job;
        //Service time is the same because the machines are the same.
        ar->job.serviceTime = ar->job.serviceTime;
        insertList(ev,ar,0);
    }
    center.servers[ter->server_index] = 0;
    if(center.jobs>=N_PREMIUM){
        int server_index = findFreeServer(center.servers,N_PREMIUM);
        center.servers[server_index] = 1;
        job_queue *j_q = popQueue(&center.queue);
        job job = j_q->job;
        free(j_q);
        //The actual service time can be less than the generated service time due to timeout expiration
        center.service_time[server_index]+=min(job.serviceTime,TIMEOUT);
        termination* termi = malloc(sizeof(termination));
        termi->time = job.serviceTime + simulationTime;
        termi->center=PREMIUM;
        termi->job = job;
        termi->server_index = server_index;
        insertList(ev,termi,1);
    }
    free(ter);
}