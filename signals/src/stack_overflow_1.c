

// https://stackoverflow.com/questions/2850091/wait-on-multiple-condition-variables-on-linux-without-unnecessary-sleeps

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static pthread_cond_t var;
static pthread_mutex_t mtx;

unsigned event_flags = 0;
#define FLAG_EVENT_1    1
#define FLAG_EVENT_2    2

void signal_1()
{
    pthread_mutex_lock(&mtx);
    event_flags |= FLAG_EVENT_1;
    pthread_cond_signal(&var);
    pthread_mutex_unlock(&mtx);
}

void signal_2()
{
    pthread_mutex_lock(&mtx);
    event_flags |= FLAG_EVENT_2;
    pthread_cond_signal(&var);
    pthread_mutex_unlock(&mtx);
}

void* handler(void* p)
{
    // Mutex is unlocked only when we wait or process received events.
    pthread_mutex_lock(&mtx);

    // Here should be race-condition prevention in real code.

    while(1)
    {
        if (event_flags)
        {
            unsigned copy = event_flags;
            event_flags = 0;
            // We unlock mutex while we are processing received events.
            pthread_mutex_unlock(&mtx);

            if (copy & FLAG_EVENT_1)
            {
                printf("EVENT 1\n");
                copy ^= FLAG_EVENT_1;
            }

            if (copy & FLAG_EVENT_2)
            {
                printf("EVENT 2\n");
                copy ^= FLAG_EVENT_2;

                // And let EVENT 2 to be 'quit' signal.
                // In this case for consistency we break with locked mutex.
                pthread_mutex_lock(&mtx);
                break;
            }

            // Note we should have mutex locked at the iteration end.
            pthread_mutex_lock(&mtx);
        }
        else
        {
            // Mutex is locked. It is unlocked while we are waiting.
            pthread_cond_wait(&var, &mtx);
            // Mutex is locked.
        }
    }

    // ... as we are dying.
    pthread_mutex_unlock(&mtx);

    return NULL;
}

int stack_overflow_1_main(int argc, char *argv[]);
int stack_overflow_1_main(int argc, char *argv[])
{
    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&var, NULL);

    pthread_t id;
    pthread_create(&id, NULL, handler, NULL);
    sleep(1);

    signal_1();
    sleep(1);
    signal_1();
    sleep(1);
    signal_2();
    sleep(1);

    pthread_join(id, NULL);
    return 0;
}

