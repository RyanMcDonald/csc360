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

flowPointer currentlyTransmittingFlow;

pthread_cond_t nobodyTransmittingCondVar = PTHREAD_COND_INITIALIZER;
pthread_cond_t somebodyTransmittingCondVar = PTHREAD_COND_INITIALIZER;

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
	
	// Initialize the currentlyTransmittingFlow to the emptyFlow because there are no flows tranmsitting yet.
	currentlyTransmittingFlow = &emptyFlow;
	
	// Start scheduler thread
	pthread_t schedulerThreadId;
	pthread_create(&schedulerThreadId, NULL, schedulerFunction, NULL);
	
	// Create all the threads
	for (i = 0; i < numberOfFlows; i ++)
	{
		pthread_create(&(allFlows[i]->threadId), NULL, flowFunction, allFlows[i]);
	}
	
	pthread_mutex_lock(&remainingFlowsMutex);
	
	// Tell the scheduler to begin
	printf("MAIN: Telling Scheduler to begin!\n");
	pthread_cond_signal(&nobodyTransmittingCondVar);
	pthread_mutex_unlock(&remainingFlowsMutex);
	
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
	if (fscanf(inputFilePointer, "%d", &numberOfFlows) != 1)
	{
		fprintf(stderr, "Couldn't get the total number of flows from input file! Is it formatted correctly?\n");
		exit(1);
	}
	
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
	//printf("FLOW: Flow %d Trying to gain control of flowQueueMutex!\n", flowInfo->flowNumber);
	pthread_mutex_lock(&flowQueueMutex);
	//printf("FLOW: Flow %d Got control of flowQueueMutex!\n", flowInfo->flowNumber);
	for (i = 0; i < numberOfFlows; i ++)
	{
		// Find the first empty spot in the queue; the end of the line. A flow with a flow number of 0 is considered empty.
		if (flowQueue[i]->flowNumber == 0)
		{
			flowQueue[i] = flowInfo;
			break;
		}
	}
	
	//pthread_mutex_unlock(&flowQueueMutex);
	
	// Check if there is already a flow transmitting
	//printf("FLOW: Flow %d Trying to gain control of flowQueueMutex!\n", flowInfo->flowNumber);
	//pthread_mutex_lock(&flowQueueMutex);
	//printf("FLOW: Flow %d Got control of flowQueueMutex!\n", flowInfo->flowNumber);
	while (currentlyTransmittingFlow->flowNumber != flowInfo->flowNumber)
	{
		if (currentlyTransmittingFlow->flowNumber != 0)
		{
			printf("FLOW: Flow %d waits for the finish of flow %d. \n", flowInfo->flowNumber, currentlyTransmittingFlow->flowNumber);
		}
		pthread_cond_wait(&somebodyTransmittingCondVar, &flowQueueMutex);
	}

	// Transmit
	printf("FLOW: Flow %d starts its transmission at time %.2f.\n", flowInfo->flowNumber, getElapsedTime());
	usleep(flowInfo->transmissionTime * 1000000);
	printf("FLOW: Flow %d finishes its transmission at time %.2f.\n", flowInfo->flowNumber, getElapsedTime());
	
	pthread_mutex_unlock(&flowQueueMutex);
	
	pthread_mutex_lock(&remainingFlowsMutex);
	// Signals the scheduler that another flow can transmit (condvar, w/ mutex).
	pthread_cond_signal(&nobodyTransmittingCondVar);
	pthread_mutex_unlock(&remainingFlowsMutex);
	
	return (void *) 0;
}

void *schedulerFunction(void *pointer)
{
	while (remainingFlows != 0)
	{	
		//printf("SCHEDULER: Trying to gain control of remainingFlowsMutex!\n");
		pthread_mutex_lock(&remainingFlowsMutex);
		//printf("SCHEDULER: Got control of remainingFlowsMutex!\n");
		
		//printf("SCHEDULER: Waiting for flows to finish transmitting!\n");
		// Waits to be signaled that another flow can transmit.
		pthread_cond_wait(&nobodyTransmittingCondVar, &remainingFlowsMutex);
		//printf("SCHEDULER: Flow finished transmitting!\n");
		
		pthread_mutex_unlock(&remainingFlowsMutex);
		
		// While the queue of flows waiting to transmit is empty: Do nothing
		while(flowQueue[0]->flowNumber == 0);
		
		pthread_mutex_lock(&flowQueueMutex);
		// Sort the queue of flows waiting to transmit (mutex�d).
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
		
		currentlyTransmittingFlow = flowToTransmit;
		
		//pthread_mutex_unlock(&flowQueueMutex);
		
		//pthread_mutex_lock(&remainingFlowsMutex);
		remainingFlows --;
		//printf("SCHEDULER: Signalling flow %d to begin!\n", currentlyTransmittingFlow->flowNumber);
		pthread_cond_broadcast(&somebodyTransmittingCondVar);
		pthread_mutex_unlock(&flowQueueMutex);
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