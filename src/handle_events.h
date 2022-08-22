#ifndef __handle_events_c__
    #define __handle_events_c__

#include "centers.h"
#include "events_queue.h"
typedef struct __event_list
{
    arrival *arrivals;
    termination *terminations;
} event_list;

// Definition of functions to handle the events to the four centers of the system
double handleDigestArrival(digestCenter *digestCenter, event_list *ev, double simulationTime);
double handleNormalArrival(normalAnalysisCenter *center, event_list *ev, double simulationTime);
double handlePremiumArrival(premiumAnalysisCenter *center, event_list *ev, double simulationTime);
double handleReliableArrival(reliableAnalysisCenter *center, event_list *ev, double simulationTime);
double handleMachineLearningArrival(machineLearningCenter *mlCenter, event_list *ev, double simulationTime);
double handleDigestTermination(digestCenter *digestCenter, event_list *ev, double simulationTime);
double handleNormalTermination(normalAnalysisCenter *center, event_list *ev, digestCenter *digestCenter, double simulationTime);
double handlePremiumTermination(premiumAnalysisCenter *center, event_list *ev, digestCenter *digestCenter, double simulationTime);
double handleReliableTermination(reliableAnalysisCenter *center, event_list *ev, digestCenter *digestCenter, double simulationTime);
double handleMachineLearningTermination(machineLearningCenter *mlCenter, event_list *ev, double simulationTime);

void insertList(event_list *ev, void *node, int type);
double min(double val1, double val2);
int findFreeServer(int *servers, int n);
int nextEvent(event_list ev);
#endif