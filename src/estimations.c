/**
 * @file estimations.c
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-08-23
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file contains the implementation of the Welford's Algorithm used to calculate interval estimation
 * and of a function to compute autocorrelation of a dataset.
 */

#include "estimations.h"
#include "../lib/rvms.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


/**
 * @brief Welford's Algorithm to compute confidence intervals of a dataset.
 * 95% confidence level is recommended.
 * @param confidence Confidence level, a value between 0.0 and 1.0.
 * @param statistics Pointer to an array of doubles, containing the data points of the dataset
 * @param size Size of the array
 * @return double* Returns a pointer to an array of 2 doubles; the first one is the mean of the dataset, the second one is the semi-width of the confidence interval
 */
double *welford(double confidence, double *statistics, int size)
{
    long n = 0;                             /* counts data points */
    double sum = 0.0;
    double mean = 0.0;
    double data;
    double stdev;
    double u, t, w;
    double diff;

    for (int i = 0; i < size; i++)
    {                                       /* use Welford's one-pass method */
        data = statistics[i];               /* to calculate the sample mean  */
        n++;                                /* and standard deviation        */
        diff = data - mean;
        sum += diff * diff * (n - 1.0) / n;
        mean += diff / n;
    }
    stdev = sqrt(sum / n);

    if (n > 1)
    {
        u = 1.0 - 0.5 * (1.0 - confidence); /* interval parameter  */
        t = idfStudent(n - 1, u);           /* critical value of t */
        w = t * stdev / sqrt(n - 1);        /* interval half width */
        double *array = malloc(sizeof(double) * 2);
        array[0] = round(1000000 * mean) / 1000000.0;
        array[1] = round(1000000 * w) / 1000000.0;
        return array;
    }
    else
        printf("ERROR - insufficient data\n");
    return (0);
}

/**
 * @brief This function calculate the autocorrelation with lag 1
 * over a dataset of data points.
 * 
 * @param statistics array of data points
 * @param size number of datapoints
 * @param j autocorrelation lag (set 1)
 * @return double autocorrelation value (C_j/C_0)
 */
double autocorrelation(double *statistics, int size, int j)
{
    int i = 0;
    double sum = 0.0;
    double n;
    int p = 0;
    double *cosum = malloc(sizeof(double) * (j + 1));
    double *hold = malloc(sizeof(double) * (j + 1));
    double mean = 0.0;
    while (i < j + 1)
    {                                       /* initialize the hold array with */
        double x = statistics[i];           /* the first K + 1 data values    */
        sum += x;
        hold[i] = x;
        i++;
    }

    while (i < size)
    {
        for (int k = 0; k < j + 1; k++)
            cosum[k] += hold[p] * hold[(p + k) % (j + 1)];
        double x = statistics[i];
        sum += x;
        hold[p] = x;
        p = (p + 1) % (j + 1);
        i++;
    }
    n = i;

    while (i < n + j + 1)
    { /* empty the circular array */
        for (int k = 0; k < j + 1; k++){
            cosum[k] += hold[p] * hold[(p + k) % (j + 1)];
        }
        hold[p] = 0.0;
        p = (p + 1) % (j + 1);
        i++;
    }

    mean = sum / n;
    for (int k = 0; k <= j; k++)
        cosum[k] = (cosum[k] / (n - k)) - (mean * mean);

    // return C_j / C_0
    return cosum[j] / cosum[0];
}