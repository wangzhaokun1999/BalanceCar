#include <Arduino.h>
#include "IMU.h"
#include "Encodeur.h"
#include "PWM.h"
#include "PID.h"
#include "HyperTerminal.h"

IMU imu;
Encodeur encodeur;
PWM pwm;
PID pid(imu, encodeur, pwm);
HyperTerminal hyperTerminal(pid, Serial);

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
    pid.startTask();

    hyperTerminal.begin();
}

void loop()
{
    hyperTerminal.update();
}