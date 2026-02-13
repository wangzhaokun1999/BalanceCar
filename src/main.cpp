#include <Arduino.h>

// ================= 编码器引脚 =================
#define PULSE_L 35
#define DIR_L   34
#define PULSE_R 36
#define DIR_R   39

// ================= PWM 引脚 =================
#define G_IN1  17
#define G_IN2  18
#define D_IN1  16
#define D_IN2  4

#define CH_G1  0
#define CH_G2  1
#define CH_D1  2
#define CH_D2  3

// ================= 方向符号补偿 =================
// 定义：正PWM = 小车向前
#define LEFT_SIGN   -1 
#define RIGHT_SIGN  1   

// ================= 参数 =================
#define PPR_TOTAL 12
#define SAMPLE_TIME 100
#define WHEEL_DIAMETER 0.065

// ================= 全局变量 =================
volatile long countLeft = 0;
volatile long countRight = 0;

// ================= 编码器中断 =================
void IRAM_ATTR ISR_Left() {
    if (digitalRead(DIR_L))
        countLeft++;
    else
        countLeft--;
}

void IRAM_ATTR ISR_Right() {
    if (digitalRead(DIR_R))
        countRight--;
    else
        countRight++;
}

// ================= 底层电机驱动（纯PWM） =================
void driveLeftRaw(int duty) {

    if (duty >= 0) {
        ledcWrite(CH_G1, duty);
        ledcWrite(CH_G2, 0);
    } else {
        ledcWrite(CH_G1, 0);
        ledcWrite(CH_G2, -duty);
    }
}

void driveRightRaw(int duty) {

    if (duty >= 0) {
        ledcWrite(CH_D1, duty);
        ledcWrite(CH_D2, 0);
    } else {
        ledcWrite(CH_D1, 0);
        ledcWrite(CH_D2, -duty);
    }
}

// ================= 对外统一接口 =================
void setMotor(int left, int right) {

    // 方向补偿在这里统一处理
    left  = LEFT_SIGN  * left;
    right = RIGHT_SIGN * right;

    left  = constrain(left,  -255, 255);
    right = constrain(right, -255, 255);

    driveLeftRaw(left);
    driveRightRaw(right);
}

void motorStop() {
    setMotor(0, 0);
}

// ================= 初始化 =================
void setup() {

    Serial.begin(115200);

    // 编码器
    pinMode(PULSE_L, INPUT);
    pinMode(DIR_L, INPUT);
    pinMode(PULSE_R, INPUT);
    pinMode(DIR_R, INPUT);

    attachInterrupt(digitalPinToInterrupt(PULSE_L), ISR_Left, RISING);
    attachInterrupt(digitalPinToInterrupt(PULSE_R), ISR_Right, RISING);

    // PWM
    ledcSetup(CH_G1, 1000, 8);
    ledcSetup(CH_G2, 1000, 8);
    ledcSetup(CH_D1, 1000, 8);
    ledcSetup(CH_D2, 1000, 8);

    ledcAttachPin(G_IN1, CH_G1);
    ledcAttachPin(G_IN2, CH_G2);
    ledcAttachPin(D_IN1, CH_D1);
    ledcAttachPin(D_IN2, CH_D2);
}

// ================= 主循环 =================
void loop() {

    static unsigned long lastMotorSwitch = 0;
    static int motorState = 0;

    static long lastL = 0;
    static long lastR = 0;

    unsigned long now = millis();

    // ===== 电机测试状态机 =====
    if (now - lastMotorSwitch > 3000) {
        lastMotorSwitch = now;
        motorState++;

        if (motorState > 3) motorState = 0;

        switch (motorState) {
            case 0:
                Serial.println("Forward");
                setMotor(180, 180);  // 统一前进
                break;

            case 1:
                Serial.println("Stop");
                motorStop();
                break;

            case 2:
                Serial.println("Backward");
                setMotor(-180, -180); // 统一后退
                break;

            case 3:
                Serial.println("Stop");
                motorStop();
                break;
        }
    }

    // ===== 速度计算 =====
    static unsigned long lastSpeedTime = 0;

    if (now - lastSpeedTime >= SAMPLE_TIME) {
        lastSpeedTime = now;

        noInterrupts();
        long nowL = countLeft;
        long nowR = countRight;
        interrupts();

        long deltaL = nowL - lastL;
        long deltaR = nowR - lastR;

        lastL = nowL;
        lastR = nowR;

        float rpmL = deltaL * 50.0;
        float rpmR = deltaR * 50.0;

        float wheelCirc = 3.1416 * WHEEL_DIAMETER;
        float speedL = (rpmL / 60.0) * wheelCirc;
        float speedR = (rpmR / 60.0) * wheelCirc;

        Serial.print("L_count: ");
        Serial.print(nowL);
        Serial.print("  RPM_L: ");
        Serial.print(rpmL);
        Serial.print("  V_L: ");
        Serial.print(speedL);

        Serial.print("   |   R_count: ");
        Serial.print(nowR);
        Serial.print("  RPM_R: ");
        Serial.print(rpmR);
        Serial.print("  V_R: ");
        Serial.println(speedR);
    }
}
