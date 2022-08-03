#ifndef __stats_h__
    #define __stats_h__
#include<stdio.h>
#include<stdlib.h>
typedef struct __stats{
    int numJobs; // number of processed jobs
    int numNormalJobs; // number of processed jobs sumbitted by Normal users
    int numPremiumJobs; //number of processed jobs sumbitted by Premium users
    double realSimulationTime; // real duration in seconds of the simulation run

    double responseTime[4]; // avg response time in the four centers
    double waitTime[4]; // avg wait time in the four centers
    int numDigestMatching; // number of jobs whose digest has matched
    double avgNumberOFJobs[4]; // E(N), average number of jobs in the four centers
    double interarrivalTime[4]; // 1/lambda for the 4 centers
    double serviceTime[4]; // avg service time in the four centers
    struct __stats *next; // pointer to the next struct in the list

    double numOfTimeouts[3]; // Num of timeouts in the last 3 centers

}stats;

void insertListStats(stats** head, stats *s){
    stats* temp = *head;
    stats* tail = NULL;

    printf("%p\n",temp);
    while(temp!=NULL){
        tail=temp;

        temp=temp->next;

    }

    if(tail==NULL){
        *head = s;

        s->next=NULL;
    }
    else{
        tail->next = s;
        s->next = NULL;
    }
}
#endif