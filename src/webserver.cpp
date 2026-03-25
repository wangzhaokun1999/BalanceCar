#include "webserver.h"
#include <Arduino.h>
#include "esp_http_server.h"
#include <stdlib.h>
#include <stdio.h>

/* ========= 外部变量 ========= */
extern volatile float web_angle;
extern volatile float web_speed;

/* ========= 你的主程序中的参数 ========= */
extern float Te;
extern float Tau;
extern float Kp_theta;
extern float Kd_theta;
extern float Kp_speed;
extern float Kd_speed;
extern float theta_eq;
extern float theta_max_deg;
extern float C0_L;
extern float C0_R;
extern float ec_max;

/* ========= 你的主程序里的函数 ========= */
extern void applyCompatibilityUpdate();

static volatile int *joy_x_ptr = nullptr;
static volatile int *joy_y_ptr = nullptr;

/* =========================================================
   操控页面
   ========================================================= */
static const char control_page[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 BalanceCar - Control</title>
<style>
body{
    font-family:Arial, sans-serif;
    background:#1e1e2f;
    color:white;
    margin:0;
    padding:0;
}
h1,h2,h3{
    text-align:center;
}
.container{
    display:flex;
    flex-direction:column;
    align-items:center;
    padding:15px;
}
.row{
    display:flex;
    flex-wrap:wrap;
    justify-content:center;
    gap:20px;
}
.card{
    background:#2c2c3e;
    padding:15px;
    border-radius:12px;
    box-shadow:0 2px 8px rgba(0,0,0,0.25);
}
button, .navbtn{
    background:#4CAF50;
    color:white;
    border:none;
    border-radius:8px;
    padding:10px 16px;
    font-size:16px;
    cursor:pointer;
    text-decoration:none;
    display:inline-block;
}
button:hover, .navbtn:hover{
    opacity:0.9;
}
#joystick{
    width:220px;
    height:220px;
    background:#444;
    border-radius:50%;
    position:relative;
    touch-action:none;
    user-select:none;
}
#stick{
    width:80px;
    height:80px;
    background:#4CAF50;
    border-radius:50%;
    position:absolute;
    left:70px;
    top:70px;
}
.data{
    font-size:20px;
    line-height:1.8;
    min-width:220px;
}
.topbar{
    display:flex;
    justify-content:center;
    margin-bottom:15px;
}
</style>
</head>
<body>

<h1>ESP32 BalanceCar</h1>

<div class="container">
    <div class="topbar">
        <a class="navbtn" href="/tuning">进入在线调参界面</a>
    </div>

    <div class="row">
        <div class="card">
            <h3>Joystick</h3>
            <div id="joystick">
                <div id="stick"></div>
            </div>
        </div>

        <div class="card data">
            <h3>实时数据</h3>
            Angle: <span id="angle">0</span><br>
            Speed: <span id="speed">0</span><br>
            Theta error: <span id="thetaerr">0</span>
        </div>
    </div>
</div>

<script>
let joy = document.getElementById('joystick');
let stick = document.getElementById('stick');

function clamp(v, min, max){
    return Math.max(min, Math.min(max, v));
}

function sendJoy(x, y){
    fetch('/joy?x=' + Math.round(x) + '&y=' + Math.round(y))
    .catch(err => console.log(err));
}

function setStick(x, y){
    stick.style.left = (x + 70) + 'px';
    stick.style.top  = (y + 70) + 'px';
}

function handlePointer(clientX, clientY){
    let rect = joy.getBoundingClientRect();
    let x = clientX - rect.left - 110;
    let y = clientY - rect.top  - 110;

    x = clamp(x, -85, 85);
    y = clamp(y, -85, 85);

    setStick(x, y);
    sendJoy(x, y);
}

joy.addEventListener('touchmove', function(e){
    e.preventDefault();
    handlePointer(e.touches[0].clientX, e.touches[0].clientY);
});

joy.addEventListener('touchend', function(){
    setStick(0, 0);
    sendJoy(0, 0);
});

joy.addEventListener('mousedown', function(e){
    handlePointer(e.clientX, e.clientY);

    function moveHandler(ev){
        handlePointer(ev.clientX, ev.clientY);
    }

    function upHandler(){
        setStick(0, 0);
        sendJoy(0, 0);
        document.removeEventListener('mousemove', moveHandler);
        document.removeEventListener('mouseup', upHandler);
    }

    document.addEventListener('mousemove', moveHandler);
    document.addEventListener('mouseup', upHandler);
});

function updateData(){
    fetch('/data')
    .then(r => r.json())
    .then(d => {
        document.getElementById('angle').innerText = d.angle.toFixed(3);
        document.getElementById('speed').innerText = d.speed.toFixed(3);
        document.getElementById('thetaerr').innerText = (d.angle - d.theta_eq).toFixed(3);
    })
    .catch(err => console.log(err));
}

setInterval(updateData, 150);
</script>

</body>
</html>
)rawliteral";

/* =========================================================
   调参页面
   ========================================================= */
static const char tuning_page[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Tuning</title>
</head>
<body>
  <h1>Online Tuning</h1>
  <button onclick="location.href='/'">Back</button>
</body>
</html>
)rawliteral";