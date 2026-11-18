#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "rt_tasks.h"
#define SLEEP_TIME_NS 200000000

void *display(void *data_tasks){

    struct data_tasks *data_display = (struct data_tasks*)data_tasks;
    double last_y_value[3] = {0, 0, 0};
    double last_u_value[2] = {0, 0};
    int64_t time_display_us;
    int ret_wait_buf;
    int ret_wait_end;
    int num_threads_finished = 0;
    int result_display = 0;
    int ret_clk_ns;
    struct timespec sleep_time_display_ns;
    sleep_time_display_ns.tv_nsec = SLEEP_TIME_NS;
    sleep_time_display_ns.tv_sec = 0;

    while((num_threads_finished < 3) || (buffer_count(&data_display->buf) != 0)){
        
// *********************************Critical***********************************************
        while(((ret_wait_buf = sem_wait(&data_display->sem_buffer)) == -1) && errno == EINTR){
            // try again after EINTR
        }
        if(ret_wait_buf == -1){
            perror("display: sem_wait");
            result_display = 1;
            return (void *)(intptr_t) result_display;
        }
        time_display_us = data_display->log_thread.time_us;
        switch (data_display->log_thread.process_name)
            {
            case TASK_SAMPLING:
                last_y_value[0] = data_display->log_thread.y[0];
                last_y_value[1] = data_display->log_thread.y[1];
                last_y_value[2] = data_display->log_thread.y[2];
                break;
            case TASK_GEN_U:
                last_u_value[0] = data_display->log_thread.u[0];
                last_u_value[1] = data_display->log_thread.u[1];
                break;
            default:
                break;
            }
       
        if(sem_post(&data_display->sem_buffer) < 0){
            perror("display: sem_post sem_buffer");
            result_display = 1;
            return (void *)(intptr_t) result_display;
        }
        

// *******************************************************************************************
        printf("Time_us: %"PRId64"\t", time_display_us);
        printf("U value: (%g, %g)\t", last_u_value[0], last_u_value[1]);
        printf("Y value: (%g, %g, %g)\n", last_y_value[0], last_y_value[1], last_y_value[2]);
        fflush(stdout);

// ***********************************Critical********************************************************
        while(((ret_wait_end = sem_wait(&data_display->sem_end)) == -1) && errno == EINTR){
                // try again after EINTR
        }
        if(ret_wait_end == -1){
            perror("display: sem_wait sem_end");
            result_display = 1;
            return (void *)(intptr_t) result_display;
        }

        num_threads_finished = data_display->num_threads_finished;

        if(sem_post(&data_display->sem_end) < 0){
            perror("display: sem_post sem_end");
            result_display = 1;
            return (void *)(intptr_t) result_display;
        }
// ************************************************************************************************************        
    
        ret_clk_ns = clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_time_display_ns, NULL);
        while (ret_clk_ns == EINTR){
            ret_clk_ns = clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_time_display_ns, NULL);
        }
        if(ret_clk_ns > 0){
            fprintf(stderr, "display: clock_nanosleep: %s\n", strerror(ret_clk_ns));
            result_display = 1;
            return (void *)(intptr_t) result_display;
        }
    
    }

    return (void *)(intptr_t)result_display;

}
    
    