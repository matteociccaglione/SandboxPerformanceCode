#define PREMIUM_MEAN_SERVICE_TIME 60 //60 sec; this value is used also as mean service time for reliable analysis multi server queue node
#define NORMAL_MEAN_SERVICE_TIME 120 //120 sec
#define DIGEST_MEAN_SERVICE_TIME 0.6 // 0.6 sec
#define OBSERVATION_PERIOD 86400 //86400 sec = 24 hr
#define INITIAL_DIGEST_MATCHING_PROB 0.02
#define FINAL_DIGEST_MATCHING_PROB 0.05
#define LINEAR_INCREASING_PROB_FACTOR 0.000003 // prob reaches final value after 10.000 jobs
#define N_NORMAL 84 // number of servers in normal user multi server queue node
#define N_PREMIUM 60 // number of servers in premium user multi server queue node
#define N_RELIABLE 12 // number of servers in realiable analysis multi server queue node
#define TIMEOUT 180 // 180 sec
#define TIMEOUT_RELIABLE 360 // 360 sec
