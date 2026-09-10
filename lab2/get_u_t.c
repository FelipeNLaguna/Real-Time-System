#include <stdio.h>
#include "calculus.h"


void get_u_t(const float t, float u[2], const float pi){
    if(t < 0){
        u[0] = 0;
        u[1] = 0;
    }
    else if( (t >= 0) && (t < 10) ){
        u[0] = 1;
        u[1] = 0.2 * pi;
    }
    else{
        u[0] = 1;
        u[1] = -0.2 * pi;
    }
    
}