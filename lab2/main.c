#include <stdio.h>
#include "calculus.h"

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
    float x[3] = {0};
    float pre_x[3] = {0};
    float t;
    float u[2][1];
    float x_der[3][1] = {0};
    float y[3][1] = {0};

    u_t current_u = get_u_t(0);
    u[0][0] = current_u.linear_spd;
    u[1][0] = current_u.ang_spd;

    
    // imprimir x(t = 0) = 0 e colocar no arquivo
    // chamar a funcao da derivada de x
    derivate(u, x, x_der);
    // chamar a funcao de y 
    result_y(x, y);

    for(t = 0.01; t < 20; t= t + 0.01){
        
        u_t current_u = get_u_t(t);
        u[0][0] = current_u.linear_spd;
        u[1][0] = current_u.ang_spd;

        // calcula o x
        next_x(x, pre_x, x_der);

        // calcula x' 
        derivate(u, x, x_der);

        // calcula y
        result_y(x, y);
        

        write_file(t, y, u);
    }

    fclose(file);

    return 0;
}