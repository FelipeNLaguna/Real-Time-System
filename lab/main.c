#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>
#include "rt_tasks.h"

int create_threads(pthread_t *thread, void *(*start_routine)(void *), void *arg, int policy, int priority){
    pthread_attr_t attr;
    struct sched_param param;
    int ret;

    pthread_attr_init(&attr);

    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if(ret != 0){
        fprintf(stderr, "main: setinheritsched: %s\n", strerror(ret));
        return ret;
    }

    ret = pthread_attr_setschedpolicy(&attr, policy);
    if(ret != 0){
        fprintf(stderr, "main: setschedpolicy: %s\n", strerror(ret));
        return ret;
    }

    param.sched_priority = priority;
    ret = pthread_attr_setschedparam(&attr, &param);
    if(ret != 0){
        fprintf(stderr, "main: setscheparam: %s\n", strerror(ret));
        return ret;
    }

    ret = pthread_create(thread, &attr, start_routine, arg);
    pthread_attr_destroy(&attr);
    
    if(ret != 0){
        fprintf(stderr, "main: pthread_create: %s\n", strerror(ret));
    }
    return ret;
}

int main(int argc, char *argv[]){

    if(argc < 2){
        fprintf(stderr, "Miss the file name\n");
        return 1;
    }


    if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1){
        perror("main: mlockall");
        return 1;
    }

    struct data_tasks data_main;

    sem_init(&data_main.sem_variable_y, 0, 1);
    sem_init(&data_main.sem_variable_u, 0 ,1);
    sem_init(&data_main.sem_buffer, 0, 1);
    sem_init(&data_main.can_send, 0, BUFFER_SIZE-1);
    sem_init(&data_main.can_save, 0, 0);
    sem_init(&data_main.sem_end,0 ,1);

    buffer_init(&data_main.buf);

    data_main.u_rt[0] = 1;
    data_main.u_rt[1] = 0.2 * M_PI;
    data_main.y_rt[0] = 0.3;
    data_main.y_rt[1] = 0;
    data_main.y_rt[2] = 0;
    data_main.num_threads_finished = 0;
    data_main.file_name = argv[1];

    if(clock_gettime(CLOCK_MONOTONIC, &data_main.timer_init) == -1){
        perror("init: clock_gettime");
        return 1;
    }

    pthread_t t_simulator;
    pthread_t t_generate_u;
    pthread_t t_sampling;
    pthread_t t_logger;
    pthread_t t_display;

    if(create_threads(&t_simulator, simulator, &data_main, SCHED_FIFO, PRIO_SIM) != 0){
        return 1;
    }
    if(create_threads(&t_generate_u, gen_u, &data_main, SCHED_FIFO, PRIO_GEN_U) != 0){
        return 1;
    }
    if(create_threads(&t_sampling, sampling, &data_main, SCHED_FIFO, PRIO_SAMPLING) != 0){
        return 1;
    }
    if(create_threads(&t_logger, logger, &data_main, SCHED_OTHER, 0) != 0){
        return 1;
    }
    if(create_threads(&t_display, display, &data_main, SCHED_OTHER, 0) != 0){
        return 1;
    }

    void *return_sim;
    void *return_gen_u;
    void *return_sampling;
    void *return_logger;
    void *return_display;

    pthread_join(t_simulator, &return_sim);
    pthread_join(t_generate_u, &return_gen_u);
    pthread_join(t_sampling, &return_sampling);
    
    if(sem_post(&data_main.can_save) < 0){
        perror("main: sem_post can_save");
        return 1;
    }

    pthread_join(t_logger, &return_logger);
    pthread_join(t_display, &return_display);

    int result_sim = (int)(intptr_t)return_sim;
    int result_gen_u = (int)(intptr_t)return_gen_u;
    int result_sampling = (int)(intptr_t)return_sampling;
    int result_logger = (int)(intptr_t)return_logger;
    int result_display = (int)(intptr_t)return_display;

    if(result_sim != 0){
        fprintf(stderr, "Simulator return error %d\n", result_sim);
        return 1;
    }
    if(result_gen_u != 0){
        fprintf(stderr, "Generate u return error %d\n", result_gen_u);
        return 1;
    }
    if(result_sampling != 0){
        fprintf(stderr, "Sampling return error %d\n", result_sampling);
        return 1;
    }
    if(result_logger != 0){
        fprintf(stderr, "Logger return error %d\n", result_logger);
        return 1;
    }
    if(result_display != 0){
        fprintf(stderr, "Display return error %d\n", result_display);
        return 1;
    }

    return 0;

}