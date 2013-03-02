#ifndef MFS_H_INCLUDED
#define MFS_H_INCLUDED

void getFlows(char *fileName);
void *flowFunction(void *pointer);
void *schedulerFunction(void *pointer);
double getElapsedTime();
int compareFlows(void *flowA, void *flowB);

#endif