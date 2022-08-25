#ifndef __config_h__
    #define __config_h__
#define FINITE_HORIZON 1                        // set this value to 1 for a finite horizon simulation; 0 otherwise
#define INFINITE_HORIZON 0                      // set this value to 1 for an infinite horizon simulation; 0 otherwise
#define ITERATIONS 128                          // number of iterations for a finite horizon simulation
#define PREMIUM_MEAN_SERVICE_TIME 120           // E(Si) for Premium Analysis Center
#define NORMAL_MEAN_SERVICE_TIME 150            // E(Si) for Normal Analysis Center
#define RELIABLE_MEAN_SERVICE_TIME 75           // E(Si) for Reliable Analysis Center; 75 sec original system; 300 sec improved system
#define DIGEST_MEAN_SERVICE_TIME 0.6            // E(S) for Digest Center
#define MEAN_INTERARRIVAL_TIME 0.8640           // 0.8640 in original system (100K jobs per day); set to 0.617142857 for 140K jobs per day
#define OBSERVATION_PERIOD 86400                // 24 h = 86400 sec
#define INITIAL_DIGEST_MATCHING_PROB 0.02       // initial probability to have a matching digest in Digest Center
#define FINAL_DIGEST_MATCHING_PROB 0.05         // final value of the probability of digst matching; reached this value, the probability will not increase anymore
#define LINEAR_INCREASING_PROB_FACTOR 0.000003  // increasing factor of the probability of digest matching at each jobs that exits the system successfully
#define N_NORMAL 50                             // number of servers of the Normal Analysis multi-server center
#define N_PREMIUM 95                            // number of servers of the Premium Analysis multi-server center
#define N_RELIABLE 1                            // number of servers of the Reliable Analysis center; 1 (SSQ) in the original system; 45 in the improved system (optimal value)
#define TIMEOUT 600                             // timeot of Normal and Premium Analysis centers; optimal values: 600 sec in the original system, 300 sec in the improved system
#define TIMEOUT_RELIABLE 900                    // timeout of Reliable Analysis center; optimal values: 900 sec in the original system, 1080 sec in the improved system
#define PROBABILITY_PREMIUM 0.70                // probability that a job is submitted by a premium user
#define N_ML 20                                 // number of servers of the ML multi-server center; suggested values in different situations: 0, 10, 20
#define ML_MEAN_SERVICE_TIME 30                 // E(Si) for ML Center
#define PROB_POSITIVE_ML 0.7                    // probability that the ML analysis predicts malware (positive result) over non malware (negative result)
#define IMPROVEMENT 0                           // set this value to 1 to run the simulation of the improved system; 0 to run it for the original system
#endif
