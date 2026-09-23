#ifndef CALCULUS_H
#define CALCULUS_H

#include <stdio.h>

// vou declara como void ja que vamos resolver problemas em arrays
extern void derivate(const float u[2], const float x[3], float x_der[3]);
extern void result_y(const float x[3], float y[3]);
extern void get_u_t(const float t, float u[2]);
extern void write_file(const float t, const float y[3], const float u[2]);
extern void next_x(float x[3], const float pre_x[3], const float x_der[3], const float dt); 

extern FILE *file;

#endif
