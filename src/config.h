#ifndef __config_h__
    #define __config_h__
#define FINITE_HORIZON 1
#define ITERATIONS 128
#define INFINITE_HORIZON 0
#define PREMIUM_MEAN_SERVICE_TIME 120 //60 sec; this value is used also as mean service time for reliable analysis multi server queue node
#define NORMAL_MEAN_SERVICE_TIME 150 //120 sec
#define RELIABLE_MEAN_SERVICE_TIME 75
#define DIGEST_MEAN_SERVICE_TIME 0.6 // 0.6 sec
#define MEAN_INTERARRIVAL_TIME 0.8640
#define OBSERVATION_PERIOD 86400 //86400 sec = 24 hr
#define INITIAL_DIGEST_MATCHING_PROB 0.02
#define FINAL_DIGEST_MATCHING_PROB 0.05
#define LINEAR_INCREASING_PROB_FACTOR 0.000003 // prob reaches final value after 10.000 jobs
#define N_NORMAL 50// number of servers in normal user multi server queue node
#define N_PREMIUM 95// number of servers in premium user multi server queue node
#define N_RELIABLE 1 // number of servers in realiable analysis multi server queue node
#define TIMEOUT 600  // 120 sec
#define TIMEOUT_RELIABLE 900// 240 sec 
#define PROBABILITY_PREMIUM 0.70
#define PROBABILITY_MALWARE 0.86
#endif
