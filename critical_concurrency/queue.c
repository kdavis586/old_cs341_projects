/**
 * critical_concurrency
 * CS 341 - Fall 2022
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    /* Your code here */
    queue * new_q = (queue *) malloc(sizeof(queue));
    new_q->max_size = max_size;
    new_q->size = 0;
    new_q->head = NULL;
    new_q->tail = NULL;
    pthread_mutex_init(&(new_q->m), NULL);
    pthread_cond_init(&(new_q->cv), NULL);

    return new_q;
}

void queue_destroy(queue *this) {
    /* Your code here */
    if (this) {
        queue_node * cur = this->head;
        while (cur) {
            free(cur->data);
            queue_node * temp = cur;
            cur = cur->next;
            free(temp);
        }
        // TODO: Maybe free queue "this" as well
        free(this);
        this = NULL;
    }
}

void queue_push(queue *this, void *data) {
    /* Your code here */
    pthread_mutex_lock(&(this->m));
    if (this->max_size >= 0) {
        while (this->size == this->max_size) {
            pthread_cond_wait(&(this->cv), &(this->m));
        }
    }
    queue_node * new_node = (queue_node *) malloc(sizeof(queue_node));
    new_node->data = data;
    new_node->next = NULL;
    if (this->size == 0) {
        this->head = new_node;
        this->tail = new_node;
    } else {
        this->tail->next = new_node;
        this->tail = new_node;
    }
    this->size++;
    pthread_cond_broadcast(&(this->cv));
    pthread_mutex_unlock(&(this->m));
}

void *queue_pull(queue *this) {
    /* Your code here */
    pthread_mutex_lock(&(this->m));
    while (this->size == 0) {
        pthread_cond_wait(&(this->cv), &(this->m));
    }

    void * return_data = this->head->data;
    if (this->size == 1) {
        free(this->head);
        this->head = NULL;
        this->tail = NULL;
    } else {
        queue_node * temp = this->head;
        this->head = this->head->next;
        free(temp);
    }
    this->size--;
    pthread_cond_broadcast(&(this->cv));
    pthread_mutex_unlock(&(this->m));

    return return_data;
}
