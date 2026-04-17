#include "IMU.h"
#include <Wire.h>

IMU::IMU()
: tetagr(0.0f),
  tetaF(0.0f),
  lastGyroZ(0.0f),
  Te_ms(5.0f),
  Tau_ms(1000.0f),
  alpha(0.0f),
  newData(false)
{}

bool IMU::begin()
{
    Wire.begin(21, 22);

    if (!mpu.begin())
        return false;

    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);

    alpha = Tau_ms / (Tau_ms + Te_ms);

    return true;
}

void IMU::startTask()
{
    xTaskCreate(
        taskWrapper,
        "IMU_Task",
        4096,
        this,
        5,
        NULL
    );
}

void IMU::taskWrapper(void *param)
{
    IMU *imu = static_cast<IMU*>(param);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        imu->update();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS((uint32_t)imu->Te_ms));
    }
}

void IMU::update()
{
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    tetagr = atan2(a.acceleration.x, a.acceleration.y);

    float dt = Te_ms / 1000.0f;

    lastGyroZ = g.gyro.z;
    float gyroAngle = tetaF + lastGyroZ * dt;

    tetaF = alpha * gyroAngle + (1.0f - alpha) * tetagr;

    newData = true;
}

float IMU::getAngle()
{
    return tetaF;
}

float IMU::getAccAngle()
{
    return tetagr;
}

float IMU::getGyroZ()
{
    return lastGyroZ;
}

float IMU::getTe()
{
    return Te_ms;
}

float IMU::getTau()
{
    return Tau_ms;
}

