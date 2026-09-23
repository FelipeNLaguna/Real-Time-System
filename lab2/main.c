#define _USE_MATH_DEFINES
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <math.h>
#include "calculus.h"

#define DT 0.01
#define DEADLINE 20000000

FILE *file;

int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Falta o nome do arquivo para salvar os dados");
        return 1;
    }

    file = fopen(argv[1], "w");

    // x[0] hold the position x_c
    // x[1] hold the position y_c
    // x[2] hold the degree 
    float x[3] = {0, 0, 0};
    float pre_x[3] = {0, 0, 0};
    int i;
    float u[2];
    float x_der[3];
    float y[3];

    struct timeval timer_init;
    double time_init;

    pid_t pid_generate_u, pid_sampling_y;
    
    // pipe declaration
    int pipe_gen_u_to_sim[2];

    if(pipe(pipe_gen_u_to_sim) == -1){
        printf("Erro no pipe gen u to simulator\n");
        return 1;
    }
    fcntl(pipe_gen_u_to_sim[0], F_SETFL, O_NONBLOCK);
    if(gettimeofday(&timer_init, NULL) == -1){
        printf("Erro em pegar o tempo inicial\n");
    }
    time_init = (timer_init.tv_sec * 1000000) + timer_init.tv_usec;

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
    
            struct timeval timer_simulator;
            double time_sim = 0;
            double time_exec_sim;
            double u_array[2];

            while((time_sim - time_init) < DEADLINE){
                if(gettimeofday(&timer_simulator, NULL) == -1){
                    printf("Erro em pegar o tempo do simulador_dat\n");
                }
                
                time_sim = (timer_simulator.tv_sec * 1000000) + timer_simulator.tv_usec;

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
                for(i = 0; i < 3; i++){
                    pre_x[i] = x[i];
                }        
                
                if(write(pipe_sim_to_save_dat[1], y, sizeof(y)) == -1){
                    printf("Erro ao escrever no sampling/n");
                }
                if(gettimeofday(&timer_simulator, NULL) == -1){
                    printf("Erro em pegar o tempo do simulador_dat\n");
                }
                time_exec_sim = (timer_simulator.tv_sec * 1000000) + timer_simulator.tv_usec;
                if((time_exec_sim - time_sim) <= 30000){
                    usleep(30000 - (time_exec_sim - time_sim));
                }
            }
        }
        
        // sampling y
        else{
            // recebe os dados para salvar do arquivo do pipe e escreve
            close(pipe_sim_to_save_dat[1]);
            struct timeval timer_save_dat;
            double time_save_dat = 0;
            double time_exec_save_dat;
            double y_value[3];

            while((time_save_dat - time_init) < DEADLINE){
                if(gettimeofday(&timer_save_dat, NULL) == -1){
                    printf("Erro em pegar o tempo do save_dat\n");
                }    
                time_save_dat = (timer_save_dat.tv_sec * 1000000) + timer_save_dat.tv_usec;
                while(read(pipe_sim_to_save_dat[0], y_value, sizeof(y_value)) > 0){
                    // just clean the buffer
                }
                fprintf(file, "%g\t",(time_save_dat - time_init));
                fprintf(file, "%g\t%g\t%g\n", y_value[0], y_value[1], y_value[2]);
                if(gettimeofday(&timer_save_dat, NULL) == -1){
                    printf("Erro em pegar o tempo do save_dat\n");
                }
                time_exec_save_dat = (timer_save_dat.tv_sec * 1000000) + timer_save_dat.tv_usec;
                if((time_exec_save_dat - time_save_dat) <= 50000){
                    usleep(50000 - (time_exec_save_dat - time_save_dat));
                }
            }
        }

    }
    
    // generate u function
    else{
        
        struct timeval timer_u_curr;
        double time_gen_u = 0 ;
        double time_exec_gen_u;
        double vector_u[2];

        while((time_gen_u - time_init) < DEADLINE){
            if(gettimeofday(&timer_u_curr, NULL) == -1){
                printf("Erro em pegar o tempo\n");
            }
            
            time_gen_u = (timer_u_curr.tv_sec * 1000000) + timer_u_curr.tv_usec;

            close(pipe_gen_u_to_sim[0]);


            if((time_gen_u - time_init) < 0){
                vector_u[0] = 0;
                vector_u[1] = 0;
            }
            else if( ((time_gen_u - time_init) >= 0) && ((time_gen_u - time_init) < 10000000) ){
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
            
            if(gettimeofday(&timer_u_curr, NULL) == -1){
                printf("Erro em pegar o tempo\n");
            }
            time_exec_gen_u = (timer_u_curr.tv_sec * 1000000) + timer_u_curr.tv_usec;
            if((time_exec_gen_u - time_gen_u) <= 50000){
                usleep(50000 - (time_exec_gen_u - time_gen_u));
            }
        }
    }


    fclose(file);

    return 0;
}