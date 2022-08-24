/**
 * @file estimations.h
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-08-23
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file declares two functions, used to calculated interval estimations starting from a dataset
 * and to compute autocorrelation of a dataset.
 */
#ifndef __estimation_h__
    #define __estimation_h__
double *welford(double confidence, double *statistics, int size);
double autocorrelation(double *statistics, int size, int j);
#endif