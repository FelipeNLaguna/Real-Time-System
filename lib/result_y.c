#include "calculus.h"

int matrix_y[3][3] = {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1}
};

void result_y(float *x, float y[3][1]){
    int i;

    for (int i = 0; i < 3; i++) {
        y[i][0] = 0;
    }

    for(i = 0; i < 3; i++){
       y[i][0] = x[i];
    }

}
