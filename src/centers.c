/**
 * @file centers.c
 * @author A. Pepe - M. Ciccaglione
 * @version 1.0
 * @date 2022-08-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */

/**
 * @brief This function initializes to 0 (not busy) the array of servers of a multi-server center. 
 * 
 * @param servers Pointer to the array of the server status
 * @param n Size of the array
 */
void initializeServerArray(int *servers, int n){
    int i=0;
    for (i=0;i<n;i++){
        servers[i]=0;
    }
}
