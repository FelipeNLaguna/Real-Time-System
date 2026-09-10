#include "calculus.h"
#define DT 0.01

void next_x(float *x, float *pre_x, float x_der[3][1]){
    int i;
    for(i = 0; i < 3; i++){
        x[i] =  pre_x[i] + DT * x_der[i][0];
        pre_x[i] = x[i];
    }
}