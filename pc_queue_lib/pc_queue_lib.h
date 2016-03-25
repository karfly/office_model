#ifndef PC_QUEUE_H
#define PC_QUEUE_H

#include <stdio.h>
#include <pthread.h>
#include <malloc.h>

typedef struct pc_queue_elem_t {

	struct pc_queue_elem_t * next;
	void * value;
} pc_queue_elem_t;

typedef struct pc_queue_t {

        struct pc_queue_elem_t * head;
        struct pc_queue_elem_t * tail;

        pthread_mutex_t mutex;

        pthread_cond_t not_empty_cond;
        pthread_cond_t not_full_cond;

        size_t size;
        size_t load;
} pc_queue_t;

// pc_queue_t * pc_queue_create (size_t size);
// int pc_queue_destroy (pc_queue_t * pc_queue);
// int pc_queue_put (pc_queue_t * pc_queue, void * value);
// void * pc_queue_get (pc_queue_t * pc_queue);

/**
 * [pc_queue_create  Creates new queue with length given in size.]
 * 
 * @param       size            Size of queue. Must be positive
 * 
 * @return      NOT NULL        Pointer to the new queue
 *              NULL            Some error happend
 */
 pc_queue_t *
 pc_queue_create (size_t size) {

        if (size <= 0) {
                fprintf (stderr, "pc_queue_lib: Invalid value [%d] for size of queue. It must be positive.\n", (int) size);
                return NULL;
        }

        pc_queue_t * sample_pc_queue = (pc_queue_t *) calloc (1, sizeof(pc_queue_t));
        if (!sample_pc_queue) {
                fputs ("pc_queue_lib: Can't allocate memory for a new pc_queue.\n", stderr);
                return NULL;
        }

        sample_pc_queue->head = NULL;
        sample_pc_queue->tail = NULL;

        pthread_mutex_init (&sample_pc_queue->mutex, NULL);

        pthread_cond_init (&sample_pc_queue->not_empty_cond, NULL);
        pthread_cond_init (&sample_pc_queue->not_full_cond, NULL);

        sample_pc_queue->size = size;
        sample_pc_queue->load = 0;

        return sample_pc_queue;
}

/**
 * [pc_queue_destroy  Destroys pc_queue.]
 * 
 * @param       pc_queue        Pointer to the queue
 * 
 * @return      0               On success
 *              -1              Invalid pointer to the queue
 */
int
pc_queue_destroy (pc_queue_t * pc_queue) {

        if (!pc_queue) {
                fputs ("pc_queue_lib: Failed to destroy queue. Invalid pointer to the queue.\n", stderr);
                return -1;
        }

        pc_queue->load = -1;       
        pc_queue->size = -1;

        pthread_cond_destroy (&pc_queue->not_full_cond);
        pthread_cond_destroy (&pc_queue->not_empty_cond);

        pthread_mutex_destroy (&pc_queue->mutex);

        free(pc_queue);

        return 0;
}
/**
 * [pc_queue_put  Puts value to the pc_queue.]
 * 
 * @param       pc_queue        Pointer to the queue
 * @param       value           Data, you put to the queue
 * 
 * @return      0               On success
 *              -1              If some error occured
 *
 * @note        This function is blocking. So if put element
 *              and queue is overloaded function blocks until
 *              there is empty space for element.
 */
int
pc_queue_put (pc_queue_t * pc_queue, void * value) {

        // +++ Wrapping value into pc_queue_elem_t
        pc_queue_elem_t * sample_pc_queue_elem = (pc_queue_elem_t *) calloc (1, sizeof(pc_queue_elem_t));
        if (!sample_pc_queue_elem) {
                fputs ("pc_queue_lib: Can't allocate memory for a new pc_queue_elem\n", stderr);
                return -1;
        }
        sample_pc_queue_elem->next = NULL;
        sample_pc_queue_elem->value = value;            // pc_queue_lib doesn't do any copies of data.
                                                        // User is responsible for it.

        // +++ Critical zone begins
        pthread_mutex_lock (&pc_queue->mutex);

        while (pc_queue->load >= pc_queue->size) {
                pthread_cond_wait (&pc_queue->not_full_cond, &pc_queue->mutex);
        }

        if (pc_queue->tail) {
                pc_queue->tail->next = sample_pc_queue_elem;
        }
        else {
                pc_queue->head = sample_pc_queue_elem;
                pthread_cond_broadcast (&pc_queue->not_empty_cond);
        }
        pc_queue->tail = sample_pc_queue_elem;

        pc_queue->load++;

        pthread_mutex_unlock (&pc_queue->mutex);
        // +++ End of critical zone

        return 0;
}

/**
 * [pc_queue_get  Gets element from the pc_queue]
 * 
 * @param  pc_queue Pointer to the queue
 * 
 * @return      NOT NULL     Pointer to the value
 *              NULL         It means the end of transmitting tasks. NULL is sent by
 *                           pc_queue_stop to tell consumer, that tasks are over
 *                      
 * @note        This function is blocking. So if put element
 *              and queue is overloaded function blocks until
 *              there is empty space for element.
 */
void *
pc_queue_get (pc_queue_t * pc_queue) {

        pc_queue_elem_t * tmp = NULL;

        // +++ Critical zone begins
        pthread_mutex_lock (&pc_queue->mutex);       

        while (!pc_queue->head) {
                pthread_cond_wait (&pc_queue->not_empty_cond, &pc_queue->mutex);
        }
       
        tmp = pc_queue->head;

        pc_queue->head = pc_queue->head->next;
        if (!pc_queue->head) {
                pc_queue->tail = NULL;
        }

        pc_queue->load--;

        if (pc_queue->load + 1 == pc_queue->size) {
                pthread_cond_broadcast(&pc_queue->not_full_cond);
        }

        pthread_mutex_unlock (&pc_queue->mutex);
        // +++ End of critical zone
        
        void * value = tmp->value;
        free(tmp);
        return value;
}

/**
 * [pc_queue_stop  Send stop-element to the pc_queue.]
 * 
 * @param       pc_queue        Pointer to the queue
 *
 * @note        Equal the call of pc_queue_put (pc_queue, NULL).
 */
void
pc_queue_stop (pc_queue_t * pc_queue) {

        pc_queue_put (pc_queue, NULL);

        /*
        // +++ Critical zone begins
        pthread_mutex_lock (&pc_queue->mutex);

        pc_queue->stopped = 1;

        pthread_cond_broadcast(&pc_queue->not_full_cond);
        pthread_cond_broadcast(&pc_queue->not_empty_cond);

        pthread_mutex_unlock (&pc_queue->mutex);
        // +++ End of critical zone
        printf("stop: Stopped\n");
        */

}

#endif