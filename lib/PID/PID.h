#ifndef PID_H
#define PID_H

#include <Arduino.h>
#include "IMU.h"
#include "Encodeur.h"
#include "PWM.h"

class PID
{
private:
    IMU* imu;
    Encodeur* encodeur;
    PWM* pwm;

    // parameters
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

    // runtime variables
    float theta;
    float gyro;
    float theta_corr;
    float theta_ref;
    float error_theta;
    float error_theta_deg;
    float gyro_deg;

    float pwm_value;
    float v_left;
    float v_right;
    float v_mean;
    float speed_error;
    float d_speed;

    float ec;
    float ec_corr_L;
    float ec_corr_R;

    int motor_cmd_L;
    int motor_cmd_R;

    float last_speed_error;

    // manual test mode
    bool manual_test_enable;
    float manual_test_ec_L;
    float manual_test_ec_R;

    // debug
    volatile float dbg_theta;
    volatile float dbg_gyro;
    volatile float dbg_speed;
    volatile float dbg_u;

    static void taskWrapper(void* param);
    void controlLoop();

    void applyCompatibilityUpdate();
    float ecCompensate(float ecValue, float C0);
    int pwmcalcul(float ecInput);

public:
    PID(IMU& imuRef, Encodeur& encRef, PWM& pwmRef);

    void begin();
    void startTask();

    // setters
    void setTe(float value);
    void setTau(float value);
    void setKpTheta(float value);
    void setKdTheta(float value);
    void setKpSpeed(float value);
    void setKdSpeed(float value);
    void setThetaEq(float value);
    void setThetaMaxDeg(float value);
    void setC0L(float value);
    void setC0R(float value);
    void setEcMax(float value);

    void setManualTestEnable(bool value);
    void setManualTestEcL(float value);
    void setManualTestEcR(float value);

    // getters
    float getTe() const;
    float getTau() const;

    float getKpTheta() const;
    float getKdTheta() const;
    float getKpSpeed() const;
    float getKdSpeed() const;

    float getThetaEq() const;
    float getThetaMaxDeg() const;

    float getEc() const;
    float getEcMax() const;
    float getC0L() const;
    float getC0R() const;

    bool getManualTestEnable() const;
    float getManualTestEcL() const;
    float getManualTestEcR() const;

    float getDbgTheta() const;
    float getDbgGyro() const;
    float getDbgSpeed() const;
    float getDbgU() const;

    float getLeftSpeed() const;
    float getRightSpeed() const;

    // realtime values for web
    float getAngle() const;
    float getGyro() const;
    float getSpeed() const;

    Encodeur* getEncodeur();
    PWM* getPWM();
};

#endif