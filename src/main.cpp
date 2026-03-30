#include <Arduino.h>
#include "IMU.h"
#include "Encodeur.h"
#include "PWM.h"
#include "PID.h"
#include "wifi_ap.h"
#include "webserver.h"

// =====================================================
// Modules
// =====================================================
IMU imu;
Encodeur encodeur;
PWM pwm;
PID pid(imu, encodeur, pwm);

// =====================================================
// Web realtime variables
// =====================================================
volatile float web_angle = 0.0f;
volatile float web_speed = 0.0f;
volatile int joy_x = 0;
volatile int joy_y = 0;

// =====================================================
// Global tuning parameters
// IMPORTANT: ranges must match webserver.cpp
// =====================================================
float Te = 5.0f;
float Tau = 1000.0f;

float Kp_theta = 0.01f;
float Kd_theta = 0.0f;

float Kp_speed = 0.0f;
float Kd_speed = 0.0f;

float theta_eq = 1.56f;
float theta_max_deg = 30.0f;

float C0_L = 0.22f;
float C0_R = 0.22f;

float ec_max = 0.45f;

// =====================================================
// Push globals -> PID
// Rename these if your PID method names differ
// =====================================================
static void syncGlobalsToPID()
{
    pid.setTe(Te);
    pid.setTau(Tau);

    pid.setKpTheta(Kp_theta);
    pid.setKdTheta(Kd_theta);

    pid.setKpSpeed(Kp_speed);
    pid.setKdSpeed(Kd_speed);

    pid.setThetaEq(theta_eq);
    pid.setThetaMaxDeg(theta_max_deg);

    pid.setC0L(C0_L);
    pid.setC0R(C0_R);

    pid.setEcMax(ec_max);
}

// =====================================================
// Pull PID -> globals
// Rename these if your PID method names differ
// =====================================================
static void syncPIDToGlobals()
{
    Te            = pid.getTe();
    Tau           = pid.getTau();

    Kp_theta      = pid.getKpTheta();
    Kd_theta      = pid.getKdTheta();

    Kp_speed      = pid.getKpSpeed();
    Kd_speed      = pid.getKdSpeed();

    theta_eq      = pid.getThetaEq();
    theta_max_deg = pid.getThetaMaxDeg();

    C0_L          = pid.getC0L();
    C0_R          = pid.getC0R();

    ec_max        = pid.getEcMax();
}

// =====================================================
// Required by webserver.cpp
// =====================================================
void applyCompatibilityUpdate()
{
    imu.setTeMs(Te);
    imu.setTauMs(Tau);

    pid.setTe(Te);
    pid.setTau(Tau);
}

// =====================================================
// Callback: web writes one parameter
// =====================================================
static void web_apply_param(const char *name, float value)
{
    if (strcmp(name, "Te") == 0)
    {
        Te = constrain(value, 1.0f, 100.0f);
        applyCompatibilityUpdate();
    }
    else if (strcmp(name, "Tau") == 0)
    {
        Tau = constrain(value, 1.0f, 10000.0f);
        applyCompatibilityUpdate();
    }
    else if (strcmp(name, "KpT") == 0)
    {
        Kp_theta = constrain(value, 0.0f, 1.0f);
    }
    else if (strcmp(name, "KdT") == 0)
    {
        Kd_theta = constrain(value, 0.0f, 1.0f);
    }
    else if (strcmp(name, "KpS") == 0)
    {
        Kp_speed = constrain(value, 0.0f, 50.0f);
    }
    else if (strcmp(name, "KdS") == 0)
    {
        Kd_speed = constrain(value, 0.0f, 50.0f);
    }
    else if (strcmp(name, "theta") == 0)
    {
        theta_eq = value;
    }
    else if (strcmp(name, "Tmax") == 0)
    {
        theta_max_deg = constrain(value, 1.0f, 60.0f);
    }
    else if (strcmp(name, "C0L") == 0)
    {
        C0_L = constrain(value, 0.0f, 0.5f);
    }
    else if (strcmp(name, "C0R") == 0)
    {
        C0_R = constrain(value, 0.0f, 0.5f);
    }
    else if (strcmp(name, "ECmax") == 0)
    {
        ec_max = constrain(value, 0.01f, 1.0f);
    }

    syncGlobalsToPID();
}

// =====================================================
// Callback: web reads all parameters
// =====================================================
static void web_read_params(WebParams *p)
{
    syncPIDToGlobals();

    p->Te            = Te;
    p->Tau           = Tau;
    p->Kp_theta      = Kp_theta;
    p->Kd_theta      = Kd_theta;
    p->Kp_speed      = Kp_speed;
    p->Kd_speed      = Kd_speed;
    p->theta_eq      = theta_eq;
    p->theta_max_deg = theta_max_deg;
    p->C0_L          = C0_L;
    p->C0_R          = C0_R;
    p->ec_max        = ec_max;
}

// =====================================================
// Setup
// =====================================================
void setup()
{
    Serial.begin(115200);
    delay(1000);

    if (!imu.begin())
    {
        while (1)
        {
            delay(10);
        }
    }

    imu.startTask();
    encodeur.begin();
    pwm.begin();

    pid.begin();
    syncGlobalsToPID();
    pid.startTask();

    wifi_init_softap();

    webserver_begin(
        &joy_x,
        &joy_y,
        &web_angle,
        &web_speed,
        web_apply_param,
        web_read_params
    );

    Serial.println("System ready");
}

// =====================================================
// Loop
// =====================================================
void loop()
{
    web_angle = imu.getAngle();
    web_speed = encodeur.getSpeed();

    delay(20);
}