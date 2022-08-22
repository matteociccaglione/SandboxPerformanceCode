#ifndef __config_h__
    #define __config_h__
#define FINITE_HORIZON 1
#define INFINITE_HORIZON 0
#define ITERATIONS 128
#define PREMIUM_MEAN_SERVICE_TIME 120 //60 sec; this value is used also as mean service time for reliable analysis multi server queue node
#define NORMAL_MEAN_SERVICE_TIME 150 //120 sec
#define RELIABLE_MEAN_SERVICE_TIME 300 //75 no imp; 300 imp
#define DIGEST_MEAN_SERVICE_TIME 0.6 // 0.6 sec
#define MEAN_INTERARRIVAL_TIME 0.617142857 //0.8640 in normal situation; 0.617142857
#define OBSERVATION_PERIOD 86400 //86400 sec = 24 hr
#define INITIAL_DIGEST_MATCHING_PROB 0.02
#define FINAL_DIGEST_MATCHING_PROB 0.05
#define LINEAR_INCREASING_PROB_FACTOR 0.000003 // prob reaches final value after 10.000 jobs
#define N_NORMAL 50 // number of servers in normal user multi server queue node
#define N_PREMIUM 95 // number of servers in premium user multi server queue node
#define N_RELIABLE 45 // number of servers in realiable analysis multi server queue node; 1 no imp; 45 imp
#define TIMEOUT 300  // 120 sec //600 no imp    //300 imp
#define TIMEOUT_RELIABLE 1080 // 240 sec //900 no imp    //1080 imp
#define PROBABILITY_PREMIUM 0.70
#define PROBABILITY_MALWARE 0.86
#define N_ML 20 //Only for improvement model
#define ML_MEAN_SERVICE_TIME 30
#define PROB_POSITIVE_ML 0.7
#define IMPROVEMENT 1
#endif
