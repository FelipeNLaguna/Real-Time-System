#include <stdio.h>
#include "calculus.h"
#define PI 3.14159265358979323846

u_t get_u_t(float t){
    u_t matrix_u;
    if(t < 0){
        matrix_u.linear_spd = 0;
        matrix_u.ang_spd = 0;
    }
    else if( (t >= 0) && (t < 10) ){
        matrix_u.linear_spd = 1;
        matrix_u.ang_spd = 0.2 * PI;
    }
    else{
        matrix_u.linear_spd = 1;
        matrix_u.ang_spd = -0.2 * PI;
    }
    
    return matrix_u;
}