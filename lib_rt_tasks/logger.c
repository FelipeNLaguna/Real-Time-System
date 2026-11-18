#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include "rt_tasks.h"

void *logger (void * data_tasks){

    struct data_tasks* data_logger = (struct data_tasks*)data_tasks;
    int return_log = 0;
    char sim_name_file[64];
    int len_sim_name_file;
    len_sim_name_file = snprintf(sim_name_file, sizeof(sim_name_file), "%s%s", data_logger->file_name, "_sim.dat");
    if((len_sim_name_file < 0 ) || (len_sim_name_file >= sizeof(sim_name_file))){
        fprintf(stderr, "logger: The file name is too long\n");
        return_log = 1;
        return (void *)(intptr_t) return_log;
    }
    FILE * sim_file = fopen(sim_name_file, "w");
    if(sim_file == NULL){
        perror("logger: fopen");
        return_log = 1;
        return (void *)(intptr_t) return_log;
    }

    FILE* sampling_file;
    char sampling_name_file[64];
    int len_sampling_name_file;
    len_sampling_name_file = snprintf(sampling_name_file, sizeof(sampling_name_file), "%s%s", data_logger->file_name, "_sampling.dat");
    if((len_sampling_name_file < 0 ) || (len_sampling_name_file >= sizeof(sampling_name_file))){
        fprintf(stderr, "logger: The file name is too long\n");
        return_log = 1;
        return (void *)(intptr_t) return_log;
    }
    
    sampling_file = fopen(sampling_name_file, "w");
    if(sampling_file == NULL){
        perror("logger: fopen");
        return_log = 1;
        return (void *)(intptr_t) return_log;
    }

    FILE* u_file;
    char gen_u_name_file[64];
    int len_gen_u_name_file;
    len_gen_u_name_file = snprintf(gen_u_name_file, sizeof(gen_u_name_file), "%s%s", data_logger->file_name, "_u.dat");
    if((len_gen_u_name_file < 0) || (len_gen_u_name_file >= sizeof(gen_u_name_file))){
        fprintf(stderr, "logger: The file name is too long\n");
        return_log = 1;
        return (void *)(intptr_t) return_log;
    }
    
    u_file = fopen(gen_u_name_file, "w");
    if(u_file == NULL){
        perror("logger: fopen");
        return_log = 1;
        return (void *)(intptr_t) return_log;
    }

    double y_log[3];
    double u_log[2];
    int64_t time_log_us;
    char thread_name;
    int ret_wait_save;
    int ret_wait_buf;
    int ret_wait_end;
    int num_threads_finished = 0;

    while((num_threads_finished < 3) || (buffer_count(&data_logger->buf))){
        
// *********************************************Critical***********************************************
        
        while(((ret_wait_save = sem_wait(&data_logger->can_save)) == -1) && errno == EINTR){
            // try again after EINTR
        }
        while(((ret_wait_buf = sem_wait(&data_logger->sem_buffer)) == -1) && errno == EINTR){
            // try again after EINTR
        }

        if(buffer_take(&data_logger->log_thread, &data_logger->buf) < 0){
            fprintf(stderr, "logger: buffer_take\n");
            if(sem_post(&data_logger->sem_buffer) < 0){
                perror("logger: sem_post sem_buffer");
                return_log = 1;
                return (void *)(intptr_t) return_log;
            }
            return_log = 1;
            return (void *)(intptr_t) return_log;
        }

        if(ret_wait_save == -1){
            perror("logger: sem_wait can_save");
            return_log = 1;
            return (void *)(intptr_t) return_log;
        }
        if(ret_wait_buf == -1){
            perror("logger: sem_wait sem_buffer");
            return_log = 1;
            return (void *)(intptr_t) return_log;
        }

        u_log[0] = data_logger->log_thread.u[0];
        u_log[1] = data_logger->log_thread.u[1];
        y_log[0] = data_logger->log_thread.y[0];
        y_log[1] = data_logger->log_thread.y[1];
        y_log[2] = data_logger->log_thread.y[2];
        time_log_us = data_logger->log_thread.time_us;
        thread_name = data_logger->log_thread.process_name;
        

        if(sem_post(&data_logger->sem_buffer) < 0){
            perror("logger: sem_post sem_buffer");
            return_log = 1;
            return (void *)(intptr_t) return_log;
        }
        if(sem_post(&data_logger->can_send) < 0){
            perror("logger: sem_post can_send");
            return_log = 1;
            return (void *)(intptr_t) return_log;
        }
        
// *****************************************************************************************************
             
        switch(thread_name){
            case TASK_SIM:
                fprintf(sim_file, "%"PRId64"\t", time_log_us);
                fprintf(sim_file, "%g\t%g\t%g\n", y_log[0], y_log[1], y_log[2]);
                break;
            case TASK_SAMPLING:
                fprintf(sampling_file, "%"PRId64"\t", time_log_us);
                fprintf(sampling_file, "%g\t%g\t%g\n", y_log[0], y_log[1], y_log[2]);
                break;

            case TASK_GEN_U:
                fprintf(u_file, "%"PRId64"\t", time_log_us);
                fprintf(u_file, "%g\t%g\n", u_log[0], u_log[1]);
                break;

            default:
                fprintf(stderr, "logger receive a invalid process\n");
                return_log = 1;
                return (void *)(intptr_t) return_log;
            }
    
// ***********************************Critical********************************************************
        while(((ret_wait_end = sem_wait(&data_logger->sem_end)) == -1) && errno == EINTR){
                // try again after EINTR
        }
        if(ret_wait_end == -1){
            perror("logger: sem_wait sem_end");
            return_log = 1;
            return (void *)(intptr_t) return_log;
        }

        num_threads_finished = data_logger->num_threads_finished;

        if(sem_post(&data_logger->sem_end) < 0){
            perror("logger: sem_post sem_end");
            return_log = 1;
            return (void *)(intptr_t) return_log;
        }
// ************************************************************************************************************    

    }
    
    fclose(sim_file);
    fclose(sampling_file);
    fclose(u_file);
    return (void*)(intptr_t) return_log;   
}