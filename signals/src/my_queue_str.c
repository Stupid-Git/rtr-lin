/*
 * my_queue.c
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>

#define QUEUE_SIZE 10
#define MESSAGE_SIZE 256

typedef struct {
    char messages[QUEUE_SIZE][MESSAGE_SIZE]; // Fixed-size queue
    int head;                               // Index for dequeue
    int tail;                               // Index for enqueue
    int count;                              // Number of messages in the queue
    pthread_mutex_t mutex;                 // Mutex for synchronization
    pthread_cond_t not_empty;              // Condition for waiting when queue is empty
    pthread_cond_t not_full;               // Condition for waiting when queue is full
} tx_queue;

// Initialize the queue
void tx_queue_create(tx_queue *queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
}

int tx_queue_send(tx_queue *queue, const char *message, bool wait) {
    pthread_mutex_lock(&queue->mutex);

    // Wait if the queue is full
    while (queue->count == QUEUE_SIZE) {
        if (!wait) { // Non-blocking behavior
            pthread_mutex_unlock(&queue->mutex);
            return -1; // Queue full
        }
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    // Add the message to the queue
    strncpy(queue->messages[queue->tail], message, MESSAGE_SIZE - 1);
    queue->messages[queue->tail][MESSAGE_SIZE - 1] = '\0'; // Ensure null termination
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->count++;

    // Signal that the queue is no longer empty
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return 0; // Success
}

int tx_queue_receive(tx_queue *queue, char *message, bool wait) {
    pthread_mutex_lock(&queue->mutex);

    // Wait if the queue is empty
    while (queue->count == 0) {
        if (!wait) { // Non-blocking behavior
            pthread_mutex_unlock(&queue->mutex);
            return -1; // Queue empty
        }
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    // Retrieve the message from the queue
    strncpy(message, queue->messages[queue->head], MESSAGE_SIZE - 1);
    message[MESSAGE_SIZE - 1] = '\0'; // Ensure null termination
    queue->head = (queue->head + 1) % QUEUE_SIZE;
    queue->count--;

    // Signal that the queue is no longer full
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return 0; // Success
}

#include <pthread.h>
#include <unistd.h>

void *producer(void *arg) {
    tx_queue *queue = (tx_queue *)arg;
    for (int i = 0; i < 15; i++) {
        char message[MESSAGE_SIZE];
        snprintf(message, MESSAGE_SIZE, "Message %d", i);
        if (tx_queue_send(queue, message, true) == 0) {
            printf("Sent: %s\n", message);
        } else {
            printf("Queue full, could not send: %s\n", message);
        }
        sleep(1); // Simulate work
    }
    return NULL;
}

void *consumer(void *arg) {
    tx_queue *queue = (tx_queue *)arg;
    for (int i = 0; i < 15; i++) {
        char message[MESSAGE_SIZE];
        if (tx_queue_receive(queue, message, true) == 0) {
            printf("Received: %s\n", message);
        } else {
            printf("Queue empty, could not receive\n");
        }
        sleep(2); // Simulate work
    }
    return NULL;
}

int my_queue_main();
int my_queue_main()
{
    tx_queue queue;
    tx_queue_create(&queue);

    pthread_t producer_thread, consumer_thread;
    pthread_create(&producer_thread, NULL, producer, &queue);
    pthread_create(&consumer_thread, NULL, consumer, &queue);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    return 0;
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

int tx_queue_send2(tx_queue *queue, const char *message, unsigned long timeout_ticks, unsigned int tick_rate_ms) {
    struct timespec timeout;
    if (timeout_ticks != 0) {
        calculate_timeout(&timeout, timeout_ticks, tick_rate_ms);
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count == QUEUE_SIZE) {
        if (timeout_ticks == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return -1; // Queue full and non-blocking
        }
        if (pthread_cond_timedwait(&queue->not_full, &queue->mutex, &timeout) == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return -2; // Timeout
        }
    }

    // Add the message to the queue
    strncpy(queue->messages[queue->tail], message, MESSAGE_SIZE - 1);
    queue->messages[queue->tail][MESSAGE_SIZE - 1] = '\0'; // Ensure null termination
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->count++;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return 0; // Success
}

int tx_queue_receive2(tx_queue *queue, char *message, unsigned long timeout_ticks, unsigned int tick_rate_ms) {
    struct timespec timeout;
    if (timeout_ticks != 0) {
        calculate_timeout(&timeout, timeout_ticks, tick_rate_ms);
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0) {
        if (timeout_ticks == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return -1; // Queue empty and non-blocking
        }
        if (pthread_cond_timedwait(&queue->not_empty, &queue->mutex, &timeout) == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return -2; // Timeout
        }
    }

    // Retrieve the message from the queue
    strncpy(message, queue->messages[queue->head], MESSAGE_SIZE - 1);
    message[MESSAGE_SIZE - 1] = '\0'; // Ensure null termination
    queue->head = (queue->head + 1) % QUEUE_SIZE;
    queue->count--;

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return 0; // Success
}

#define TICK_RATE_MS 10

void *producer2(void *arg) {
    tx_queue *queue = (tx_queue *)arg;
    for (int i = 0; i < 15; i++) {
        char message[MESSAGE_SIZE];
        snprintf(message, MESSAGE_SIZE, "Message %d", i);
        int result = tx_queue_send2(queue, message, 100, TICK_RATE_MS); // 100 ticks = 1 second
        if (result == 0) {
            printf("Sent: %s\n", message);
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
    tx_queue *queue = (tx_queue *)arg;
    for (int i = 0; i < 15; i++) {
        char message[MESSAGE_SIZE];
        int result = tx_queue_receive2(queue, message, 50, TICK_RATE_MS); // 50 ticks = 500 ms
        if (result == 0) {
            printf("Received: %s\n", message);
        } else if (result == -1) {
            printf("Queue empty, non-blocking receive failed\n");
        } else if (result == -2) {
            printf("Queue empty, receive timed out\n");
        }
        sleep(2); // Simulate work
    }
    return NULL;
}

int my_queue_main2();
int my_queue_main2()
{
    tx_queue queue;
    tx_queue_create(&queue);

    pthread_t producer_thread, consumer_thread;
    pthread_create(&producer_thread, NULL, producer2, &queue);
    pthread_create(&consumer_thread, NULL, consumer2, &queue);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    return 0;
}




