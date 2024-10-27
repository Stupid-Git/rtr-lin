


#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include <stdint.h>
#include <string.h>

//static pthread_cond_t var;
//static pthread_mutex_t mtx;

#define FLAG_EVENT_1    1
#define FLAG_EVENT_2    2


typedef struct
{
	char name[32];
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    uint32_t flags;
} myflags_t;

myflags_t g_MyFlags;

int MF_init(myflags_t* mf, char * name);

int MF_init(myflags_t* mf, char * name)
{
    pthread_mutex_init(&mf->lock, NULL);
    pthread_cond_init(&mf->cond, NULL);
    strcpy(mf->name, name);
    mf->flags = 0x0;

    return 0;
}

int MF_set(myflags_t* mf, uint32_t set_flags)
{
	// Just OR for now
    pthread_mutex_lock(&mf->lock);
    mf->flags = set_flags;
    pthread_cond_signal(&mf->cond);
    pthread_mutex_unlock(&mf->lock);

    return 0;
}

int MF_get(myflags_t* mf, uint32_t mask_flags, uint32_t *actual_flags, uint32_t timeout)
{

	uint32_t flags_A;

    pthread_mutex_lock(&mf->lock);

    flags_A = mf->flags & mask_flags;
    if((flags_A != 0) || (timeout == 0))
    {
    	*actual_flags = flags_A;
    	mf->flags &= ~flags_A;
        pthread_mutex_unlock(&mf->lock);
        return 0;
    }

    // Don't have yet so wait
    pthread_cond_wait(&mf->cond, &mf->lock);

    flags_A = mf->flags & mask_flags;
    if(flags_A != 0)
    {
    	*actual_flags = flags_A;
    	mf->flags &= ~flags_A;
        pthread_mutex_unlock(&mf->lock);
        return 0;
    }

    pthread_mutex_unlock(&mf->lock);

    return 1;
}


static void signal_1()
{
	MF_set(&g_MyFlags, FLAG_EVENT_1);
}

static void signal_2()
{
	MF_set(&g_MyFlags, FLAG_EVENT_2);
}

static void signal_1_2()
{
	MF_set(&g_MyFlags, FLAG_EVENT_1 | FLAG_EVENT_2);
}

void* my_thread(void* p)
{
	uint32_t status;

	uint32_t mask_flags;
	uint32_t actual_flags;
	uint32_t timeout;

	mask_flags = 0xFFFFFFFF;
	actual_flags = 0;
	timeout = 0;
	MF_get(&g_MyFlags, mask_flags, &actual_flags, timeout);

    while(1)
    {

    	mask_flags = 0xFFFFFFFF;
    	actual_flags = 0;
    	timeout = 1;
    	status = MF_get(&g_MyFlags, mask_flags, &actual_flags, timeout);

    	if (status == 0)
        {
    		printf("actual_flags = 0x%08x\n", actual_flags);
        }
        else
        {
    		printf("MF_get failed. status = 0x%08x\n", status);
        }

    	if(actual_flags == 0x00000003)
    		break;
    }

    return NULL;
}


int my_event_main(int argc, char *argv[]);
int my_event_main(int argc, char *argv[])
{

	printf("[0]\n");

	MF_init(&g_MyFlags, "name");
	printf("[1]\n");

    pthread_t id;
    pthread_create(&id, NULL, my_thread, NULL);
	printf("[2]\n");
    sleep(1);

    signal_1();
	printf("[3]\n");
    sleep(1);
    signal_1();
	printf("[4]\n");
    sleep(1);
    signal_2();
	printf("[5]\n");
    sleep(1);
    signal_1_2();
	printf("[6]\n");
    sleep(1);

    pthread_join(id, NULL);
	printf("[6]\n");
    return 0;
}

