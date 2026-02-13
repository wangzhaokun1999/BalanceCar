#include "SuperCar.h"
#include <Arduino.h>

/* =========================================================
 * Mini_L298N 引脚定义（来自你的原理图）
 * PWM 直接作用在 IN 引脚上
 * ========================================================= */

// 电机 A（G）
#define MA_IN1  32   // G-in
#define MA_IN2  33   // G+in

// 电机 B（D）
#define MB_IN1  34   // D+in
#define MB_IN2  35   // D-in

/* ===================== PWM 参数 ===================== */
#define PWM_FREQ 1000
#define PWM_RES  8          // 0~255
#define PWM_MAX  255

// 每个 IN 引脚一个 PWM 通道
#define CH_MA_IN1 0
#define CH_MA_IN2 1
#define CH_MB_IN1 2
#define CH_MB_IN2 3

CarControl_t carCTRL;

/* =========================================================
 * Mini_L298N 电机驱动函数
 * 正转：IN1 PWM，IN2 = 0
 * 反转：IN1 = 0，IN2 PWM
 * ========================================================= */
static void setMotor(int ch_in1, int ch_in2, float pwm)
{
    pwm = constrain(pwm, -PWM_MAX, PWM_MAX);

    if (pwm > 0)
    {
        ledcWrite(ch_in1, (int)pwm);
        ledcWrite(ch_in2, 0);
    }
    else if (pwm < 0)
    {
        ledcWrite(ch_in1, 0);
        ledcWrite(ch_in2, (int)(-pwm));
    }
    else
    {
        ledcWrite(ch_in1, 0);
        ledcWrite(ch_in2, 0);
    }
}

/* =========================================================
 * 初始化
 * ========================================================= */
void SuperCar::startTask()
{
    Serial.begin(115200);

    // GPIO 模式
    pinMode(MA_IN1, OUTPUT);
    pinMode(MA_IN2, OUTPUT);
    pinMode(MB_IN1, OUTPUT);
    pinMode(MB_IN2, OUTPUT);

    // PWM 通道配置
    ledcSetup(CH_MA_IN1, PWM_FREQ, PWM_RES);
    ledcSetup(CH_MA_IN2, PWM_FREQ, PWM_RES);
    ledcSetup(CH_MB_IN1, PWM_FREQ, PWM_RES);
    ledcSetup(CH_MB_IN2, PWM_FREQ, PWM_RES);

    // PWM 引脚绑定
    ledcAttachPin(MA_IN1, CH_MA_IN1);
    ledcAttachPin(MA_IN2, CH_MA_IN2);
    ledcAttachPin(MB_IN1, CH_MB_IN1);
    ledcAttachPin(MB_IN2, CH_MB_IN2);

    // 初始停止
    setMotor(CH_MA_IN1, CH_MA_IN2, 0);
    setMotor(CH_MB_IN1, CH_MB_IN2, 0);

    Serial.println("Mini_L298N motor init OK");
}

/* =========================================================
 * 主循环：基础 PWM 测试
 * ========================================================= */
void SuperCar::running()
{
    // 正转
    setMotor(CH_MA_IN1, CH_MA_IN2, 120);
    setMotor(CH_MB_IN1, CH_MB_IN2, 120);
    delay(2000);

    // 反转
    setMotor(CH_MA_IN1, CH_MA_IN2, -120);
    setMotor(CH_MB_IN1, CH_MB_IN2, -120);
    delay(2000);

    // 停止
    setMotor(CH_MA_IN1, CH_MA_IN2, 0);
    setMotor(CH_MB_IN1, CH_MB_IN2, 0);
    delay(2000);
}

SuperCar BalanceCar;
