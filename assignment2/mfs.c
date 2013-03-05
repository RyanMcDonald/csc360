#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "mfs.h"

typedef struct {
	int flowNumber;
	float arrivalTime;
	float transmissionTime;
	int priority;
	pthread_t threadId;
	pthread_cond_t readyToTransmitCondVar;
} flow;

typedef flow * flowPointer;

struct timeval startTime;

int numberOfFlows;
int remainingFlows;

flowPointer *allFlows;
flowPointer *flowQueue;

pthread_cond_t nobodyTransmittingCondVar = PTHREAD_COND_INITIALIZER;

pthread_mutex_t remainingFlowsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flowQueueMutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char *argv[])
{
	int i;
	
	// Keep track of when the simulation starts
	gettimeofday(&startTime, NULL);
	
	if(argc != 2)
	{
		fprintf(stderr, "Usage: MFS <input file>\n");
		return -1;
	}

	// Parse input file, put all flows into allFlows, initialize remainingFlows
	getFlows(argv[1]);
	
	// Create the queue for the threads to wait in. Set the default queue values to an empty flow, i.e., flowNumber = 0.
	flowQueue = malloc(numberOfFlows * sizeof(flowPointer));
	flow emptyFlow;
	emptyFlow.flowNumber = 0;
	for (i = 0; i < numberOfFlows; i ++)
	{
		flowQueue[i] = &emptyFlow;
	}
	
	// Start scheduler thread
	pthread_t schedulerThreadId;
	pthread_create(&schedulerThreadId, NULL, schedulerFunction, NULL);
	
	// Create all the threads
	for (i = 0; i < numberOfFlows; i ++)
	{
		pthread_create(&(allFlows[i]->threadId), NULL, flowFunction, allFlows[i]);
	}
	
	// Tell the scheduler to transmit a flow
	pthread_cond_signal(&nobodyTransmittingCondVar);
	
	// Wait for all threads to finish
	for (i = 0; i < numberOfFlows; i ++)
	{
		pthread_join(allFlows[i]->threadId, NULL);
	}
	
	pthread_join(schedulerThreadId, NULL);
	
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
	
	allFlows = malloc(numberOfFlows * sizeof(flowPointer));
	remainingFlows = 0;
	while (!feof(inputFilePointer)) {
		flow inputFlow;
		
		//allFlows[remainingFlows - 1] = malloc(sizeof(flow));
		//flowPointer inputFlow = allFlows[remainingFlows - 1];
		
		if (fscanf(inputFilePointer, "%d:%f,%f,%d", &inputFlow.flowNumber, &inputFlow.arrivalTime, &inputFlow.transmissionTime, &inputFlow.priority) != 4) break;
		//if (fscanf(inputFilePointer, "%d:%f,%f,%d", &inputFlow->flowNumber, &inputFlow->arrivalTime, &inputFlow->transmissionTime, &inputFlow->priority) != 4) break;
		
		// Put the times in seconds
		inputFlow.arrivalTime /= 10;
		inputFlow.transmissionTime /= 10;
		//inputFlow->arrivalTime /= 10;
		//inputFlow->transmissionTime /= 10;
		
		allFlows[remainingFlows] = malloc(sizeof(flow));
		allFlows[remainingFlows]->flowNumber = inputFlow.flowNumber;
		allFlows[remainingFlows]->arrivalTime = inputFlow.arrivalTime;
		allFlows[remainingFlows]->transmissionTime = inputFlow.transmissionTime;
		allFlows[remainingFlows]->priority = inputFlow.priority;
		
		// Initialize its condition variable
		pthread_cond_init(&allFlows[remainingFlows]->readyToTransmitCondVar, NULL);
		
		remainingFlows ++;
	}
	
	// Don't need the file anymore
	fclose(inputFilePointer);
}

void *flowFunction(void *pointer)
{
	int i;
	
	flowPointer flowInfo = (flowPointer) pointer;
	
	// Sleep until its arrival time.
	usleep(flowInfo->arrivalTime * 1000000);
	printf("FLOW: Flow %d arrives: arrival time (%.2f), transmission time (%.1f), priority (%d) condvar address: %p.\n", flowInfo->flowNumber, getElapsedTime(), flowInfo->transmissionTime, flowInfo->priority, &flowInfo->readyToTransmitCondVar);
	
	// Add itself to the queue of flows waiting to transmit (mutex protected).
	pthread_mutex_lock(&flowQueueMutex);
	for (i = 0; i < numberOfFlows; i ++)
	{
		// Find the first empty spot in the queue; the end of the line. A flow with a flow number of 0 is considered empty.
		if (flowQueue[i]->flowNumber == 0)
		{
			flowQueue[i] = flowInfo;
			break;
		}
	}
	
	// Waits for its turn to transmit (condvar, w/ mutex).
	pthread_cond_wait(&flowInfo->readyToTransmitCondVar, &flowQueueMutex);

	// Transmit
	printf("FLOW: Flow %d starts its transmission at time %.2f.\n", flowInfo->flowNumber, getElapsedTime());
	usleep(flowInfo->transmissionTime * 1000000);
	printf("FLOW: Flow %d finishes its transmission at time %.2f.\n", flowInfo->flowNumber, getElapsedTime());
	
	// Signals the scheduler that another flow can transmit (condvar, w/ mutex).
	pthread_cond_signal(&nobodyTransmittingCondVar);
	pthread_mutex_unlock(&flowQueueMutex);
	
	return (void *) 0;
}

void *schedulerFunction(void *pointer)
{
	while (remainingFlows != 0)
	{	
		pthread_mutex_lock(&remainingFlowsMutex);
		
		// Waits to be signaled that another flow can transmit.
		pthread_cond_wait(&nobodyTransmittingCondVar, &remainingFlowsMutex);
		
		// While the queue of flows waiting to transmit is empty: Do nothing
		while(flowQueue[0]->flowNumber == 0);
		
		pthread_mutex_unlock(&remainingFlowsMutex);
		
		pthread_mutex_lock(&flowQueueMutex);
		
		// Sort the queue of flows waiting to transmit (mutex’d).
		sortQueue(flowQueue);
		
		// Remove the head of the queue and signal flow to transmit
		flowPointer flowToTransmit = flowQueue[0];
		
		flow emptyFlow;
		emptyFlow.flowNumber = 0;
		flowQueue[0] = &emptyFlow;
		
		// Move the rest of the queue down one spot
		int i;
		for (i = 1; i < numberOfFlows; i ++)
		{
			flowQueue[i - 1] = flowQueue[i];
		}
	
		pthread_cond_signal(&flowToTransmit->readyToTransmitCondVar);
		pthread_mutex_unlock(&flowQueueMutex);
		
		remainingFlows --;
	}
	
	return (void *) 0;
}

double getElapsedTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	double elapsedTime = (tv.tv_sec - startTime.tv_sec) * 1.0;
	elapsedTime += (tv.tv_usec - startTime.tv_usec) / 1000000.0; // Microseconds to seconds
	
	return elapsedTime;
}

/* Return values:
-1 if flowA > flowB
+1 if flowA < flowB */
int compareFlows(void *pointerA, void *pointerB)
{
	flowPointer flowA = (flowPointer) pointerA;
	flowPointer flowB = (flowPointer) pointerB;
	
	if (flowA->priority < flowB->priority) return -1;
	else if (flowA->priority > flowB->priority) return 1;
	else
		if (flowA->arrivalTime < flowB->arrivalTime) return -1;
		else if (flowA->arrivalTime > flowB->arrivalTime) return 1;
		else
			if (flowA->transmissionTime < flowB->transmissionTime) return -1;
			else if (flowA->transmissionTime > flowB->transmissionTime) return 1;
			else
				if (flowA->flowNumber < flowB->flowNumber) return -1;
				else return 1;	
}

void sortQueue(void *queue)
{
	flowPointer *flows = (flowPointer *) queue;
	int i;
	int highestFlowIndex = 0;
	flowPointer highestFlow = flows[0];
	flowPointer currentFlow = flows[0];
	
	// Find the flow that should run next, and put him in spot 0.
	for (i = 1; i < numberOfFlows; i ++)
	{
		currentFlow = flows[i];
		if (currentFlow->flowNumber == 0)
		{
			break;
		}
		
		if (compareFlows(highestFlow, currentFlow) == 1)
		{
			highestFlow = currentFlow;
			highestFlowIndex = i;
		}
	}
	
	// Swap the highest flow with the one in position 0
	flowPointer temp = flows[0];
	flows[0] = flows[highestFlowIndex];
	flows[highestFlowIndex] = temp;
}