#include<stdlib.h>
#include<stdio.h>
typedef enum __userType{
    PREMIUM = 1,
    NORMAL = 0
}UserType;

typedef enum __jobType{
    MALWARE = 1,
    BENIGN = 0
}JobType;

typedef struct __job{
    UserType userType; // Job submitted by a premium user or a normal one
    JobType type; // Job type: malware, benign
    double serviceTime; // Service time 
}job;

typedef struct __job_queue{
    job job;
    struct __job_queue *next;
}job_queue;

void insertQueue(job_queue **queue, job_queue *node){
    job_queue *head = *queue;
    job_queue *tail = NULL;
    while(head!=NULL){
        tail = head;
        head = head->next;
    }
    if(tail==NULL){
        *queue = node;
    }
    else{
        tail->next = node;
        node -> next = NULL;
    }
}

job_queue* popQueue(job_queue **queue){
    job_queue *toReturn = *queue;
    if(toReturn == NULL){
        return toReturn;
    }
    *queue = (*queue)->next;
    return toReturn;
}