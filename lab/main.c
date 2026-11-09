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
#include "calculus.h"

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

int main(int argc, char *argv[]){

    if(argc < 2){
        fprintf(stderr, "Miss the file name\n");
        return 1;
    }

    double u[2] = {0, 0};
    double x_der[3];
    double y[3];

    struct timespec timer_init;
    int64_t time_init;

    struct sched_param param_rt;
    param_rt.sched_priority = PRIO_SIM;
    if(sched_setscheduler(0, SCHED_FIFO, &param_rt) == -1){
        perror("simulator: sched_setsetscheduler");
        return 1;
    }

    pid_t pid_generate_u, pid_sampling_y;
    
    // pipe declaration
    // pipe A
    int pipe_gen_u_to_sim[2];

    if(pipe(pipe_gen_u_to_sim) == -1){
        printf("Erro no pipe gen u to simulator\n");
        return 1;
    }
    fcntl(pipe_gen_u_to_sim[0], F_SETFL, O_NONBLOCK);
    if(clock_gettime(CLOCK_MONOTONIC, &timer_init) == -1){
        perror("init: clock_gettime");
    }
    time_init = timespec_to_ns(timer_init); 

    // first fork
    pid_generate_u = fork();

    // error
    if(pid_generate_u < 0 ){
        printf("Erro no fork do generate\n");
        return 1;
    }
    // main processor
    else if(pid_generate_u > 0){
        // ========================================================================================
        // pipe B
        int pipe_sim_to_save_dat[2];
        if(pipe(pipe_sim_to_save_dat) == -1){
            printf("Erro no pipe simulator to save_data\n");
            return 1;
        }
        fcntl(pipe_sim_to_save_dat[0], F_SETFL, O_NONBLOCK);
        // =========================================================================================
        pid_sampling_y = fork();
        
        // error
        if(pid_sampling_y < 0 ){
            printf("Erro no fork do sampling\n");
            return 1;
        }
        
        // simulator
        else if(pid_sampling_y > 0 ){
    
            char sim_name_file[64];
            int len_sim_name_file;
            len_sim_name_file = snprintf(sim_name_file, sizeof(sim_name_file), "%s%s", argv[1], "_sim.dat");
            if((len_sim_name_file < 0 ) || (len_sim_name_file >= sizeof(sim_name_file))){
                fprintf(stderr, "simulator: The file name is too long\n");
                return 1;
            }
            FILE * sim_file = fopen(sim_name_file, "w");
            if(sim_file == NULL){
                perror("simulator: fopen");
                return 1;
            }

            struct timespec timer_simulator;
            struct timespec next_activation_sim;
            next_activation_sim.tv_nsec = timer_init.tv_nsec;
            next_activation_sim.tv_sec = timer_init.tv_sec;
            int64_t next_activation_sim_time_ns = 0;
            int64_t time_sim;
            int ret_clk_n;
            double u_array[2];
            // x[0] hold the position x_c
            // x[1] hold the position y_c
            // x[2] hold the degree 
            double x[3] = {0, 0, 0};
            double pre_x[3] = {0, 0, 0};
            
            // simulator just write to pipe_sim_to_dat
            if(close(pipe_sim_to_save_dat[0]) == -1){
                perror("simulator: close pipe B read");
                return 1;
            }
            // simulator just read to pipe_gen_u_to_sim
            if(close(pipe_gen_u_to_sim[1]) == -1){
                perror("simulator: close pipe A write");
                return 1;
            }

            if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1){
                perror("simulator: mlockall");
                return 1;
            }

            while((next_activation_sim_time_ns - time_init) < DURATION_NS){
                if(clock_gettime(CLOCK_MONOTONIC, &timer_simulator) == -1){
                    perror("simulator: clock_gettime");
                }
                
                time_sim = timespec_to_ns(timer_simulator);

                if(read(pipe_gen_u_to_sim[0], u_array, sizeof(u_array)) > 0){
                    u[0] = u_array[0];
                    u[1] = u_array[1];
                }  
                // calcula x' 
                derivate(u, x, x_der);

                // calcula y
                result_y(x, y);

                // calcula o x
                next_x(x, pre_x, x_der, DT);
                for(int i = 0; i < 3; i++){
                    pre_x[i] = x[i];
                }        
                
                if(write(pipe_sim_to_save_dat[1], y, sizeof(y)) == -1){
                    printf("Erro ao escrever no sampling\n");
                }

                fprintf(sim_file, "%"PRId64"\t",((time_sim - time_init)/1000));
                fprintf(sim_file, "%g\t%g\t%g\n", y[0], y[1], y[2]);
                
                timespec_add_ns(&next_activation_sim, T_SIM_NS);
                next_activation_sim_time_ns = timespec_to_ns(next_activation_sim);

                ret_clk_n = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_sim, NULL);
                while (ret_clk_n == EINTR){
                    ret_clk_n = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_sim, NULL);
                }
                if(ret_clk_n > 0){
                    fprintf(stderr, "simulator: %s\n", strerror(ret_clk_n));
                }
            }
            if(close(pipe_sim_to_save_dat[1]) == -1){
                perror("simulator: close pipe B write");
                return 1;
            }
            
            pid_t pid_wait;
            int status;
            int exit_status;
            int signal_value;
            int child_failed = 0;
            do{
                pid_wait = wait(&status);
                if(pid_wait == -1){
                    if(errno == ECHILD){
                        // all children reaped: normal loop exit
                    }
                    else if(errno == EINTR){
                        fprintf(stderr, "wait() was interrupted by a signal\n");
                        pid_wait = 1;
                    }
                    else{
                        perror("wait");
                    }
                }
                else{
                    if(WIFEXITED(status)){
                        exit_status = WEXITSTATUS(status);
                        if(exit_status == 0){
                            // exit return the correct value
                        }
                        else{
                            fprintf(stderr, "child %d exited with code %d\n", pid_wait, exit_status);
                            child_failed = 1;
                        }
                    }
                    else if(WIFSIGNALED(status)){
                        signal_value = WTERMSIG(status);
                        fprintf(stderr, "child %d killed by signal: %d\n", pid_wait, signal_value);
                        child_failed = 1;
                    }
                }
            }
            while((pid_wait > 0)); // if the wait was interruped for the system pid_wait = 1   
            if(close(pipe_gen_u_to_sim[0]) == -1){
                perror("simulator: close pipe A read end");
                return 1;
            }         
            fclose(sim_file);
            return child_failed;
        }
        
        // sampling y
        else{
            FILE* sampling_file;
            char sampling_name_file[64];
            int len_sampling_name_file;
            len_sampling_name_file = snprintf(sampling_name_file, sizeof(sampling_name_file), "%s%s", argv[1], "_sampling.dat");
            if((len_sampling_name_file < 0 ) || (len_sampling_name_file >= sizeof(sampling_name_file))){
                fprintf(stderr, "sampling: The file name is too long\n");
                return 1;
            }
            
            sampling_file = fopen(sampling_name_file, "w");
            if(sampling_file == NULL){
                perror("sampling: fopen");
                return 1;
            }

            // recebe os dados para salvar do arquivo do pipe e escreve
            if(close(pipe_sim_to_save_dat[1]) == -1){
                perror("sampling: close pipe B write");
                return 1;
            }
            if(close(pipe_gen_u_to_sim[0]) == -1){
                perror("sampling: close pipe A read");
                return 1;
            }
            if(close(pipe_gen_u_to_sim[1]) == -1){
                perror("sampling: close pipe A write");
                return 1;
            }
            struct timespec timer_save_dat;
            struct timespec next_activation_sampling;
            next_activation_sampling.tv_nsec = timer_init.tv_nsec;
            next_activation_sampling.tv_sec = timer_init.tv_sec;
            int64_t next_activation_sampling_time_ns = 0;
            int64_t time_save_dat;
            int ret_clk_n_sampling;

            struct sched_param param_sampling;
            param_sampling.sched_priority = PRIO_SAMPLING;
            if(sched_setscheduler(0, SCHED_FIFO, &param_sampling) == -1){
                perror("sampling: sched_setscheduler");
                return 1;
            }
            // falta implementar o result_y quando t < 0
            double y_value[3] = {0.3, 0, 0};

            if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1){
                perror("sampling: mlockall");
                return 1;
            }

            while((next_activation_sampling_time_ns - time_init) < DURATION_NS){
                if(clock_gettime(CLOCK_MONOTONIC, &timer_save_dat) == -1){
                    perror("sampling: clock_gettime");
                }    
                time_save_dat = timespec_to_ns(timer_save_dat);
                while(read(pipe_sim_to_save_dat[0], y_value, sizeof(y_value)) > 0){
                    // just clean the buffer
                }
                fprintf(sampling_file, "%"PRId64"\t",((time_save_dat - time_init)/1000));
                fprintf(sampling_file, "%g\t%g\t%g\n", y_value[0], y_value[1], y_value[2]);

                timespec_add_ns(&next_activation_sampling, T_SAMPLING_NS);
                next_activation_sampling_time_ns = timespec_to_ns(next_activation_sampling);
                
                ret_clk_n_sampling = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_sampling, NULL);
                
                while(ret_clk_n_sampling == EINTR){
                    ret_clk_n_sampling = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_sampling, NULL);
                }
                if(ret_clk_n_sampling > 0){
                    fprintf(stderr, "sampling: %s\n", strerror(ret_clk_n_sampling));
                }
            
            }

            ssize_t byte_read_pipe_b;
            do{
                byte_read_pipe_b = read(pipe_sim_to_save_dat[0], y_value, sizeof(y_value));
                if(byte_read_pipe_b == -1){
                    if(errno == EAGAIN){
                        usleep(3000);
                    }
                    else{
                        perror("sampling: read");
                        exit(1);
                    }
                }
                else if(byte_read_pipe_b > 0){
                    // forwent the datas
                }
                
            }while(byte_read_pipe_b != 0);

            // close pipe
            if(close(pipe_sim_to_save_dat[0]) == -1){
                perror("sampling: close pipe B read");
                return 1;
            }
            fclose(sampling_file);
            exit(0);
        }

    }
    
    // generate u function
    else{
        
        FILE* gen_u_file;
        char gen_u_name_file[64];
        int len_gen_u_name_file;
        len_gen_u_name_file = snprintf(gen_u_name_file, sizeof(gen_u_name_file), "%s%s", argv[1], "_u.dat");
        if((len_gen_u_name_file < 0) || (len_gen_u_name_file >= sizeof(gen_u_name_file))){
            fprintf(stderr, "generate u: The file name is too long\n");
            return 1;
        }
        
        gen_u_file = fopen(gen_u_name_file, "w");
        if(gen_u_file == NULL){
            perror("gen u: fopen");
            return 1;
        }

        struct timespec timer_u_curr;
        struct timespec next_activation_gen;
        next_activation_gen.tv_nsec = timer_init.tv_nsec;
        next_activation_gen.tv_sec = timer_init.tv_sec;
        int64_t next_activation_gen_time_ns = 0;
        int64_t time_gen_u;
        int ret_clk_n_gen;
        double vector_u[2];

        struct sched_param param_gen_u;
        param_gen_u.sched_priority = PRIO_GEN_U;
        if(sched_setscheduler(0, SCHED_FIFO, &param_gen_u) == -1){
            perror("generate u: sched_setscheduler");
            return 1;
        }

        if(close(pipe_gen_u_to_sim[0]) == -1){
            perror("generate u: close pipe A read");
            return 1;
        }

        if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1){
            perror("generate u: mlockall");
            return 1;
        }

        while((next_activation_gen_time_ns - time_init) < DURATION_NS){
            if(clock_gettime(CLOCK_MONOTONIC, &timer_u_curr) == -1){
                perror("generate u: clock_gettime");
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
            if(write(pipe_gen_u_to_sim[1], vector_u, sizeof(vector_u)) == -1){
                printf("Erro ao mandar o u gerado\n");
            }

            fprintf(gen_u_file, "%"PRId64"\t",((time_gen_u - time_init)/1000));
            fprintf(gen_u_file, "%g\t%g\n", vector_u[0], vector_u[1]);

            timespec_add_ns(&next_activation_gen, T_GEN_U_NS);
            next_activation_gen_time_ns = timespec_to_ns(next_activation_gen);

            ret_clk_n_gen = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_gen, NULL);

            while(ret_clk_n_gen == EINTR){
                ret_clk_n_gen = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation_gen, NULL);
            }
            if(ret_clk_n_gen > 0){
                fprintf(stderr, "generate u: %s\n", strerror(ret_clk_n_gen));
            }
        }
        if(close(pipe_gen_u_to_sim[1]) == -1){
            perror("generate u: close pipe write A end");
            return 1;
        }
        fclose(gen_u_file);
        exit(0);
    }

}