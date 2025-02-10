
#include "tdx.h"


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

/*
 * Other Shared memory info
 * How to Set up Shared Memory in Your Linux and MacOS Programs. (shmget, shmat, shmdt, shmctl, ftok)
 *   https://www.youtube.com/watch?v=WgVSq-sgHOc&ab_channel=JacobSorber
 *
 */

//=============================================================================
//
// THREAD
//
//=============================================================================

uint32_t txd_thread_sleep(uint32_t timer_ticks)
{
    //TODO
    return 0;
}

//=============================================================================
//
// MUTEX
//
//=============================================================================
int tx_mutex_get(pthread_mutex_t* mutex, uint32_t timeout)
{
    return 0;
}
int tx_mutex_put(pthread_mutex_t* mutex)
{
    return 0;
}

//=============================================================================
//
// FLAGS
//
//=============================================================================


int tdx_flags_create(tdx_flags_t* tdxf, char * name)
{
    pthread_mutex_init(&tdxf->lock, NULL);
    pthread_cond_init(&tdxf->cond, NULL);
    strcpy(tdxf->name, name);
    tdxf->flags = 0x0;

    return 0;
}

int tdx_flags_delete(tdx_flags_t* tdxf)
{
	//TODO
    pthread_mutex_destroy(&tdxf->lock);
    pthread_cond_destroy(&tdxf->cond);
    tdxf->flags = 0x0;

    return 0;
}

int tdx_flags_set(tdx_flags_t* tdxf, uint32_t set_flags)
{
	// Just OR for now
    pthread_mutex_lock(&tdxf->lock);
    tdxf->flags = set_flags;
    pthread_cond_signal(&tdxf->cond);
    pthread_mutex_unlock(&tdxf->lock);

    return 0;
}

int tdx_flags_get(tdx_flags_t* tdxf, uint32_t mask_flags, uint32_t *actual_flags, uint32_t timeout)
{

	uint32_t flags_A;

    pthread_mutex_lock(&tdxf->lock);

    flags_A = tdxf->flags & mask_flags;
    if((flags_A != 0) || (timeout == 0))
    {
    	*actual_flags = flags_A;
    	tdxf->flags &= ~flags_A;
        pthread_mutex_unlock(&tdxf->lock);
        return 0;
    }

    // Don't have yet so wait
    pthread_cond_wait(&tdxf->cond, &tdxf->lock);

    flags_A = tdxf->flags & mask_flags;
    if(flags_A != 0)
    {
    	*actual_flags = flags_A;
    	tdxf->flags &= ~flags_A;
        pthread_mutex_unlock(&tdxf->lock);
        return 0;
    }

    pthread_mutex_unlock(&tdxf->lock);

    return 1;
}



//=============================================================================
//
// QUEUE
//
//=============================================================================

// Create the queue
int tdx_queue_create(tdx_queue_t *queue, void *queue_memory, size_t queue_size, size_t message_size)
{
    if (!queue || !queue_memory || queue_size == 0 || message_size == 0) {
        return -1; // Invalid parameters
    }
    queue->queue_memory = queue_memory;
    queue->message_size = message_size;
    queue->queue_size = queue_size;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    return 0; // Success
}

int tdx_queue_delete(tdx_queue_t *queue)
{
    if (!queue ) {
        return -1; // Invalid parameters
    }
    queue->queue_memory = 0;
    queue->message_size = 0;
    queue->queue_size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    return 0; // Success
}

#include <time.h>
#include <errno.h>

// Convert ticks to absolute timespec for pthread_cond_timedwait
static void calculate_timeout(struct timespec *timeout, unsigned long timeout_ticks, unsigned int tick_rate_ms)
{
    clock_gettime(CLOCK_REALTIME, timeout);
    unsigned long timeout_ns = timeout_ticks * tick_rate_ms * 1000000L;
    timeout->tv_sec += timeout_ns / 1000000000L;
    timeout->tv_nsec += timeout_ns % 1000000000L;
    if (timeout->tv_nsec >= 1000000000L) {
        timeout->tv_sec++;
        timeout->tv_nsec -= 1000000000L;
    }
}

int tdx_queue_send(tdx_queue_t *queue, const void *message, unsigned long timeout_ticks, unsigned int tick_rate_ms)
{
    struct timespec timeout;
    if (timeout_ticks != 0) {
        calculate_timeout(&timeout, timeout_ticks, tick_rate_ms);
    }

    pthread_mutex_lock(&queue->mutex);

    // Wait if the queue is full
    while (queue->count == queue->queue_size) {
        if (timeout_ticks == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return -1; // Queue full, non-blocking
        }
        if (pthread_cond_timedwait(&queue->not_full, &queue->mutex, &timeout) == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return -2; // Timeout
        }
    }

    // Copy the message into the queue
    void *destination = (char *)queue->queue_memory + (queue->tail * queue->message_size);
    memcpy(destination, message, queue->message_size);
    queue->tail = (queue->tail + 1) % queue->queue_size;
    queue->count++;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return 0; // Success
}

int tdx_queue_create(tdx_queue_t *queue, void *queue_memory, size_t queue_size, size_t message_size);
int tdx_queue_delete(tdx_queue_t *queue);
int tdx_queue_send(tdx_queue_t *queue, const void *message, unsigned long timeout_ticks, unsigned int tick_rate_ms);
int tdx_queue_receive(tdx_queue_t *queue, void *message, unsigned long timeout_ticks, unsigned int tick_rate_ms);
int tdx_queue_receive(tdx_queue_t *queue, void *message, unsigned long timeout_ticks, unsigned int tick_rate_ms)
{
    struct timespec timeout;
    if (timeout_ticks != 0) {
        calculate_timeout(&timeout, timeout_ticks, tick_rate_ms);
    }

    pthread_mutex_lock(&queue->mutex);

    // Wait if the queue is empty
    while (queue->count == 0) {
        if (timeout_ticks == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return -1; // Queue empty, non-blocking
        }
        if (pthread_cond_timedwait(&queue->not_empty, &queue->mutex, &timeout) == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return -2; // Timeout
        }
    }

    // Copy the message from the queue
    void *source = (char *)queue->queue_memory + (queue->head * queue->message_size);
    memcpy(message, source, queue->message_size);
    queue->head = (queue->head + 1) % queue->queue_size;
    queue->count--;

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return 0; // Success
}




