#include "PID.h"

PID::PID(IMU& imuRef, Encodeur& encRef, PWM& pwmRef)
: imu(&imuRef),
  encodeur(&encRef),
  pwm(&pwmRef),
  Te(5.0f),
  Tau(1000.0f),
  Kp_theta(0.01f),
  Kd_theta(0.0f),
  Kp_speed(0.0f),
  Kd_speed(0.0f),
  theta_eq(1.57f),
  theta_max_deg(30.0f),
  C0_L(0.199f),
  C0_R(0.196f),
  ec_max(0.45f),
  theta(0.0f),
  gyro(0.0f),
  theta_corr(0.0f),
  theta_ref(0.0f),
  error_theta(0.0f),
  error_theta_deg(0.0f),
  gyro_deg(0.0f),
  pwm_value(0.0f),
  v_left(0.0f),
  v_right(0.0f),
  v_mean(0.0f),
  speed_error(0.0f),
  d_speed(0.0f),
  ec(0.0f),
  ec_corr_L(0.0f),
  ec_corr_R(0.0f),
  motor_cmd_L(0),
  motor_cmd_R(0),
  last_speed_error(0.0f),
  dbg_theta(0.0f),
  dbg_gyro(0.0f),
  dbg_speed(0.0f),
  dbg_u(0.0f)
{
}

void PID::begin()
{
    applyCompatibilityUpdate();
}

void PID::applyCompatibilityUpdate()
{
    imu->setTeMs(Te);
    imu->setTauMs(Tau);
}

float PID::ecCompensate(float ec, float C0)
{
    if (fabs(ec) < 0.0001f)
        return 0.0f;

    if (ec > 0.0f)
        return ec + C0;
    else
        return ec - C0;
}

int PID::pwmcalcul(float ecInput)
{
    ecInput = constrain(ecInput, -ec_max, ec_max);

    pwm_value = (ecInput / ec_max) * 1000.0f;
    pwm_value = constrain(pwm_value, -1000.0f, 1000.0f);

    return (int)pwm_value;
}

void PID::taskWrapper(void* param)
{
    PID* self = static_cast<PID*>(param);
    self->controlLoop();
}

void PID::startTask()
{
    xTaskCreate(
        taskWrapper,
        "control",
        4096,
        this,
        5,
        NULL
    );
}

void PID::controlLoop()
{
    TickType_t lastWake = xTaskGetTickCount();

    while (1)
    {
        theta = imu->getAngle();
        gyro  = imu->getGyroZ();

        encodeur->update();

        v_left  = encodeur->getSpeed_L();
        v_right = encodeur->getSpeed_R();
        v_mean  = 0.5f * (v_left + v_right);

        float theta_err_deg_abs = fabs(theta - theta_eq) * 180.0f / PI;
        if (theta_err_deg_abs > theta_max_deg)
        {
            pwm->stop();

            dbg_theta = theta;
            dbg_gyro  = gyro;
            dbg_speed = v_mean;
            dbg_u     = 0.0f;

            vTaskDelayUntil(&lastWake, pdMS_TO_TICKS((uint32_t)Te));
            continue;
        }

        float Te_s = Te / 1000.0f;

        speed_error = -v_mean;
        d_speed = (speed_error - last_speed_error) / Te_s;
        last_speed_error = speed_error;

        theta_corr =
            Kp_speed * speed_error +
            Kd_speed * d_speed;

        theta_corr = constrain(theta_corr,
                               -3.0f * DEG_TO_RAD,
                                3.0f * DEG_TO_RAD);

        theta_ref = theta_eq + theta_corr;
        error_theta = theta_ref - theta;

        error_theta_deg = error_theta * 180.0f / PI;
        gyro_deg        = gyro * 180.0f / PI;

        ec = Kp_theta * error_theta_deg
           - Kd_theta * gyro_deg;

        ec = constrain(ec, -ec_max, ec_max);

        ec_corr_L = ecCompensate(ec, C0_L);
        ec_corr_R = ecCompensate(ec, C0_R);

        motor_cmd_L = pwmcalcul(ec_corr_L);
        motor_cmd_R = pwmcalcul(ec_corr_R);

        pwm->setSpeedLR(-motor_cmd_L, -motor_cmd_R);

        dbg_theta = theta;
        dbg_gyro  = gyro;
        dbg_speed = v_mean;
        dbg_u     = 0.5f * (motor_cmd_L + motor_cmd_R);

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS((uint32_t)Te));
    }
}

void PID::setTe(float value)
{
    Te = constrain(value, 1.0f, 100.0f);
    applyCompatibilityUpdate();
}

void PID::setTau(float value)
{
    Tau = constrain(value, 1.0f, 10000.0f);
    applyCompatibilityUpdate();
}

void PID::setKpTheta(float value)   { Kp_theta = constrain(value, 0.0f, 0.5f); }
void PID::setKdTheta(float value)   { Kd_theta = constrain(value, 0.0f, 0.05f); }
void PID::setKpSpeed(float value)   { Kp_speed = constrain(value, 0.0f, 50.0f); }
void PID::setKdSpeed(float value)   { Kd_speed = constrain(value, 0.0f, 50.0f); }
void PID::setThetaEq(float value)   { theta_eq = value; }
void PID::setThetaMaxDeg(float value) { theta_max_deg = constrain(value, 1.0f, 60.0f); }
void PID::setC0L(float value)       { C0_L = constrain(value, 0.0f, 0.5f); }
void PID::setC0R(float value)       { C0_R = constrain(value, 0.0f, 0.5f); }

float PID::getTe() const       { return Te; }
float PID::getTau() const      { return Tau; }
float PID::getEc() const       { return ec; }

float PID::getDbgTheta() const { return dbg_theta; }
float PID::getDbgGyro() const  { return dbg_gyro; }
float PID::getDbgSpeed() const { return dbg_speed; }
float PID::getDbgU() const     { return dbg_u; }