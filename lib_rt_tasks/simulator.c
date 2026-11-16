#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "rt_tasks.h"
#include "calculus.h"

void *simulator(void *data_tasks){
    
    struct data_tasks *data_sim = (struct data_tasks*)data_tasks;
    struct timespec timer_simulator;
    struct timespec next_activation_sim;
    next_activation_sim.tv_nsec = data_sim->timer_init.tv_nsec;
    next_activation_sim.tv_sec = data_sim->timer_init.tv_sec;
    int64_t next_activation_sim_time_ns = 0;
    int64_t time_sim;
    int64_t time_init;
    int ret_clk_n;
    // x[0] hold the position x_c
    // x[1] hold the position y_c
    // x[2] hold the degree 
    double x[3] = {0, 0, 0};
    double pre_x[3] = {0, 0, 0};
    double x_der[3] = {0, 0, 0};
    double u[2] = {0, 0};
    double y[3] = {0, 0, 0};
    int dropped_sim = 0;
    int res_sim = 0;
    int ret_wait_y;
    int ret_wait_u;

    time_init = timespec_to_ns(data_sim->timer_init);

    while((next_activation_sim_time_ns - time_init) < DURATION_NS){
        if(clock_gettime(CLOCK_MONOTONIC, &timer_simulator) == -1){
            perror("simulator: clock_gettime");
            res_sim = 1;
            return (void *)(intptr_t)res_sim;
        }
        
        time_sim = timespec_to_ns(timer_simulator);


//***************************Critical************************************************
        while(((ret_wait_u = sem_wait(&data_sim->sem_variable_u)) == -1) && errno == EINTR){
           // try again after EINTR
        }
        if(ret_wait_u == -1){
            perror("simulator: sem_wait");
            res_sim = 1;
            return (void *)(intptr_t) res_sim;
        }
        u[0] = data_sim->u_rt[0];
        u[1] = data_sim->u_rt[1];
        if(sem_post(&data_sim->sem_variable_u) < 0){
            perror("simulator: sem_post");
            res_sim = 1;
            return (void *)(intptr_t) res_sim;
        }
//*********************************************************************************

        // compute x' 
        derivate(u, x, x_der);

        
        // compute y
        result_y(x, y);
//*************************Critical***************************

        while(((ret_wait_y = sem_wait(&data_sim->sem_variable_y)) == -1) && errno == EINTR){
            // try again after EINTR
        }
        if(ret_wait_y == -1){
            perror("simulator: sem_wait");
            res_sim = 1;
            return (void *)(intptr_t) res_sim;
        }

        data_sim->y_rt[0] = y[0];
        data_sim->y_rt[1] = y[1];
        data_sim->y_rt[2] = y[2];

        if(sem_post(&data_sim->sem_variable_y) < 0){
            perror("simulator: sem_post sem_variable_y");
            res_sim = 1;
            return (void *)(intptr_t) res_sim;
        }
//*********************************************************************

        // compute x
        next_x(x, pre_x, x_der, DT); // I think DT is just use in this function
        for(int i = 0; i < 3; i++){
            pre_x[i] = x[i];
        }        
// ************************ Critical **********************************************
        if(sem_trywait(&data_sim->can_send) == 0){
            if(sem_trywait(&data_sim->sem_buffer) == 0){
                
                data_sim->log_thread.time_us = (time_sim - time_init)/1000;
                data_sim->log_thread.y[0] = y[0];
                data_sim->log_thread.y[1] = y[1];
                data_sim->log_thread.y[2] = y[2];
                data_sim->log_thread.process_name = TASK_SIM;
                if(buffer_append(data_sim->log_thread, &data_sim->buf) < 0){
                    
                }

                if(sem_post(&data_sim->sem_buffer) < 0){
                    perror("simulator: sem_post sem_buffer");
                    res_sim = 1;
                    return (void *)(intptr_t) res_sim;
                }
                if(sem_post(&data_sim->can_save) < 0){
                    perror("simulator: sem_post can_save");
                    res_sim = 1;
                    return (void *)(intptr_t) res_sim;
                }
            }
            else{
                if(errno == EAGAIN){
                    dropped_sim++;
                }
                else{
                    perror("simulator: sem_trywait");
                    res_sim = 1;
                    return (void *)(intptr_t)res_sim;
                }
                if(sem_post(&data_sim->can_send) < 0){
                    perror("simulator: sem_post can_send");
                    res_sim = 1;
                    return (void *)(intptr_t) res_sim;
                }
            }
        }
        else{
            if(errno == EAGAIN){
                dropped_sim++;
            }
            else{
                perror("simulator: sem_trywait");
                res_sim = 1;
                return (void *)(intptr_t)res_sim;
            }
        }
// ************************************************************************************
        
        timespec_add_ns(&next_activation_sim, T_SIM_NS);
        next_activation_sim_time_ns = timespec_to_ns(next_activation_sim);

        ret_clk_n = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_sim, NULL);
        while (ret_clk_n == EINTR){
            ret_clk_n = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_sim, NULL);
        }
        if(ret_clk_n > 0){
            fprintf(stderr, "simulator: clock_nanosleep: %s\n", strerror(ret_clk_n));
        }
    }

    if(dropped_sim > 0) {
    fprintf(stderr, "simulator: %d records dropped by logger\n", dropped_sim);
    }

    return (void *)(intptr_t)res_sim;        

}
