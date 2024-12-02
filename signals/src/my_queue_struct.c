/*
 * my_queue_struct.c
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

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
} tdx_queue;

// Create the queue
int tdx_queue_create(tdx_queue *queue, void *queue_memory, size_t queue_size, size_t message_size) {
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

int tdx_queue_delete(tdx_queue *queue)
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
static void calculate_timeout(struct timespec *timeout, unsigned long timeout_ticks, unsigned int tick_rate_ms) {
    clock_gettime(CLOCK_REALTIME, timeout);
    unsigned long timeout_ns = timeout_ticks * tick_rate_ms * 1000000L;
    timeout->tv_sec += timeout_ns / 1000000000L;
    timeout->tv_nsec += timeout_ns % 1000000000L;
    if (timeout->tv_nsec >= 1000000000L) {
        timeout->tv_sec++;
        timeout->tv_nsec -= 1000000000L;
    }
}

int tdx_queue_send(tdx_queue *queue, const void *message, unsigned long timeout_ticks, unsigned int tick_rate_ms) {
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

int tdx_queue_receive(tdx_queue *queue, void *message, unsigned long timeout_ticks, unsigned int tick_rate_ms)
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


typedef struct {
    int command_id;
    char data[50];
} command_t;

#define QUEUE_MEMORY_SIZE 10
#define TICK_RATE_MS 10

tdx_queue cmd_queue;
command_t queue_memory[QUEUE_MEMORY_SIZE]; // Pre-allocated queue memory


int my_queue_struct_main();
int my_queue_struct_main()
{
    // Create the queue
    if (tdx_queue_create(&cmd_queue, queue_memory, QUEUE_MEMORY_SIZE, sizeof(command_t)) != 0) {
        fprintf(stderr, "Failed to create queue\n");
        return -1;
    }

    // Example: Sending to the queue
    command_t cmd = {1, "Test Command"};
    if (tdx_queue_send(&cmd_queue, &cmd, 100, TICK_RATE_MS) == 0) {
        printf("Message sent: Command ID = %d, Data = %s\n", cmd.command_id, cmd.data);
    } else {
        printf("Failed to send message\n");
    }

    // Example: Receiving from the queue
    command_t received_cmd;
    if (tdx_queue_receive(&cmd_queue, &received_cmd, 100, TICK_RATE_MS) == 0) {
        printf("Message received: Command ID = %d, Data = %s\n", received_cmd.command_id, received_cmd.data);
    } else {
        printf("Failed to receive message\n");
    }

    tdx_queue_delete(&cmd_queue);

    return 0;
}


void *producer2(void *arg) {
    tdx_queue *queue = (tdx_queue *)arg;
    for (int i = 0; i < 15; i++) {

    	//char message[MESSAGE_SIZE];
        //snprintf(message, MESSAGE_SIZE, "Message %d", i);
        command_t send_cmd;
        snprintf(send_cmd.data, 50/*MESSAGE_SIZE*/, "Message %d", i);
        send_cmd.command_id = i;

//        int result = tdx_queue_send(&cmd_queue, &cmd, 100, TICK_RATE_MS) == 0) {
        int result = tdx_queue_send(queue, &send_cmd, 100, TICK_RATE_MS); // 100 ticks = 1 second
        if (result == 0) {
            printf("Sent: %s\n", send_cmd.data);
        } else if (result == -1) {
            printf("Queue full, non-blocking send failed\n");
        } else if (result == -2) {
            printf("Queue full, send timed out\n");
        }
        sleep(1); // Simulate work
    }
    return NULL;
}

void *consumer2(void *arg) {
    tdx_queue *queue = (tdx_queue *)arg;
    for (int i = 0; i < 15; i++) {

        //char message[MESSAGE_SIZE];
        command_t received_cmd;

        int result = tdx_queue_receive(queue, &received_cmd, 50, TICK_RATE_MS); // 50 ticks = 500 ms
        if (result == 0) {
            printf("Received: [%d] : '%s'\n", received_cmd.command_id, received_cmd.data);
        } else if (result == -1) {
            printf("Queue empty, non-blocking receive failed\n");
        } else if (result == -2) {
            printf("Queue empty, receive timed out\n");
        }
        sleep(2); // Simulate work
    }
    return NULL;
}

int my_queue_main3();
int my_queue_main3()
{
    if (tdx_queue_create(&cmd_queue, queue_memory, QUEUE_MEMORY_SIZE, sizeof(command_t)) != 0) {
        fprintf(stderr, "Failed to create queue\n");
        return -1;
    }

    pthread_t producer_thread, consumer_thread;
    pthread_create(&producer_thread, NULL, producer2, &cmd_queue);
    pthread_create(&consumer_thread, NULL, consumer2, &cmd_queue);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    return 0;
}

