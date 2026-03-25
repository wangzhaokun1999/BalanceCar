#ifndef HYPERTERMINAL_H
#define HYPERTERMINAL_H

#include <Arduino.h>
#include "PID.h"

class HyperTerminal
{
private:
    PID* pid;
    HardwareSerial* serial;
    String buffer;

    float disp_theta_gain;
    float disp_gyro_gain;
    float disp_speed_gain;
    float disp_u_gain;

    void inputChar(char ch);
    void processCommand(const String& line);
    void sendPlotData();

public:
    HyperTerminal(PID& pidRef, HardwareSerial& serialRef = Serial);

    void begin();
    void update();

    void setThetaGain(float value);
    void setGyroGain(float value);
    void setSpeedGain(float value);
    void setUGain(float value);
};

#endif