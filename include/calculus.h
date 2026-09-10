#ifndef CALCULUS_H
#define CALCULUS_H

#include <stdio.h>

typedef struct {
    float linear_spd;
    float ang_spd;
}u_t;

// vou declara como void ja que vamos resolver problemas em arrays
extern void derivate(float u[2][1], float *x, float x_der[3][1]);
extern void result_y(float *x, float y[3][1]);
extern u_t get_u_t(float t);
extern void write_file(float t, float y[3][1], float u[2][1]);
extern void next_x(float *x, float *pre_x, float x_der[3][1]); 

extern FILE *file;

#endif
