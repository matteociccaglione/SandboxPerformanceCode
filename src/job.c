typedef enum __userType{
    PREMIUM = 0,
    NORMAL = 1
}UserType;

typedef enum __jobType{
    MALWARE = 0,
    BENIGN = 1
}JobType;

typedef struct __job{
    UserType userType; // Job submitted by a premium user or a normal one
    JobType type; // Job type: malware, benign
    double serviceTime; // Service time 
}job;