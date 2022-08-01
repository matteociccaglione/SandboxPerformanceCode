

void initializeServerArray(double *service_time,int *servers, int n){
    int i=0;
    for (i=0;i<n;i++){
        service_time[i]=0.0;
        servers[i]=0;
    }
}
