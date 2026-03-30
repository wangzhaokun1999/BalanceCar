#include "PWM.h"

// ================= Constructor =================
PWM::PWM()
{
    gainLeft = 1.0f;
    gainRight = 1.0f;
}

// ================= Init =================
void PWM::begin()
{
    ledcSetup(CH_G1, 20000, PWM_RESOLUTION);
    ledcSetup(CH_G2, 20000, PWM_RESOLUTION);
    ledcSetup(CH_D1, 20000, PWM_RESOLUTION);
    ledcSetup(CH_D2, 20000, PWM_RESOLUTION);

    ledcAttachPin(G_IN1, CH_G1);
    ledcAttachPin(G_IN2, CH_G2);
    ledcAttachPin(D_IN1, CH_D1);
    ledcAttachPin(D_IN2, CH_D2);
}

// ================= Symmetric drive =================
void PWM::driveLeft(int alpha)
{
    alpha = constrain(alpha, -ALPHA_MAX, ALPHA_MAX);

    int in1 = PWM_CENTER - alpha;
    int in2 = PWM_CENTER + alpha;

    ledcWrite(CH_G1, in1);
    ledcWrite(CH_G2, in2);
}

void PWM::driveRight(int alpha)
{
    alpha = constrain(alpha, -ALPHA_MAX, ALPHA_MAX);

    int in1 = PWM_CENTER + alpha;
    int in2 = PWM_CENTER - alpha;

    ledcWrite(CH_D1, in1);
    ledcWrite(CH_D2, in2);
}

// ================= Gain setters =================
void PWM::setGainLeft(float g)
{
    gainLeft = constrain(g, 0.5, 1.5);
}

void PWM::setGainRight(float g)
{
    gainRight = constrain(g, 0.5, 1.5);
}

float PWM::getGainLeft()
{
    return gainLeft;
}

float PWM::getGainRight()
{
    return gainRight;
}

// ================= Outside interface =================
void PWM::setSpeed(int speed)
{
    setSpeedLR(speed, speed);
}

void PWM::setSpeedLR(int speedL, int speedR)
{
    speedL = constrain(speedL, -1000, 1000);
    speedR = constrain(speedR, -1000, 1000);

    int cmdL = (int)(speedL * gainLeft);
    int cmdR = (int)(speedR * gainRight);

    cmdL = constrain(cmdL, -1000, 1000);
    cmdR = constrain(cmdR, -1000, 1000);

    int alpha_l = map(cmdL,
                      -1000, 1000,
                      -ALPHA_MAX, ALPHA_MAX);

    int alpha_r = map(cmdR,
                      -1000, 1000,
                      -ALPHA_MAX, ALPHA_MAX);

    driveLeft(alpha_l);
    driveRight(alpha_r);
}

void PWM::stop()
{
    driveLeft(0);
    driveRight(0);
}