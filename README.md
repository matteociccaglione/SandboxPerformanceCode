# Sandbox Performance (source code)
This repository contains the source code realized by Andrea Pepe (m. 0315903) and Matteo Ciccaglione (m. 0315944) for the college project of PMCSN (Performance Modeling of Computer Systems and Networks). The project is about the modeling, the simulation and the analysis of a system for the sandbox-based automatic malware analysis. 

## Execution instruction
To execute the simulation of the _SandboxPerformance_ system, configure it modifying the file **src/config.h**.
To compile it, from the root directory of the project, execute the command:
'''
make
'''

To execute:
'''
./simulation
'''

The output will be printed on standard output and several CSV files will be populated with the results of the simulation.


## Code structure
The project is structured as follows:
    - **/lib**: this folder contains the C library files used (_rngs.c_, _rngs.h_, _rvgs.c_, _rvgs.h_, _rvms.c_, _rvms.h_)
    - **/scripts**: this folder contains two Python scripts used to compute analytical values of the Erlang-C formula and of a Markov queue,
    for the multi-server centers (with queue) and for the ML center (multi-server without queue). The computed values have been used in the verification phase
    - **/src**: this folder contains the C code written to implement the computational model and to execute the simulation.

### The /src directory
In the **/src** directory several files are present:
    - **config.h**: this file contains the configuration values; modify them to have different results from the simulation or to select the type of simulation to be run
    - **centers.h, centers.c**: these files contain definition of data structures used to mantain data and status for each center of the system during the simulation
    - **job.h, job.c**: these files contain the definition of the job structure and of the FIFO queues, including functions to operate with the queues
    - **events_queue.h, events_queue.c**: these files contain the definition of data structures used to represent the events of the next-event simulation; there is also the implementation of the function that generates the arrival of a new job to the system, using the RNG functions of the libraries present in the _/lib_ folder
    - **handle_events.h, handle_events.c**: these files contain the definition and the implementations of the functions used to handle the different types of event that can occur in the centers of the system
    - **verify.h, verify.c**: these files contain the definition and the implementation of a function used in software testing activity phase, to do the verification of the system
    - **stats.h, stats.c**: these files contain the definition of the data structure where all the results of a simulation run are mantained; there is also the implementation of the function used to compute the statistics of the simulation run, derived from the data collected for each center of the system
    - **estimations.h, estimations.c**: these files contain the definitions and the implementations of the two functions used to compute the confidence intervals for the computed statistics and the autocorrelation of a dataset
    - **main.c**: this is the starting point of the simulation; here there are the functions used to perform both finite horizon simulation and infinite horizon simulation. Then, confidence intervals are computed and printed on the standard output and written into CSV files 