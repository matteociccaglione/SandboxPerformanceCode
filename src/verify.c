/**
 * @file verify.c
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-08-25
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file contains the implementation of the function used to perform verification of a simulation run.
 */
#include "verify.h"
#include "math.h"

/**
 * @brief This function is used to do the VERIFICATION of the system, by carrying out consistency checks between the measured values.
 * This function must be followed after each finite horizon simulation run.
 * If a consistency check does not pass, the entire simulation will be interrupted and the verification error will be shown.
 * 
 * The consistency checks are:
 *          - Check that response time = wait time (queue) + service time, for each center
 *          - Check that routing probabilities respect the configured values
 *          - Check that the digest matching probability reaches the expected final value (configured in configuration file)
 *          - Check that percentage of jobs in timeout in normal, premium and reliable analysis centers match the probability that the exponential random variable of the service time is greater of the timeout
 *          - Check that jobs in input = jobs in timeout + jobs analyzed with success, for each center
 *          - For each center, check the validity of the Little's Law: E(N) = lambda * E(Ts)
 * 
 * In the improved model, the following consistency check are added:
 *          - Check that the Little's Law is valid also for the ML center
 *          - Check that the utilization of the ML center is equal to the average percentage of busy servers (average number of jobs in the center / number of servers)
 * 
 * @param digestCenter Pointer to digestCenter struct, containing the data of the simulation
 * @param normalCenter Pointer to normalAnalysisCenter struct, containing the data of the simulation
 * @param premiumCenter Pointer to premiumAnalysisCenter struct, containing the data of the simulation
 * @param reliableCenter Pointer to reliableAnalysisCenter struct, containing the data of the simulation
 * @param mlCenter Pointer to machineLearningCenter struct, containing the data of the simulation
 */
void verify(digestCenter *digestCenter, normalAnalysisCenter *normalCenter, premiumAnalysisCenter *premiumCenter, reliableAnalysisCenter *reliableCenter, machineLearningCenter *mlCenter)
{

    // Check that E(Ts) = E(Tq) + E(Si)
    // digest center
    double responseTime = digestCenter->area / digestCenter->index;
    double waitTime = digestCenter->queueArea / digestCenter->index;
    double serviceTime = digestCenter->serviceArea / digestCenter->index;
    if (!((round(1000000 * responseTime) / 1000000) == (round(1000000 * (waitTime + serviceTime)) / 1000000)))
    {
        printf("Verify for digest calculation center: \n");
        printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }

    // normal analysis center
    responseTime = normalCenter->area / normalCenter->index;
    waitTime = normalCenter->queueArea / normalCenter->index;
    serviceTime = normalCenter->serviceArea / normalCenter->index;
    if (!((round(1000000 * responseTime) / 1000000) == (round(1000000 * (waitTime + serviceTime)) / 1000000)))
    {
        printf("Verify for normal center: \n");
        printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }

    // premium analysis center
    responseTime = premiumCenter->area / premiumCenter->index;
    waitTime = premiumCenter->queueArea / premiumCenter->index;
    serviceTime = premiumCenter->serviceArea / premiumCenter->index;
    if (!((round(1000000 * responseTime) / 1000000) == (round(1000000 * (waitTime + serviceTime)) / 1000000)))
    {
        printf("Verify for premium center: \n");
        printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }

    // reliable analysis center
    responseTime = reliableCenter->area / reliableCenter->index;
    waitTime = reliableCenter->queueArea / reliableCenter->index;
    serviceTime = reliableCenter->serviceArea / reliableCenter->index;
    if (!((round(1000000 * responseTime) / 1000000) == (round(1000000 * (waitTime + serviceTime)) / 1000000)))
    {
        printf("Verify for reliable center: \n");
        printf("E(Ts) = E(Tq) + E(s) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }

    // high priority queue check
    responseTime = reliableCenter->areaPremium / reliableCenter->premiumIndex;
    waitTime = reliableCenter->queueAreaPremium / reliableCenter->premiumIndex;
    serviceTime = reliableCenter->serviceAreaPremium / reliableCenter->premiumIndex;
    if (!((round(1000000 * responseTime) / 1000000) == (round(1000000 * (waitTime + serviceTime)) / 1000000)))
    {
        printf("Verify for reliable center (Premium class): \n");
        printf("E(Ts1) = E(Tq1) + E(s1) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }

    // low priority queue check
    responseTime = reliableCenter->areaNormal / reliableCenter->normalIndex;
    waitTime = reliableCenter->queueAreaNormal / reliableCenter->normalIndex;
    serviceTime = reliableCenter->serviceAreaNormal / reliableCenter->normalIndex;

    if (!((round(1000000 * responseTime) / 1000000) == (round(1000000 * (waitTime + serviceTime)) / 1000000)))
    {
        printf("Verify for reliable center (Normal class): \n");
        printf("E(Ts2) = E(Tq2) + E(s2) -> %6.6f + %6.6f = %6.6f\n", waitTime, serviceTime, responseTime);
        printf("Condition is satisfied: %d\n", round(1000000 * responseTime) / 1000000 == round(1000000 * (waitTime + serviceTime)) / 1000000);
        exit(EXIT_FAILURE);
    }


    // Verify routing probabilities
    double probabilityPremium = round(100 * (double)digestCenter->indexPremium / digestCenter->index) / 100;
    double probabilityNormal = round(100 * ((double)digestCenter->index - digestCenter->indexPremium) / digestCenter->index) / 100;

    if (!(probabilityPremium == PROBABILITY_PREMIUM))
    {
        printf("Verify that P(Job is submitted by a Premium user) = %6.2f\n", PROBABILITY_PREMIUM);
        printf("Probability computed = %6.2f\n", probabilityPremium);
        printf("Condition is satisfied: %d\n", probabilityPremium == PROBABILITY_PREMIUM);
        exit(EXIT_FAILURE);
    }
    if (!((probabilityNormal == round(100 * (1 - PROBABILITY_PREMIUM)) / 100)))
    {
        printf("Verify that P(Job is submitted by a Normal user) = %6.2f\n", 1 - PROBABILITY_PREMIUM);
        printf("Probability computed = %6.2f\n", probabilityNormal);
        printf("Condition is satisfied: %d\n", probabilityNormal == round(100 * (1 - PROBABILITY_PREMIUM)) / 100);
        exit(EXIT_FAILURE);
    }

    // Check digest matching probability
    double probabilityDigestMatching = round(100 * (double)digestCenter->digestMatching / digestCenter->index) / 100;
    if (!(probabilityDigestMatching == FINAL_DIGEST_MATCHING_PROB))
    {
        printf("Verify that P(Job matches digest) = %6.2f\n", FINAL_DIGEST_MATCHING_PROB);
        printf("Probability computed = %6.2f\n", probabilityDigestMatching);
        printf("Condition is satisfied: %d\n", probabilityDigestMatching == FINAL_DIGEST_MATCHING_PROB);
        exit(EXIT_FAILURE);
    }

    // Check probability of timeouts
    double probabilityOfTimeoutPremium = round(100 * ((double)premiumCenter->numberOfTimeouts / premiumCenter->index)) / 100;
    double probabilityOfTimeoutNormal = round(100 * ((double)normalCenter->numberOfTimeouts / normalCenter->index)) / 100;
    double probabilityOfTimeoutReliable = round(100 * ((double)reliableCenter->numberOfTimeouts / reliableCenter->index)) / 100;

    double expectedTimeoutPremium = round(100 * exp(-(double)TIMEOUT / PREMIUM_MEAN_SERVICE_TIME)) / 100;
    double expectedTimeoutNormal = round(100 * exp(-(double)TIMEOUT / NORMAL_MEAN_SERVICE_TIME)) / 100;
    double expectedTimeoutReliable = round(100 * exp(-(double)TIMEOUT_RELIABLE / RELIABLE_MEAN_SERVICE_TIME)) / 100;
    if (((probabilityOfTimeoutPremium == round(100 * (expectedTimeoutPremium + 0.01)) / 100) || (probabilityOfTimeoutPremium == round(100 * (expectedTimeoutPremium - 0.01)) / 100) || (probabilityOfTimeoutPremium == expectedTimeoutPremium)) == 0)
    {
        printf("Verify that P(Job is timed out| Job is in the premium center) = %6.2f\n", expectedTimeoutPremium);
        printf("Probability computed = %6.2f\n", probabilityOfTimeoutPremium);
        printf("Condition is satisfied: %d\n", ((probabilityOfTimeoutPremium == round(100 * (expectedTimeoutPremium + 0.01)) / 100) || (probabilityOfTimeoutPremium == round(100 * (expectedTimeoutPremium - 0.01)) / 100) || (probabilityOfTimeoutPremium == expectedTimeoutPremium)));
        exit(EXIT_FAILURE);
    }
    if (((probabilityOfTimeoutNormal == round(100 * (expectedTimeoutNormal + 0.01)) / 100) || (probabilityOfTimeoutNormal == round(100 * (expectedTimeoutNormal - 0.01)) / 100) || (probabilityOfTimeoutNormal == expectedTimeoutNormal)) == 0)
    {
        printf("Verify that P(Job is timed out| Job is in the normal center) = %6.2f\n", expectedTimeoutNormal);
        printf("Probability computed = %6.2f\n", probabilityOfTimeoutNormal);
        printf("Condition is satisfied: %d\n", (probabilityOfTimeoutNormal == expectedTimeoutNormal + 0.01) || (probabilityOfTimeoutNormal == expectedTimeoutNormal - 0.01) || (probabilityOfTimeoutNormal == expectedTimeoutNormal));
        exit(EXIT_FAILURE);
    }
    if (((probabilityOfTimeoutReliable == round(100 * (expectedTimeoutReliable + 0.01)) / 100) || (probabilityOfTimeoutReliable == round(100 * (expectedTimeoutReliable - 0.01)) / 100) || (probabilityOfTimeoutReliable == expectedTimeoutReliable)) == 0)
    {
        printf("Verify that P(Job is timed out| Job is in the reliable center) = %6.2f\n", expectedTimeoutReliable);
        printf("Probability computed = %6.2f\n", probabilityOfTimeoutReliable);
        printf("Condition is satisfied: %d\n", (probabilityOfTimeoutReliable == expectedTimeoutReliable + 0.01) || (probabilityOfTimeoutReliable == expectedTimeoutReliable - 0.01) || (probabilityOfTimeoutReliable == expectedTimeoutReliable));
        exit(EXIT_FAILURE);
    }


    // Verify that number of jobs in input is equals to number of jobs analyzed + number of jobs timed out
    int numberOfInput = digestCenter->index;
    int numberOfProcessed = premiumCenter->index - premiumCenter->numberOfTimeouts + normalCenter->index - normalCenter->numberOfTimeouts + digestCenter->digestMatching + reliableCenter->jobAnalyzed;
    if (!IMPROVEMENT)
    {
        if (!(numberOfProcessed + reliableCenter->numberOfTimeouts == numberOfInput))
        {
            printf("Verify that number of jobs in input is equals to number of jobs analyzed + number of jobs timed out\n");
            printf("#job in input = #processed+#timedout -> %d = %d+%d\n", numberOfInput, numberOfProcessed, reliableCenter->numberOfTimeouts);
            printf("Condition is satisfied: %d\n", numberOfProcessed + reliableCenter->numberOfTimeouts == numberOfInput);
            exit(EXIT_FAILURE);
        }
    }

    // Check validity of Little's Law
    // digest center
    double lambda = round(1000000 * (1 / digestCenter->interarrivalTime * digestCenter->index)) / 1000000;
    serviceTime = round(1000000 * (digestCenter->area / digestCenter->index)) / 1000000;
    double eN = round(1000000 * (double)digestCenter->area / digestCenter->interarrivalTime) / 1000000;
    double experimental = round(1000000 * (lambda * serviceTime) / 1000000);
    int condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
    if (!condition)
    {
        printf("Verify that little is valid for digest center\n");
        printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
        printf("Condition is not satisfied\n");
        exit(EXIT_FAILURE);
    }
    
    // normal analysis center
    lambda = round(1000000 * (1 / normalCenter->interarrivalTime * normalCenter->index)) / 1000000;
    serviceTime = round(1000000 * (normalCenter->area / normalCenter->index)) / 1000000;
    eN = round(1000000 * (double)normalCenter->area / normalCenter->interarrivalTime) / 1000000;
    experimental = round(1000000 * (lambda * serviceTime) / 1000000);
    condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
    if (!condition)
    {
        printf("Verify that little is valid for normal center\n");
        printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
        printf("Condition is not satisfied\n");
        exit(EXIT_FAILURE);
    }

    // premium analysis center
    lambda = round(1000000 * (1 / premiumCenter->interarrivalTime * premiumCenter->index)) / 1000000;
    serviceTime = round(1000000 * (premiumCenter->area / premiumCenter->index)) / 1000000;
    eN = round(1000000 * (double)premiumCenter->area / premiumCenter->interarrivalTime) / 1000000;
    experimental = round(1000000 * (lambda * serviceTime) / 1000000);
    condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
    if (!condition)
    {
        printf("Verify that little is valid for premium center\n");
        printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
        printf("Condition is not satisfied\n");
        exit(EXIT_FAILURE);
    }

    // reliable analysis center
    lambda = round(1000000 * (1 / reliableCenter->interarrivalTime * reliableCenter->index)) / 1000000;
    serviceTime = round(1000000 * (reliableCenter->area / reliableCenter->index)) / 1000000;
    eN = round(1000000 * (double)reliableCenter->area / reliableCenter->interarrivalTime) / 1000000;
    experimental = round(1000000 * (lambda * serviceTime) / 1000000);
    condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
    if (!condition)
    {
        printf("Verify that little is valid for reliable center\n");
        printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
        printf("Condition is not satisfied\n");
        exit(EXIT_FAILURE);
    }
    if (IMPROVEMENT)
    {
        // ML center
        lambda = round(1000000 * (1 / mlCenter->interarrivalTime * mlCenter->index)) / 1000000;
        serviceTime = round(1000000 * (mlCenter->area / mlCenter->index)) / 1000000;
        eN = round(1000000 * (double)mlCenter->area / mlCenter->interarrivalTime) / 1000000;
        experimental = round(1000000 * (lambda * serviceTime) / 1000000);
        condition = eN != experimental || (eN != experimental + 0.01) || (eN != experimental - 0.01);
        if (!condition)
        {
            printf("Verify that little is valid for machine learning center\n");
            printf("E(N) = lambda*E(Ts) : %6.6f = %6.6f * %6.6f = %6.6f\n", eN, lambda, serviceTime, round(1000000 * (lambda * serviceTime)) / 1000000);
            printf("Condition is not satisfied\n");
            exit(EXIT_FAILURE);
        }

        // verify that utilization of no queue multi-server center is equal to average number of busy servers / total number of servers
        double rhoMl = (mlCenter->serviceArea/mlCenter->index) / (N_ML * mlCenter->interarrivalTime / mlCenter->index);     // rho = lambda * E(S)
        rhoMl = round (10000 * rhoMl) / 10000;
        experimental = round(10000 * (eN/N_ML))/10000;
        condition = (rhoMl == experimental) || (rhoMl == experimental + 0.0001) || (rhoMl == experimental - 0.0001);
        if (! condition){
            printf("Verify that rho is valid for machine learning center\n");
            printf("rho = E(N)/N_ML : %6.4f = %6.4f / %d = %6.4f\n", rhoMl, eN, N_ML, round(10000 * (eN/N_ML))/10000);
            printf("Condition is not satisfied\n");
            exit(EXIT_FAILURE);
        }        
    }
}
