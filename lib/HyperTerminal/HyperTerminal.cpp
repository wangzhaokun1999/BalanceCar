#include "HyperTerminal.h"

HyperTerminal::HyperTerminal(PID& pidRef, HardwareSerial& serialRef)
: pid(&pidRef),
  serial(&serialRef),
  buffer(""),
  disp_theta_gain(0.5f),
  disp_gyro_gain(0.5f),
  disp_speed_gain(40.0f),
  disp_u_gain(0.2f)
{
}

void HyperTerminal::begin()
{
}

void HyperTerminal::setThetaGain(float value) { disp_theta_gain = value; }
void HyperTerminal::setGyroGain(float value)  { disp_gyro_gain  = value; }
void HyperTerminal::setSpeedGain(float value) { disp_speed_gain = value; }
void HyperTerminal::setUGain(float value)     { disp_u_gain     = value; }

void HyperTerminal::processCommand(const String& line)
{
    int spaceIndex = line.indexOf(' ');
    if (spaceIndex == -1)
        return;

    String cmd   = line.substring(0, spaceIndex);
    String value = line.substring(spaceIndex + 1);
    float v = value.toFloat();

    if (cmd == "Tau")
    {
        pid->setTau(v);
    }
    else if (cmd == "Te")
    {
        pid->setTe(v);
    }
    else if (cmd == "KpT")
    {
        pid->setKpTheta(v);
    }
    else if (cmd == "KdT")
    {
        pid->setKdTheta(v);
    }
    else if (cmd == "KpS")
    {
        pid->setKpSpeed(v);
    }
    else if (cmd == "KdS")
    {
        pid->setKdSpeed(v);
    }
    else if (cmd == "theta")
    {
        pid->setThetaEq(v);
    }
    else if (cmd == "Tmax")
    {
        pid->setThetaMaxDeg(v);
    }
    else if (cmd == "C0L")
    {
        pid->setC0L(v);
    }
    else if (cmd == "C0R")
    {
        pid->setC0R(v);
    }
    else if (cmd == "gainTheta")
    {
        disp_theta_gain = v;
    }
    else if (cmd == "gainGyro")
    {
        disp_gyro_gain = v;
    }
    else if (cmd == "gainSpeed")
    {
        disp_speed_gain = v;
    }
    else if (cmd == "gainU")
    {
        disp_u_gain = v;
    }
}

void HyperTerminal::inputChar(char ch)
{
    if (ch == '\n' || ch == '\r')
    {
        if (buffer.length() > 0)
        {
            processCommand(buffer);
            buffer = "";
        }
    }
    else
    {
        buffer += ch;
        if (buffer.length() > 64)
            buffer = "";
    }
}

void HyperTerminal::sendPlotData()
{
    float theta_plot = pid->getDbgTheta() * 180.0f / PI * disp_theta_gain;
    float gyro_plot  = pid->getDbgGyro()  * 180.0f / PI * disp_gyro_gain;
    float speed_plot = pid->getDbgSpeed() * disp_speed_gain;
    float u_plot     = pid->getDbgU()     * disp_u_gain;

    serial->print(theta_plot, 6);
    serial->print(' ');
    serial->print(gyro_plot, 6);
    serial->print(' ');
    serial->print(speed_plot, 6);
    serial->print(' ');
    serial->println(pid->getEc(), 6);
}
void HyperTerminal::update()
{
    while (serial->available() > 0)
    {
        inputChar((char)serial->read());
    }

    sendPlotData();
    delay((uint32_t)pid->getTe());
}