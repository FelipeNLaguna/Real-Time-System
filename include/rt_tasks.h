#ifndef RT_TASKS_H
#define RT_TASKS_H

#include <time.h>
#include <stdint.h>
#include <semaphore.h>
#include "buffer.h"

#define DT 0.03
#define DURATION_NS 20000000000LL
#define BILLION 1000000000
#define T_SIM_NS 30000000
#define T_SAMPLING_NS 50000000
#define T_GEN_U_NS 50000000
#define CHANGE_U_NS 10000000000
#define PRIO_SIM 90
#define PRIO_SAMPLING 70
#define PRIO_GEN_U 80
#define TASK_SIM 's'
#define TASK_SAMPLING 'y'
#define TASK_GEN_U 'u'

struct data_tasks{
    struct timespec timer_init;
    struct log_record log_thread;
    Buffer buf;
    sem_t sem_variable_y;
    sem_t sem_variable_u;
    sem_t sem_buffer;
    sem_t can_send; // exist at least one empty slot
    sem_t can_save; // exist at least one item in the buffer
    sem_t sem_end;
    double u_rt[2];
    double y_rt[3];
    const char *file_name;
    int num_threads_finished;
};

extern void *simulator(void *data_tasks);
extern void *gen_u(void *data_tasks);
extern void *sampling(void *data_tasks);
extern void *logger(void *data_tasks); 
extern void *display(void *data_tasks); 

extern int64_t timespec_to_ns(struct timespec timer);
extern void timespec_add_ns(struct timespec *timer, long period_nsec);

#endif