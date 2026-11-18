#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "rt_tasks.h"

void *gen_u(void *data_tasks){

    struct data_tasks *data_u = (struct data_tasks *)data_tasks;
    struct timespec timer_u_curr;
    struct timespec next_activation_gen;
    next_activation_gen.tv_nsec = data_u->timer_init.tv_nsec;
    next_activation_gen.tv_sec = data_u->timer_init.tv_sec;
    int64_t next_activation_gen_time_ns = 0;
    int64_t time_gen_u;
    int64_t time_init;
    time_init = timespec_to_ns(data_u->timer_init);
    int ret_clk_n_gen;
    int ret_wait_u;
    double vector_u[2];
    int dropped_u = 0;
    int result_u = 0;

    while((next_activation_gen_time_ns - time_init) < DURATION_NS){
        if(clock_gettime(CLOCK_MONOTONIC, &timer_u_curr) == -1){
            perror("generate u: clock_gettime");
            result_u = 1;
            return (void *)(intptr_t) result_u;
        }
        
        time_gen_u = timespec_to_ns(timer_u_curr); 

        if((time_gen_u - time_init) < 0){
            vector_u[0] = 0;
            vector_u[1] = 0;
        }
        else if( ((time_gen_u - time_init) >= 0) && ((time_gen_u - time_init) < CHANGE_U_NS) ){
            vector_u[0] = 1;
            vector_u[1] = 0.2 * M_PI;
        }
        else{
            vector_u[0] = 1;
            vector_u[1] = -0.2 * M_PI;
        }
//***********************************Critical**********************************
        while(((ret_wait_u = sem_wait(&data_u->sem_variable_u)) == -1) && errno == EINTR){
           // try again after EINTR
        }
        if(ret_wait_u == -1){
            perror("generate u: sem_wait");
            result_u = 1;
            return (void *)(intptr_t) result_u;
        }
        data_u->u_rt[0] = vector_u[0];
        data_u->u_rt[1] = vector_u[1];
        if(sem_post(&data_u->sem_variable_u) < 0){
            perror("generate u: sem_post");
            result_u = 1;
            return (void *)(intptr_t) result_u;
        }
// ****************************************************************************


//*************************************Critical********************************
            
        if(sem_trywait(&data_u->can_send) == 0){
            if(sem_trywait(&data_u->sem_buffer) == 0){
                
                data_u->log_thread.time_us = (time_gen_u - time_init)/1000;
                data_u->log_thread.u[0] = vector_u[0];
                data_u->log_thread.u[1] = vector_u[1];
                data_u->log_thread.process_name = TASK_GEN_U;
                
                if(buffer_append(data_u->log_thread, &data_u->buf) < 0){
                    fprintf(stderr, "generate u: buffer_append\n");
                    if(sem_post(&data_u->sem_buffer) < 0){
                        perror("generate u: sem_post sem_buffer");
                        result_u = 1;
                        return (void *)(intptr_t) result_u;
                    }
                    result_u = 1;
                    return (void *)(intptr_t) result_u;
                }

                if(sem_post(&data_u->sem_buffer) < 0){
                    perror("generate u: sem_post sem_buffer");
                    result_u = 1;
                    return (void *)(intptr_t) result_u;
                }
                if(sem_post(&data_u->can_save) < 0){
                    perror("generate u: sem_post can_save");
                    result_u = 1;
                    return (void *)(intptr_t) result_u;
                }
            }
            else{
                if(errno == EAGAIN){
                    dropped_u++;
                }
                else{
                    perror("generate u: sem_trywait");
                    result_u = 1;
                    return (void *)(intptr_t)result_u;
                }
                if(sem_post(&data_u->can_send) < 0){
                    perror("generate u: sem_post can_send");
                    result_u = 1;
                    return (void *)(intptr_t) result_u;
                }
            }
        }
        else{
            if(errno == EAGAIN){
                dropped_u++;
            }
            else{
                perror("generate u: sem_trywait");
                result_u = 1;
                return (void *)(intptr_t)result_u;
            }
        }


// ******************************************************************************

        timespec_add_ns(&next_activation_gen, T_GEN_U_NS);
        next_activation_gen_time_ns = timespec_to_ns(next_activation_gen);

        ret_clk_n_gen = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_gen, NULL);

        while(ret_clk_n_gen == EINTR){
            ret_clk_n_gen = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_gen, NULL);
        }
        if(ret_clk_n_gen > 0){
            fprintf(stderr, "generate u: clock_nanosleep: %s\n", strerror(ret_clk_n_gen));
            result_u = 1;
            return (void *)(intptr_t)result_u;
        }
    }
    
    if(dropped_u > 0) {
        fprintf(stderr, "generate u: %d records dropped by logger\n", dropped_u);
    }

    int ret_wait_end; 

// ***********************************Critical********************************************************
    while(((ret_wait_end = sem_wait(&data_u->sem_end)) == -1) && errno == EINTR){
            // try again after EINTR
    }
    if(ret_wait_end == -1){
        perror("generate u: sem_wait sem_end");
        result_u = 1;
        return (void *)(intptr_t) result_u;
    }

    data_u->num_threads_finished = data_u->num_threads_finished + 1;

    if(sem_post(&data_u->sem_end) < 0){
        perror("generate u: sem_post sem_end");
        result_u = 1;
        return (void *)(intptr_t) result_u;
    }
// ************************************************************************************************************    

    return (void *)(intptr_t)result_u;
}
