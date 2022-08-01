#include "job.h"
void insertQueue(job_queue **queue, job_queue *node){

    job_queue *head = *queue;

    job_queue *tail = NULL;

    while(head!=NULL){
        tail = head;
        head = head->next;
    }
    if(tail==NULL){
        *queue = node;
        node->next = NULL;
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