#include <stdio.h>
#include "calculus.h"

#define PI 3.14159265358979323846
#define DT 0.01

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
    float t;
    float u[2];
    float x_der[3];
    float y[3];
    
    for(t = 0; t < 20; t= t + DT){
        
        get_u_t(t, u, PI);

        // calcula x' 
        derivate(u, x, x_der);

        // calcula y
        result_y(x, y);

        // calcula o x
        next_x(x, pre_x, x_der, DT);
        for(i = 0; i < 3; i++){
            pre_x[i] = x[i];
        }        

        write_file(t, y, u);
    }

    fclose(file);

    return 0;
}