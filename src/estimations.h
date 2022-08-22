#ifndef __estimation_h__
    #define __estimation_h__
double *welford(double confidence, double *statistics, int size);
double autocorrelation(double *statistics, int size, int j);
#endif