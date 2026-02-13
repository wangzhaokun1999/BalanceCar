#include <Arduino.h>

// ================= 引脚定义 =================
#define PULSE_L 35
#define DIR_L   34

#define PULSE_R 36
#define DIR_R   39

// ================= 参数 =================
#define PPR_TOTAL 12          // 每圈 12 脉冲
#define SAMPLE_TIME 100       // 采样时间 ms
#define WHEEL_DIAMETER 0.065  // 轮径 65mm = 0.065m

// ================= 全局变量 =================
volatile long countLeft = 0;
volatile long countRight = 0;

// ================= 中断函数 =================
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

// ================= 初始化 =================
void setup() {
    Serial.begin(115200);

    pinMode(PULSE_L, INPUT);
    pinMode(DIR_L, INPUT);

    pinMode(PULSE_R, INPUT);
    pinMode(DIR_R, INPUT);

    attachInterrupt(digitalPinToInterrupt(PULSE_L), ISR_Left, RISING);
    attachInterrupt(digitalPinToInterrupt(PULSE_R), ISR_Right, RISING);
}

// ================= 主循环 =================
void loop() {

    static long lastL = 0;
    static long lastR = 0;

    // 关闭中断，防止读取时被修改
    noInterrupts();
    long nowL = countLeft;
    long nowR = countRight;
    interrupts();

    long deltaL = nowL - lastL;
    long deltaR = nowR - lastR;

    lastL = nowL;
    lastR = nowR;

    // ====== 计算 RPM ======
    // RPM = Δcount × (60 / (PPR × 采样时间))
    // 采样时间 0.2s -> RPM = Δcount × 25
    float rpmL = deltaL * 25.0;
    float rpmR = deltaR * 25.0;

    // ====== 计算线速度 m/s ======
    float wheelCircumference = 3.1416 * WHEEL_DIAMETER;
    float speedL = (rpmL / 60.0) * wheelCircumference;
    float speedR = (rpmR / 60.0) * wheelCircumference;

    // ====== 串口输出 ======
    Serial.print("L_count: ");
    Serial.print(nowL);
    Serial.print("  RPM_L: ");
    Serial.print(rpmL);
    Serial.print("  V_L(m/s): ");
    Serial.print(speedL);

    Serial.print("   |   R_count: ");
    Serial.print(nowR);
    Serial.print("  RPM_R: ");
    Serial.print(rpmR);
    Serial.print("  V_R(m/s): ");
    Serial.println(speedR);

    delay(SAMPLE_TIME);
}
