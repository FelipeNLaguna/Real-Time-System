#include <math.h>
#include "calculus.h"

// calula seno e cos
// declara matriz
void derivate(float u[2][1], float *x, float x_der[3][1]){

    float seno_matriz = sin(x[2]);
    float cos_matriz = cos(x[2]);

    float matrix_x[3][2] = {
        {seno_matriz, 0},
        {cos_matriz, 0},
        {0, 1}
    };

    for (int i = 0; i < 3; i++) {
        x_der[i][0] = 0;
    }

    // multiplicacao de matrizes
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 1; j++){
            for(int k = 0; k < 2; k++){
                x_der[i][j] += matrix_x[i][k] * u[k][j];
            }
        }
    }
}