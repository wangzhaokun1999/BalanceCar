#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>

struct WebParams
{
    float Te;
    float Tau;
    float Kp_theta;
    float Kd_theta;
    float Kp_speed;
    float Kd_speed;
    float theta_eq;
    float theta_max_deg;
    float C0_L;
    float C0_R;
    float ec_max;
};

typedef void (*WebApplyParamFn)(const char *name, float value);
typedef void (*WebReadParamsFn)(WebParams *p);

void webserver_begin(
    volatile int *joy_x,
    volatile int *joy_y,
    volatile float *angle_ptr,
    volatile float *speed_ptr,
    WebApplyParamFn apply_fn,
    WebReadParamsFn read_fn
);

#endif