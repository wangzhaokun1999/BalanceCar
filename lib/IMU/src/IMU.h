#ifndef IMU_H
#define IMU_H

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

class IMU {
public:
    IMU();

    bool begin();
    void startTask();

    float getAngle();
    float getAccAngle();
    float getGyroZ();
    float getTe();
    float getTau();

private:
    static void taskWrapper(void *param);
    void update();
    
    Adafruit_MPU6050 mpu;

    float tetagr;
    float tetaF;
    float lastGyroZ;

    float Te_ms;
    float Tau_ms;
    float alpha;

    volatile bool newData;
};

#endif