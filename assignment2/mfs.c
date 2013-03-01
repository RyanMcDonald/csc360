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

flow *allFlows;
flow *flowQueue;

pthread_mutex_t flowQueueMutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
	int i;
	
	// Start scheduler thread
	
	
	if(argc != 2)
	{
		fprintf(stderr, "Usage: MFS <input file>\n");
		return -1;
	}
	
	// Parse input file and put all flows into allFlows
	getFlows(argv[1]);
	
	// Create the queue for the threads to wait in. Set the default queue values to an empty flow, i.e. flow number = 0.
	flowQueue = malloc(numberOfFlows * sizeof(flow));
	flow emptyFlow;
	emptyFlow.flowNumber = 0;
	for (i = 0; i < numberOfFlows; i ++)
	{
		flowQueue[i] = emptyFlow;
	}
	
	// Create all the threads
	for (i = 0; i < numberOfFlows; i ++)
	{
		pthread_create(&allFlows[i].threadId, NULL, flowFunction, &allFlows[i]);
		//printf("Created Flow: Flow Number: %d Flow Arrival time: %2.2f Flow Transmission Time: %2.2f Flow Priority: %d Flow Thread ID: %u\n", allFlows[i].flowNumber, allFlows[i].arrivalTime, allFlows[i].transmissionTime, allFlows[i].priority, (int) allFlows[i].threadId);
	}
	
	// Wait for all threads to finish
	for (i = 0; i < numberOfFlows; i ++)
	{
		pthread_join(allFlows[i].threadId, NULL);
		//printf("Stopped Flow Number: %d\n", allFlows[i].flowNumber);
	}
	
	free(allFlows);
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
	
	allFlows = malloc(numberOfFlows * sizeof(flow));
	remainingFlows = 1;
	while (!feof(inputFilePointer)) {
		flow inputFlow;
		if (fscanf(inputFilePointer, "%d:%f,%f,%d", &inputFlow.flowNumber, &inputFlow.arrivalTime, &inputFlow.transmissionTime, &inputFlow.priority) != 4) break;
		
		// Put the times in seconds instead of milliseconds
		inputFlow.arrivalTime /= 10;
		inputFlow.transmissionTime /= 10;
		
		// Put all flows into flow structs and put them into the array/makeshift waiting queue.
		allFlows[remainingFlows - 1] = inputFlow;
		remainingFlows ++;
	}
	
	// Don't need the file anymore
	fclose(inputFilePointer);
}

void *flowFunction(void *pointer)
{
	int i;
	
	// Use gettimeofday to get current system times
	flowPointer flowInfo = (flowPointer) pointer;
	
	// Sleep until its arrival time.
	sleep(flowInfo->arrivalTime);
	printf("Flow %d arrived at: %2.2f\n", flowInfo->flowNumber, flowInfo->arrivalTime);
	
	// Add itself to the queue of flows waiting to transmit (mutex protected).
	//pthread_mutex_lock(&flowQueueMutex);
	for (i = 0; i < numberOfFlows; i ++)
	{
		// Find the first empty spot in the queue; the end of the line. A flow with a flow number of 0 is considered empty.
		if (flowQueue[i].flowNumber == 0)
		{
			flowQueue[i] = *flowInfo;
			printf("AFTER INSERT: Flow %d took queue position %d arrival time %2.2f transmission time %2.2f priority %d\n", flowQueue[i].flowNumber, i, flowQueue[i].arrivalTime, flowQueue[i].transmissionTime, flowQueue[i].priority);
			break;
		}
	}
	//pthread_mutex_unlock(&flowQueueMutex);
	
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