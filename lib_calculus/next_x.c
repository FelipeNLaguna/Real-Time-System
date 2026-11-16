#include "calculus.h"

void next_x(double x[3], const double pre_x[3], const double x_der[3], const double dt){
    int i;
    for(i = 0; i < 3; i++){
        x[i] =  pre_x[i] + dt * x_der[i];
    }
}