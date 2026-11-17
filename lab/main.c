#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>
#include "calculus.h"
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

int run_display(int pipe_display[2], int pipe_sim[2], int pipe_u[2], int pipe_logger[2]){
    struct sched_param param_display;
    param_display.sched_priority = 0;
    if(sched_setscheduler(0, SCHED_OTHER, &param_display) == -1){
        perror("display: sched_setscheduler");
        return 1;
    }

    if(close(pipe_logger[0]) == -1){
        perror("display: close pipe C read");
        return 1;
    }
    if(close(pipe_logger[1]) == -1){
        perror("display: close pipe C write");
        return 1;
    }
    if(close(pipe_sim[0]) == -1){
        perror("display: close pipe B read");
        return 1;
    }
    if(close(pipe_sim[1]) == -1){
        perror("display: close pipe B write");
        return 1;
    }
    if(close(pipe_u[0]) == -1){
        perror("display: close pipe A read");
        return 1;
    }
    if(close(pipe_u[1]) == -1){
        perror("display: close pipe A write");
        return 1;
    }
    if(close(pipe_display[1]) == -1){
        perror("display: close pipe D write");
        return 1;
    }

    struct log_record data_showed;
    ssize_t ret_read_display;
    double last_y_value[3] = {0, 0, 0};
    double last_u_value[2] = {0, 0};

    while((ret_read_display = read(pipe_display[0], &data_showed, sizeof(data_showed))) > 0 ){
        switch (data_showed.process_name)
            {
            case TASK_SAMPLING:
                last_y_value[0] = data_showed.y[0];
                last_y_value[1] = data_showed.y[1];
                last_y_value[2] = data_showed.y[2];
                break;
            case TASK_GEN_U:
                last_u_value[0] = data_showed.u[0];
                last_u_value[1] = data_showed.u[1];
                break;
            default:
                fprintf(stderr, "display receive a invalid process\n");
                return 1;
            }
        printf("Time_us: %"PRId64"\t", data_showed.time_us);
        printf("U value: (%g, %g)\t", last_u_value[0], last_u_value[1]);
        printf("Y value: (%g, %g, %g)\n", last_y_value[0], last_y_value[1], last_y_value[2]);
        fflush(stdout);
    }
    if(ret_read_display == -1){
        perror("display: read pipe D");
        return 1;
    }

    if(close(pipe_display[0]) == -1){
        perror("display: close pipe D write");
        return 1;
    }

    return 0;
}

int run_logger(int pipe_logger[2], const char *file_name, int pipe_sim[2], int pipe_u[2], int pipe_display[2]){

    struct sched_param param_logger;
    param_logger.sched_priority = 0;
    if(sched_setscheduler(0, SCHED_OTHER, &param_logger) == -1){
        perror("logger: sched_setscheduler");
        return 1;
    }
    
    char sim_name_file[64];
    int len_sim_name_file;
    len_sim_name_file = snprintf(sim_name_file, sizeof(sim_name_file), "%s%s", file_name, "_sim.dat");
    if((len_sim_name_file < 0 ) || (len_sim_name_file >= sizeof(sim_name_file))){
        fprintf(stderr, "logger: The file name is too long\n");
        return 1;
    }
    FILE * sim_file = fopen(sim_name_file, "w");
    if(sim_file == NULL){
        perror("logger: fopen");
        return 1;
    }
    
    if(close(pipe_display[0]) == -1){
        perror("logger: close pipe D write");
        return 1;
    }
    if(close(pipe_display[1]) == -1){
        perror("logger: close pipe D write");
        return 1;
    }
    if(close(pipe_logger[1]) == -1){
        perror("logger: close pipe C");
        return 1;
    }
    if(close(pipe_sim[0]) == -1){
        perror("logger: close read pipe B");
        return 1;
    }
    if(close(pipe_sim[1]) == -1){
        perror("logger: close write pipe B");
        return 1;
    }
    if(close(pipe_u[0]) == -1){
        perror("logger: close read pipe A");
        return 1;
    }
    if(close(pipe_u[1]) == -1){
        perror("logger: close write pipe A");
        return 1;
    }


    FILE* sampling_file;
    char sampling_name_file[64];
    int len_sampling_name_file;
    len_sampling_name_file = snprintf(sampling_name_file, sizeof(sampling_name_file), "%s%s", file_name, "_sampling.dat");
    if((len_sampling_name_file < 0 ) || (len_sampling_name_file >= sizeof(sampling_name_file))){
        fprintf(stderr, "logger: The file name is too long\n");
        return 1;
    }
    
    sampling_file = fopen(sampling_name_file, "w");
    if(sampling_file == NULL){
        perror("logger: fopen");
        return 1;
    }

    FILE* u_file;
    char gen_u_name_file[64];
    int len_gen_u_name_file;
    len_gen_u_name_file = snprintf(gen_u_name_file, sizeof(gen_u_name_file), "%s%s", file_name, "_u.dat");
    if((len_gen_u_name_file < 0) || (len_gen_u_name_file >= sizeof(gen_u_name_file))){
        fprintf(stderr, "logger: The file name is too long\n");
        return 1;
    }
    
    u_file = fopen(gen_u_name_file, "w");
    if(u_file == NULL){
        perror("logger: fopen");
        return 1;
    }
    
    struct log_record data;
    
    ssize_t return_read_logger;

    while((return_read_logger = read(pipe_logger[0], &data, sizeof(data))) > 0){
        switch(data.process_name){
            case TASK_SIM:
                fprintf(sim_file, "%"PRId64"\t",data.time_us);
                fprintf(sim_file, "%g\t%g\t%g\n", data.y[0], data.y[1], data.y[2]);
                break;
            case TASK_SAMPLING:
                fprintf(sampling_file, "%"PRId64"\t",data.time_us);
                fprintf(sampling_file, "%g\t%g\t%g\n", data.y[0], data.y[1], data.y[2]);
                break;

            case TASK_GEN_U:
                fprintf(u_file, "%"PRId64"\t", data.time_us);
                fprintf(u_file, "%g\t%g\n", data.u[0], data.u[1]);
                break;

            default:
                fprintf(stderr, "logger receive a invalid process\n");
                return 1;
            }
    }
    if(return_read_logger == -1){
        perror("logger: read pipe C");
        return 1;
    }

    fclose(sim_file);
    fclose(sampling_file);
    fclose(u_file);
    return 0;
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

    buffer_init(&data_main.buf);

    data_main.u_rt[0] = 1;
    data_main.u_rt[1] = 0.2 * M_PI;
    data_main.y_rt[0] = 0.3;
    data_main.y_rt[1] = 0;
    data_main.y_rt[2] = 0;
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