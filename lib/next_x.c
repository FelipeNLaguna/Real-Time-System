#include "calculus.h"

void next_x(float x[3], const float pre_x[3], const float x_der[3], const float dt){
    int i;
    for(i = 0; i < 3; i++){
        x[i] =  pre_x[i] + dt * x_der[i];
    }
}