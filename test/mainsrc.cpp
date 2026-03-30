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
// Web variables (required by webserver.cpp)
// =====================================================
volatile float web_angle = 0.0;
volatile float web_speed = 0.0;
volatile int joy_x = 0;
volatile int joy_y = 0;

// =====================================================
// Global tuning parameters (required by webserver.cpp)
// These ranges match your latest webserver.cpp constraints
// =====================================================
float Te = 5.0f;              // ms
float Tau = 1000.0f;          // ms

float Kp_theta = 0.01f;       // 0 ~ 1
float Kd_theta = 0.0f;        // 0 ~ 1

float Kp_speed = 0.0f;        // 0 ~ 50
float Kd_speed = 0.0f;        // 0 ~ 50

float theta_eq = 1.56f;
float theta_max_deg = 30.0f;  // 1 ~ 60

float C0_L = 0.22f;           // 0 ~ 0.5
float C0_R = 0.22f;           // 0 ~ 0.5

float ec_max = 0.45f;         // 0.01 ~ 1.0

// =====================================================
// Push globals -> PID
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
// =====================================================
static void syncPIDToGlobals()
{
    Te            = pid.getTe();
    Tau           = pid.getTau();

    C0_L          = pid.getC0L();
    C0_R          = pid.getC0R();

    ec_max        = pid.getEcMax();

}


void applyCompatibilityUpdate()
{
    imu.setTeMs(Te);
    imu.setTauMs(Tau);

    pid.setTe(Te);
    pid.setTau(Tau);
}

// Setup

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

    // push initial globals into PID before starting control task
    syncGlobalsToPID();

    pid.startTask();

    // Start SoftAP
    wifi_init_softap();

    // Start WebServer
    webserver_begin(&joy_x, 
                    &joy_y, 
                    &web_angle, 
                    &web_speed,
                    syncGlobalsToPID,
                    syncPIDToGlobals
                    );

    Serial.println("System ready");
}

// Loop

void loop()
{
    // Init web data to PID
    syncGlobalsToPID();

    web_angle = imu.getAngle();
    web_speed = encodeur.getSpeed();
    // Save data in web
    syncPIDToGlobals();

    delay(20);
}