/**
 * @file verify.h
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-08-25
 * 
 * @copyright Copyright (c) 2022
 * 
 * @brief This file contains the declaration of the function used to perform verification through software testing.
 */
#ifndef __verify_h__
    #define __verify_h__

#include "centers.h"
void verify(digestCenter *digestCenter, normalAnalysisCenter *normalCenter, premiumAnalysisCenter *premiumCenter, reliableAnalysisCenter *reliableCenter, machineLearningCenter *mlCenter);
#endif