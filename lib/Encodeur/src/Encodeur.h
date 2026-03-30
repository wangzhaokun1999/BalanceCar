#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Encodeur {
public:
    Encodeur();

    void begin();
    void update();

    float getRPM_L();
    float getRPM_R();

    float getSpeed_L();
    float getSpeed_R();
    float getSpeed();
private:
    static void IRAM_ATTR isrLeft();
    static void IRAM_ATTR isrRight();

    static volatile long countLeft;
    static volatile long countRight;

    long lastL;
    long lastR;

    float rpmL;
    float rpmR;

    float speedL;
    float speedR;

    static constexpr int PULSE_L = 35;
    static constexpr int DIR_L   = 34;
    static constexpr int PULSE_R = 36;
    static constexpr int DIR_R   = 39;

    static constexpr float WHEEL_DIAMETER = 0.065;
    static constexpr int SAMPLE_TIME = 100;
};

#endif
