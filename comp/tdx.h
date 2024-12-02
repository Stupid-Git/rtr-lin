/*
 * tdx.h
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#ifndef TDX_H_
#define TDX_H_

#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>


#define TX_SUCCESS 0

#define TDX_NO_WAIT 0
#define TDX_WAIT_FOREVER -1

#define TX_NO_WAIT                      ((uint32_t)  0)
#define TX_WAIT_FOREVER                 ((uint32_t)  0xFFFFFFFFUL)
#define TX_AND                          ((uint32_t)   2)
#define TX_AND_CLEAR                    ((uint32_t)   3)
#define TX_OR                           ((uint32_t)   0)
#define TX_OR_CLEAR                     ((uint32_t)   1)

#define tx_thread_sleep txd_thread_sleep
uint32_t txd_thread_sleep(uint32_t timer_ticks);

#define TX_SUCCESS 0
int tx_mutex_get(pthread_mutex_t* mutex, uint32_t timeout);
int tx_mutex_put(pthread_mutex_t* mutex);


typedef struct
{
	char name[32];
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    uint32_t flags;
} tdx_flags_t;


//  tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);
//  tx_event_flags_get (&optc_event_flags, FLG_OPTC_END, TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER); //光通信コマンド終了イベント待ち
#define tx_event_flags_set(A, B, C) tdx_flags_set(A, B)
#define tx_event_flags_get(A, B, C, D, E) tdx_flags_get(A, B, D, E)

int tdx_flags_create(tdx_flags_t* tdxf, char * name);
int tdx_flags_delete(tdx_flags_t* tdxf);
int tdx_flags_set(tdx_flags_t* tdxf, uint32_t set_flags);
int tdx_flags_get(tdx_flags_t* tdxf, uint32_t mask_flags, uint32_t *actual_flags, uint32_t timeout);


typedef struct {
    void *queue_memory;  // Pointer to user-defined memory for storing messages
    size_t message_size; // Size of each message (in bytes)
    size_t queue_size;   // Total number of messages the queue can hold
    int head;            // Index for dequeue
    int tail;            // Index for enqueue
    int count;           // Number of messages currently in the queue
    pthread_mutex_t mutex;  // Mutex for synchronization
    pthread_cond_t not_empty; // Condition for waiting when queue is empty
    pthread_cond_t not_full;  // Condition for waiting when queue is full
} tdx_queue_t;

#define tx_queue_send(A, B, C) tdx_queue_send(A, B, C, 10)

int tdx_queue_create(tdx_queue_t *queue, void *queue_memory, size_t queue_size, size_t message_size);
int tdx_queue_delete(tdx_queue_t *queue);
int tdx_queue_send(tdx_queue_t *queue, const void *message, unsigned long timeout_ticks, unsigned int tick_rate_ms);
int tdx_queue_receive(tdx_queue_t *queue, void *message, unsigned long timeout_ticks, unsigned int tick_rate_ms);


#endif /* TDX_H_ */
