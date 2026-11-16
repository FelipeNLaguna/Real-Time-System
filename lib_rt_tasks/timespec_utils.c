#include "rt_tasks.h"

int64_t timespec_to_ns(struct timespec timer){
    return ((int64_t)timer.tv_sec * BILLION) + (int64_t)timer.tv_nsec; 
}

void timespec_add_ns(struct timespec *timer, long period_nsec){
    
    timer->tv_nsec += period_nsec;
    while((timer->tv_nsec) >= BILLION){
        timer->tv_sec++;
        timer->tv_nsec -= BILLION;
    }
}