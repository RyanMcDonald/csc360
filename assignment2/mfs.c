#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int id;
	int arrivalTime;
	int transmissionTime;
	int priority;
} flow;

typedef flow * flowPointer;

int numberOfFlows;
int remainingFlows;

flow *flowQueue;

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Usage: MFS <input file>\n");
		return -1;
	}
	
	// Parse file. Use atoi to convert chars to ints
	FILE *inputFilePointer;
	inputFilePointer = fopen(argv[1], "r");
	
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
		if (fscanf(inputFilePointer, "%d:%d,%d,%d", &inputFlow.id, &inputFlow.arrivalTime, &inputFlow.transmissionTime, &inputFlow.priority) != 4) break;
		
		// Put all flows into flow structs and put them into the array/makeshift waiting queue.
		flowQueue[remainingFlows - 1] = inputFlow;
		remainingFlows ++;
	}
	
	// Don't need the file anymore
	fclose(inputFilePointer);
	
	// TODO: Divide each arrival/transit time by 10 to get milliseconds
	
	// for each flow: pthread_create(threadId, NULL, print_function);
	int i;
	for (i = 0; i < numberOfFlows; i ++)
	{
		flow currentFlow = flowQueue[i];
		printf("Flow id: %d Flow Arrival time: %d Flow transmission time: %d Flow priority: %d\n", currentFlow.id, currentFlow.arrivalTime, currentFlow.transmissionTime, currentFlow.priority);
	}
	
	// Use gettimeofday to get current system times
	
	
	
	return 0; // Success!?
}