#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "rt_tasks.h"

void *sampling(void *data_tasks){

    struct data_tasks *data_sampling = (struct data_tasks*)data_tasks;
    struct timespec timer_save_dat;
    struct timespec next_activation_sampling;
    next_activation_sampling.tv_nsec = data_sampling->timer_init.tv_nsec;
    next_activation_sampling.tv_sec = data_sampling->timer_init.tv_sec;
    int64_t next_activation_sampling_time_ns = 0;
    int64_t time_save_dat;
    int64_t time_init;
    time_init = timespec_to_ns(data_sampling->timer_init);
    int ret_clk_n_sampling;
    int ret_wait_y;
    int result_sampling = 0;
    int dropped_sampling = 0;
    double y_value[3] = {0, 0, 0};

    while((next_activation_sampling_time_ns - time_init) < DURATION_NS){
        if(clock_gettime(CLOCK_MONOTONIC, &timer_save_dat) == -1){
            perror("sampling: clock_gettime");
            result_sampling = 1;
            return (void *)(intptr_t) result_sampling;
        }    
        time_save_dat = timespec_to_ns(timer_save_dat);

// ********************************Critical******************************************
        while(((ret_wait_y = sem_wait(&data_sampling->sem_variable_y)) == -1) && errno == EINTR){
            // try again after EINTR
        }
        if(ret_wait_y == -1){
            perror("sampling: sem_wait");
            result_sampling = 1;
            return (void *)(intptr_t) result_sampling;
        }

        y_value[0] = data_sampling->y_rt[0];
        y_value[1] = data_sampling->y_rt[1];
        y_value[2] = data_sampling->y_rt[2];

        if(sem_post(&data_sampling->sem_variable_y) < 0){
            perror("sampling: sem_post sem_variable_y");
            result_sampling = 1;
            return (void *)(intptr_t) result_sampling;
        }
        
// ***********************************************************************************

// ********************************Critical*****************************************

        if(sem_trywait(&data_sampling->can_send) == 0){
            if(sem_trywait(&data_sampling->sem_buffer) == 0){
                
                data_sampling->log_thread.time_us = (time_save_dat - time_init)/1000;
                data_sampling->log_thread.y[0] = y_value[0];
                data_sampling->log_thread.y[1] = y_value[1];
                data_sampling->log_thread.y[2] = y_value[2];
                data_sampling->log_thread.process_name = TASK_SAMPLING;
                
                if(buffer_append(data_sampling->log_thread, &data_sampling->buf) < 0){
                    fprintf(stderr, "sampling: buffer_append\n");
                    if(sem_post(&data_sampling->sem_buffer) < 0){
                        perror("sampling: sem_post sem_buffer");
                        result_sampling = 1;
                        return (void *)(intptr_t) result_sampling;
                    }
                    result_sampling = 1;
                    return (void *)(intptr_t) result_sampling;
                }

                if(sem_post(&data_sampling->sem_buffer) < 0){
                    perror("sampling: sem_post sem_buffer");
                    result_sampling = 1;
                    return (void *)(intptr_t) result_sampling;
                }
                if(sem_post(&data_sampling->can_save) < 0){
                    perror("sampling: sem_post can_save");
                    result_sampling = 1;
                    return (void *)(intptr_t) result_sampling;
                }
            }
            else{
                if(errno == EAGAIN){
                    dropped_sampling++;
                }
                else{
                    perror("sampling: sem_trywait");
                    result_sampling = 1;
                    return (void *)(intptr_t)result_sampling;
                }
                if(sem_post(&data_sampling->can_send) < 0){
                    perror("sampling: sem_post can_send");
                    result_sampling = 1;
                    return (void *)(intptr_t) result_sampling;
                }
            }
        }
        else{
            if(errno == EAGAIN){
                dropped_sampling++;
            }
            else{
                perror("sampling: sem_trywait");
                result_sampling = 1;
                return (void *)(intptr_t)result_sampling;
            }
        }

// *******************************************************************************


        timespec_add_ns(&next_activation_sampling, T_SAMPLING_NS);
        next_activation_sampling_time_ns = timespec_to_ns(next_activation_sampling);
        
        ret_clk_n_sampling = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_sampling, NULL);
        
        while(ret_clk_n_sampling == EINTR){
            ret_clk_n_sampling = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_sampling, NULL);
        }
        if(ret_clk_n_sampling > 0){
            fprintf(stderr, "sampling: clock_nanosleep:  %s\n", strerror(ret_clk_n_sampling));
            result_sampling = 1;
            return (void *)(intptr_t)result_sampling;
        }
    
    }

    if(dropped_sampling > 0) fprintf(stderr, "sampling: %d records dropped by logger\n", dropped_sampling);

    int ret_wait_end;

// ***********************************Critical********************************************************
    while(((ret_wait_end = sem_wait(&data_sampling->sem_end)) == -1) && errno == EINTR){
            // try again after EINTR
    }
    if(ret_wait_end == -1){
        perror("sampling: sem_wait sem_end");
        result_sampling = 1;
        return (void *)(intptr_t) result_sampling;
    }

    data_sampling->num_threads_finished = data_sampling->num_threads_finished + 1;

    if(sem_post(&data_sampling->sem_end) < 0){
        perror("sampling: sem_post sem_end");
        result_sampling = 1;
        return (void *)(intptr_t) result_sampling;
    }
// ************************************************************************************************************    


    return (void *)(intptr_t)result_sampling;
}