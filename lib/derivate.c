#include <math.h>
#include "calculus.h"

// calula seno e cos
// declara matriz
void derivate(const float u[2], const float x[3], float x_der[3]){

    float seno_matriz = sin(x[2]);
    float cos_matriz = cos(x[2]);

/*
    float matrix_x[3][2] = {
        {seno_matriz, 0},
        {cos_matriz, 0},
        {0, 1}
    };
*/

    x_der[0] = seno_matriz;
    x_der[1] = cos_matriz;
    x_der[2] = u[1];
}