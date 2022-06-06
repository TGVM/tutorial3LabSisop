#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/sched.h>

static char *buffer = NULL;
static int *policies = NULL;
static int *priorities = NULL;
int buffer_size = 0;
int buffer_index = 0;

pthread_mutex_t semaphore;

void print_sched(int policy)
{
	int priority_min, priority_max;

	switch(policy){
		case SCHED_DEADLINE:
			printf("SCHED_DEADLINE");
			break;
		case SCHED_FIFO:
			printf("SCHED_FIFO");
			break;
		case SCHED_RR:
			printf("SCHED_RR");
			break;
		case SCHED_NORMAL:
			printf("SCHED_OTHER");
			break;
		case SCHED_BATCH:
			printf("SCHED_BATCH");
			break;
		case SCHED_IDLE:
			printf("SCHED_IDLE");
			break;
		default:
			printf("unknown\n");
	}
	priority_min = sched_get_priority_min(policy);
	priority_max = sched_get_priority_max(policy);
	printf(" PRI_MIN: %d PRI_MAX: %d\n", priority_min, priority_max);
}

int setpriority(pthread_t *thr, int newpolicy, int newpriority)
{
	int policy, ret;
	struct sched_param param;

	if (newpriority > sched_get_priority_max(newpolicy) || newpriority < sched_get_priority_min(newpolicy)){
		printf("Invalid priority: MIN: %d, MAX: %d", sched_get_priority_min(newpolicy), sched_get_priority_max(newpolicy));

		return -1;
	}

	pthread_getschedparam(*thr, &policy, &param);
	printf("current: ");
	print_sched(policy);

	param.sched_priority = newpriority;
	ret = pthread_setschedparam(*thr, newpolicy, &param);
	if (ret != 0)
		perror("perror(): ");

	pthread_getschedparam(*thr, &policy, &param);
	printf("new: ");
	print_sched(policy);

	return 0;
}

void *char_printer(void *input) {
	char c = (char)input;

	while(buffer_index < buffer_size-1) {
		// Critica
		pthread_mutex_lock(&semaphore);
			buffer[buffer_index] = c;
			buffer_index++;
		pthread_mutex_unlock(&semaphore);
		
		if(buffer_index>=buffer_size)
			return 0;
	}
}



int main(int argc, char **argv)
{
	//thread_runner <número_de_threads> <tamanho_do_buffer_global_em_kilobytes> <politica> <prioridade>

	if (argc < 5){
		printf("usage: %s <número_de_threads> <tamanho_do_buffer_global_em_kilobytes> <politica thr0> <prioridade thr0> ... <politica thrN> <prioridade thrN>\n\n", argv[0]);

		return 0;
	}

	pthread_t thr[atoi(argv[1])];
	buffer_size = atoi(argv[2])*1024;
	buffer = (char *)malloc((buffer_size-1)*sizeof(char));
	policies = (int *)malloc((buffer_size-1)*sizeof(int));
	priorities = (int *)malloc((buffer_size-1)*sizeof(int));
	//int thr_policy = atoi(argv[3]);
	//int thr_priority = atoi(argv[4]);
	
	// Set policy and priority for each thread
	int j=0;
	for(int i=3; i<argc; i=i+2) {
		//printf("argv %i : %i", i, atoi(argv[i]));
		policies[j] = atoi(argv[i]);
		//printf("argv %i : %i", i+1, atoi(argv[i+1]));
		priorities[j] = atoi(argv[i+1]);

		j++;
	}

	/*printf("[");
	for(int i=0; i<atoi(argv[1]); i++) {
		printf("%i,",policies[i]);
	}
	printf("]");

	printf("[");
	for(int i=0; i<atoi(argv[1]); i++) {
		printf("%i,",priorities[i]);
	}
	printf("]");*/

	if (pthread_mutex_init(&semaphore, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

	// Threads wait until everyone is created
	pthread_mutex_lock(&semaphore);
	for(int i=0; i<atoi(argv[1]); i++) {
		printf("thread %i : char %c ; policy %i ; priority %i\n", i, (97+i), policies[i], priorities[i]);
		pthread_create(&(thr[i]), NULL, char_printer, (void *)(97+i));
		//setpriority(&(thr[i]), thr_policy, thr_priority);
		setpriority(&(thr[i]), policies[i], priorities[i]);
	}
	//Go go go
	pthread_mutex_unlock(&semaphore);

	// Wait for every thread to return
	for(int i=0; i<atoi(argv[1]); i++) {
		pthread_join(thr[i], NULL);
	}

	printf("\n%i\n\n", buffer_size);
	printf("\n%s\n",buffer);


	char *normalized_buffer = (char *)malloc((buffer_size-1)*sizeof(char));
	int normalize_index = 0;
	char last_char = buffer[0];

	for(int i=0; i<buffer_size-1; i++) {
		if(buffer[i] != last_char) {
			normalized_buffer[normalize_index] = last_char;
			normalize_index++;
		}
		last_char = buffer[i];
	}
	normalized_buffer[normalize_index] = last_char;

	printf("\n\n%s\n\n", normalized_buffer);
	
	// pthread_create(&thr, NULL, run, NULL);
	// setpriority(&thr, SCHED_FIFO, 1);
	// pthread_join(thr, NULL);

	return 0;
}
