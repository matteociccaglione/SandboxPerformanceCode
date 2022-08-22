#include "events_queue.h"

/**
 * @brief This function is used to generate an arrival to the entire system:
    the arrival is only received by the first center (digest calculation), since the
    arrivals to the other centers are some of the terminations of other centers.
 *
 * @return arrival* A pointer to an arrival event struct
 */
arrival *getArrival(double simulationTime)
{
    arrival *event = (arrival *) malloc(sizeof(arrival));
    SelectStream(MEAN_INTERARRIVAL_STREAM);
    double inter = Exponential(MEAN_INTERARRIVAL_TIME); // generate interarrival time

    event->time = inter + simulationTime; // time of the arrival = simulation time + interarrival time
    event->center = CENTER_DIGEST;        // the arrival can only be to the digest calculation center
    SelectStream(USER_PROBABILITY_STREAM);
    event->job.userType = Bernoulli(PROBABILITY_PREMIUM); // generate randomly if the job is from a Premium or Normal user
    SelectStream(MALWARE_PROBABILITY_STREAM);
    event->job.type = Bernoulli(PROBABILITY_MALWARE); // generate randomly if the job is a malware or not
    SelectStream(DIGEST_SERVICE_TIME_STREAM);
    event->job.serviceTime = Exponential(DIGEST_MEAN_SERVICE_TIME); // generate the service time for the digest calculation
    return event;
}