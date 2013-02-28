#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mfs.h"

typedef struct {
	int flowNumber;
	float arrivalTime;
	float transmissionTime;
	int priority;
	pthread_t threadId;
} flow;

typedef flow * flowPointer;

int numberOfFlows;
int remainingFlows;

flow *flowQueue;

int main(int argc, char *argv[])
{
	// Start scheduler thread
	
	
	if(argc != 2)
	{
		fprintf(stderr, "Usage: MFS <input file>\n");
		return -1;
	}
	
	// Parse input file and put all flows into flowQueue
	getFlows(argv[1]);
	
	// TODO: Divide each arrival/transit time by 10 to get milliseconds
	
	// Create all the threads
	int i;
	for (i = 0; i < numberOfFlows; i ++)
	{
		pthread_create(&flowQueue[i].threadId, NULL, flowFunction, &flowQueue[i]);
		printf("Created Flow: Flow Number: %d Flow Arrival time: %2.2f Flow Transmission Time: %2.2f Flow Priority: %d Flow Thread ID: %u\n", flowQueue[i].flowNumber, flowQueue[i].arrivalTime, flowQueue[i].transmissionTime, flowQueue[i].priority, (int) flowQueue[i].threadId);
	}
	
	// Wait for all threads to finish
	for (i = 0; i < numberOfFlows; i ++)
	{
		pthread_join(flowQueue[i].threadId, NULL);
		printf("Stopped Flow Number: %d\n", flowQueue[i].flowNumber);
	}
	
	free(flowQueue);
	
	return 0; // Success!?
}

void getFlows(char *fileName)
{
	FILE *inputFilePointer;
	inputFilePointer = fopen(fileName, "r");
	
	if (inputFilePointer == NULL)
	{
		fprintf(stderr, "Can't open input file!\n");
		exit(1);
	}
	
	// Read the first line to find how many flows we have
	fscanf(inputFilePointer, "%d", &numberOfFlows);
	
	flowQueue = malloc(numberOfFlows * sizeof(flow));
	remainingFlows = 1;
	while (!feof(inputFilePointer)) {
		flow inputFlow;
		if (fscanf(inputFilePointer, "%d:%f,%f,%d", &inputFlow.flowNumber, &inputFlow.arrivalTime, &inputFlow.transmissionTime, &inputFlow.priority) != 4) break;
		
		// Put the times in seconds instead of milliseconds
		inputFlow.arrivalTime /= 10;
		inputFlow.transmissionTime /= 10;
		
		// Put all flows into flow structs and put them into the array/makeshift waiting queue.
		flowQueue[remainingFlows - 1] = inputFlow;
		remainingFlows ++;
	}
	
	// Don't need the file anymore
	fclose(inputFilePointer);
}

void *flowFunction(void *pointer)
{
	// Use gettimeofday to get current system times
	flowPointer flowInfo = (flowPointer) pointer;
	
	// Sleep until its arrival time.
	printf("Sleeping for %2.2f\n", flowInfo->arrivalTime);
	sleep(flowInfo->arrivalTime);
	
	// Add itself to the queue of flows waiting to transmit (mutex protected).
	// Waits for its turn to transmit (condvar, w/ mutex).
	// Transmits.
	// Decrement the number of remaining flows (mutex protected).
	// Signals the scheduler that another flow can transmit (condvar, w/ mutex).
	
	return (void *) 0;
}

void *schedulerFunction(void *pointer)
{
	// Loop until number of remaining flows == 0:
    // Waits to be signaled that another flow can transmit.
	// While the queue of flows waiting to transmit is empty:
	// {
			//Do nothing.
	//	}
	// Sort the queue of flows waiting to transmit (mutex’d).
	// Remove the head of the queue and signal flow to transmit
	
	return (void *) 0;
}