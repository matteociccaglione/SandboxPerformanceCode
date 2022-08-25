/**
 * @file job.c
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-08-24
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file contains implementations of functions to insert and remove a job in/from a queue.
 * It also implements the function to get the queue length.
 */
#include "job.h"

/**
 * @brief Insert a new node in the queue structure. FIFO discipline is used.
 * 
 * @param queue Pointer to the queue head
 * @param node Pointer to the job_queue node that needs to be added
 */
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

/**
 * @brief Remove the head of the queue and move the head to the next node.
 * 
 * @param queue Pointer to the head of the queue
 * @return job_queue* Returns a pointer to the removed node
 */
job_queue* popQueue(job_queue **queue){
    job_queue *toReturn = *queue;
    if(toReturn == NULL){
        return toReturn;
    }
    *queue = (*queue)->next;
    return toReturn;
}

/**
 * @brief This function returns the length of the queue.
 * 
 * @param queue Pointer to the head of the queue
 * @return int Returns the number of nodes in the queue
 */
int sizeQueue(job_queue *queue){
    job_queue *head = queue;
    int size = 0;
    while (head!=NULL){
        size++;
        head=head->next;
    }
    return size;
}