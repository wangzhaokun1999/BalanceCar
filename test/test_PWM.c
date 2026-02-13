#include <Arduino.h>

//  GPIO setup according to the schematic
#define G_IN1  17
#define G_IN2  18
#define D_IN1  16
#define D_IN2  04

#define CH_G1  0
#define CH_G2  1
#define CH_D1  2
#define CH_D2  3

void setup()
{
    Serial.begin(115200);

    ledcSetup(CH_G1, 1000, 8);
    ledcSetup(CH_G2, 1000, 8);
    ledcSetup(CH_D1, 1000, 8);
    ledcSetup(CH_D2, 1000, 8);

    ledcAttachPin(G_IN1, CH_G1);
    ledcAttachPin(G_IN2, CH_G2);
    ledcAttachPin(D_IN1, CH_D1);
    ledcAttachPin(D_IN2, CH_D2);
}

void loop() {

    // 正转
    ledcWrite(CH_G1, 180);
    ledcWrite(CH_G2, 0);
    ledcWrite(CH_D1, 180);
    ledcWrite(CH_D2, 0);
    delay(3000);

    // 停止（防止电流冲击）
    ledcWrite(CH_G1, 0);
    ledcWrite(CH_G2, 0);
    ledcWrite(CH_D1, 0);
    ledcWrite(CH_D2, 0);
    delay(1000);

    // 反转
    ledcWrite(CH_G1, 0);
    ledcWrite(CH_G2, 180);
    ledcWrite(CH_D1, 0);
    ledcWrite(CH_D2, 180);
    delay(3000);

    // 停止
    ledcWrite(CH_G1, 0);
    ledcWrite(CH_G2, 0);
    ledcWrite(CH_D1, 0);
    ledcWrite(CH_D2, 0);
    delay(1000);
}
