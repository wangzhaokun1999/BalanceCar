#include "webserver.h"
#include <Arduino.h>
#include "esp_http_server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static volatile int *joy_x_ptr = nullptr;
static volatile int *joy_y_ptr = nullptr;
static volatile float *angle_ptr = nullptr;
static volatile float *speed_ptr = nullptr;

static WebApplyParamFn web_apply_fn = nullptr;
static WebReadParamsFn web_read_fn = nullptr;

static httpd_handle_t server = NULL;

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
        document.getElementById('thetaerr').innerText = d.theta_error.toFixed(3);
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
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 BalanceCar - Tuning</title>
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
.topbar{
    display:flex;
    justify-content:center;
    gap:12px;
    margin-bottom:20px;
    flex-wrap:wrap;
}
.card{
    background:#2c2c3e;
    padding:15px;
    border-radius:12px;
    box-shadow:0 2px 8px rgba(0,0,0,0.25);
    width:92%;
    max-width:860px;
    margin-bottom:16px;
}
.row{
    display:flex;
    align-items:center;
    gap:10px;
    margin:10px 0;
    flex-wrap:wrap;
}
label{
    width:140px;
}
input[type=range]{
    flex:1;
    min-width:180px;
}
input[type=number]{
    width:95px;
    padding:6px;
    border-radius:6px;
    border:none;
}
button, .navbtn{
    background:#4CAF50;
    color:white;
    border:none;
    border-radius:8px;
    padding:10px 16px;
    font-size:15px;
    cursor:pointer;
    text-decoration:none;
    display:inline-block;
}
button:hover, .navbtn:hover{
    opacity:0.9;
}
.small{
    font-size:13px;
    opacity:0.85;
}
.scale-row{
    background:#252535;
    border-radius:8px;
    padding:10px;
    margin-top:4px;
}
.scale-title{
    width:140px;
    font-size:13px;
    opacity:0.8;
}
</style>
</head>
<body>

<h1>ESP32 BalanceCar - Tuning</h1>

<div class="container">
    <div class="topbar">
        <a class="navbtn" href="/">返回控制页面</a>
        <button onclick="refreshParams()">刷新参数</button>
    </div>

    <div class="card">
        <h3>Angle Loop</h3>

        <div class="row">
            <label>Theta Eq</label>
            <input type="range" id="theta_eq_slider" min="1.20" max="1.90" step="0.001">
            <input type="number" id="theta_eq_num" step="0.001">
            <button onclick="sendParam('theta', 'theta_eq_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="theta_eq_min" step="0.001" value="1.20">
            <span>Max</span><input type="number" id="theta_eq_max" step="0.001" value="1.90">
            <span>Step</span><input type="number" id="theta_eq_step" step="0.001" value="0.001">
            <button onclick="applyScale('theta_eq_slider','theta_eq_num','theta_eq_min','theta_eq_max','theta_eq_step')">Apply Scale</button>
        </div>

        <div class="row">
            <label>Kp_theta</label>
            <input type="range" id="kp_theta_slider" min="0" max="1" step="0.001">
            <input type="number" id="kp_theta_num" step="0.001">
            <button onclick="sendParam('KpT', 'kp_theta_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="kp_theta_min" step="0.001" value="0">
            <span>Max</span><input type="number" id="kp_theta_max" step="0.001" value="1">
            <span>Step</span><input type="number" id="kp_theta_step" step="0.001" value="0.001">
            <button onclick="applyScale('kp_theta_slider','kp_theta_num','kp_theta_min','kp_theta_max','kp_theta_step')">Apply Scale</button>
        </div>

        <div class="row">
            <label>Kd_theta</label>
            <input type="range" id="kd_theta_slider" min="0" max="1" step="0.001">
            <input type="number" id="kd_theta_num" step="0.001">
            <button onclick="sendParam('KdT', 'kd_theta_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="kd_theta_min" step="0.001" value="0">
            <span>Max</span><input type="number" id="kd_theta_max" step="0.001" value="1">
            <span>Step</span><input type="number" id="kd_theta_step" step="0.001" value="0.001">
            <button onclick="applyScale('kd_theta_slider','kd_theta_num','kd_theta_min','kd_theta_max','kd_theta_step')">Apply Scale</button>
        </div>
    </div>

    <div class="card">
        <h3>Speed Loop</h3>

        <div class="row">
            <label>Kp_speed</label>
            <input type="range" id="kp_speed_slider" min="0" max="50" step="0.01">
            <input type="number" id="kp_speed_num" step="0.01">
            <button onclick="sendParam('KpS', 'kp_speed_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="kp_speed_min" step="0.01" value="0">
            <span>Max</span><input type="number" id="kp_speed_max" step="0.01" value="50">
            <span>Step</span><input type="number" id="kp_speed_step" step="0.01" value="0.01">
            <button onclick="applyScale('kp_speed_slider','kp_speed_num','kp_speed_min','kp_speed_max','kp_speed_step')">Apply Scale</button>
        </div>

        <div class="row">
            <label>Kd_speed</label>
            <input type="range" id="kd_speed_slider" min="0" max="50" step="0.01">
            <input type="number" id="kd_speed_num" step="0.01">
            <button onclick="sendParam('KdS', 'kd_speed_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="kd_speed_min" step="0.01" value="0">
            <span>Max</span><input type="number" id="kd_speed_max" step="0.01" value="50">
            <span>Step</span><input type="number" id="kd_speed_step" step="0.01" value="0.01">
            <button onclick="applyScale('kd_speed_slider','kd_speed_num','kd_speed_min','kd_speed_max','kd_speed_step')">Apply Scale</button>
        </div>
    </div>

    <div class="card">
        <h3>Filter / Timing</h3>

        <div class="row">
            <label>Te (ms)</label>
            <input type="range" id="te_slider" min="1" max="100" step="1">
            <input type="number" id="te_num" step="1">
            <button onclick="sendParam('Te', 'te_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="te_min" step="1" value="1">
            <span>Max</span><input type="number" id="te_max" step="1" value="100">
            <span>Step</span><input type="number" id="te_step" step="1" value="1">
            <button onclick="applyScale('te_slider','te_num','te_min','te_max','te_step')">Apply Scale</button>
        </div>

        <div class="row">
            <label>Tau (ms)</label>
            <input type="range" id="tau_slider" min="1" max="10000" step="1">
            <input type="number" id="tau_num" step="1">
            <button onclick="sendParam('Tau', 'tau_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="tau_min" step="1" value="1">
            <span>Max</span><input type="number" id="tau_max" step="1" value="10000">
            <span>Step</span><input type="number" id="tau_step" step="1" value="1">
            <button onclick="applyScale('tau_slider','tau_num','tau_min','tau_max','tau_step')">Apply Scale</button>
        </div>
    </div>

    <div class="card">
        <h3>Safety / EC Compensation</h3>

        <div class="row">
            <label>Theta Max (deg)</label>
            <input type="range" id="theta_max_slider" min="1" max="60" step="1">
            <input type="number" id="theta_max_num" step="1">
            <button onclick="sendParam('Tmax', 'theta_max_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="theta_max_min" step="1" value="1">
            <span>Max</span><input type="number" id="theta_max_max" step="1" value="60">
            <span>Step</span><input type="number" id="theta_max_step" step="1" value="1">
            <button onclick="applyScale('theta_max_slider','theta_max_num','theta_max_min','theta_max_max','theta_max_step')">Apply Scale</button>
        </div>

        <div class="row">
            <label>C0 Left</label>
            <input type="range" id="c0l_slider" min="0" max="0.5" step="0.001">
            <input type="number" id="c0l_num" step="0.001">
            <button onclick="sendParam('C0L', 'c0l_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="c0l_min" step="0.001" value="0">
            <span>Max</span><input type="number" id="c0l_max" step="0.001" value="0.5">
            <span>Step</span><input type="number" id="c0l_step" step="0.001" value="0.001">
            <button onclick="applyScale('c0l_slider','c0l_num','c0l_min','c0l_max','c0l_step')">Apply Scale</button>
        </div>

        <div class="row">
            <label>C0 Right</label>
            <input type="range" id="c0r_slider" min="0" max="0.5" step="0.001">
            <input type="number" id="c0r_num" step="0.001">
            <button onclick="sendParam('C0R', 'c0r_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="c0r_min" step="0.001" value="0">
            <span>Max</span><input type="number" id="c0r_max" step="0.001" value="0.5">
            <span>Step</span><input type="number" id="c0r_step" step="0.001" value="0.001">
            <button onclick="applyScale('c0r_slider','c0r_num','c0r_min','c0r_max','c0r_step')">Apply Scale</button>
        </div>

        <div class="row">
            <label>EC Max</label>
            <input type="range" id="ecmax_slider" min="0.01" max="1.0" step="0.001">
            <input type="number" id="ecmax_num" step="0.001">
            <button onclick="sendParam('ECmax', 'ecmax_num')">Set</button>
        </div>
        <div class="row small scale-row">
            <div class="scale-title">Scale</div>
            <span>Min</span><input type="number" id="ecmax_min" step="0.001" value="0.01">
            <span>Max</span><input type="number" id="ecmax_max" step="0.001" value="1.0">
            <span>Step</span><input type="number" id="ecmax_step" step="0.001" value="0.001">
            <button onclick="applyScale('ecmax_slider','ecmax_num','ecmax_min','ecmax_max','ecmax_step')">Apply Scale</button>
        </div>
    </div>
</div>

<script>
function bindPair(sliderId, numId){
    const s = document.getElementById(sliderId);
    const n = document.getElementById(numId);

    s.addEventListener('input', () => n.value = s.value);
    n.addEventListener('input', () => s.value = n.value);
}

bindPair('theta_eq_slider', 'theta_eq_num');
bindPair('kp_theta_slider', 'kp_theta_num');
bindPair('kd_theta_slider', 'kd_theta_num');
bindPair('kp_speed_slider', 'kp_speed_num');
bindPair('kd_speed_slider', 'kd_speed_num');
bindPair('te_slider', 'te_num');
bindPair('tau_slider', 'tau_num');
bindPair('theta_max_slider', 'theta_max_num');
bindPair('c0l_slider', 'c0l_num');
bindPair('c0r_slider', 'c0r_num');
bindPair('ecmax_slider', 'ecmax_num');

function setValue(sliderId, numId, val){
    document.getElementById(sliderId).value = val;
    document.getElementById(numId).value = val;
}

function applyScale(sliderId, numId, minId, maxId, stepId){
    const slider = document.getElementById(sliderId);
    const num    = document.getElementById(numId);
    const minVal = parseFloat(document.getElementById(minId).value);
    const maxVal = parseFloat(document.getElementById(maxId).value);
    const stepVal= parseFloat(document.getElementById(stepId).value);

    if (isNaN(minVal) || isNaN(maxVal) || isNaN(stepVal)) return;
    if (maxVal <= minVal) return;
    if (stepVal <= 0) return;

    slider.min = minVal;
    slider.max = maxVal;
    slider.step = stepVal;
    num.step = stepVal;

    let current = parseFloat(num.value);
    if (isNaN(current)) current = minVal;
    if (current < minVal) current = minVal;
    if (current > maxVal) current = maxVal;

    slider.value = current;
    num.value = current;

    localStorage.setItem(sliderId + "_min", minVal);
    localStorage.setItem(sliderId + "_max", maxVal);
    localStorage.setItem(sliderId + "_step", stepVal);
}

function restoreScale(sliderId, numId, minId, maxId, stepId){
    const savedMin  = localStorage.getItem(sliderId + "_min");
    const savedMax  = localStorage.getItem(sliderId + "_max");
    const savedStep = localStorage.getItem(sliderId + "_step");

    if (savedMin !== null) document.getElementById(minId).value = savedMin;
    if (savedMax !== null) document.getElementById(maxId).value = savedMax;
    if (savedStep !== null) document.getElementById(stepId).value = savedStep;

    applyScale(sliderId, numId, minId, maxId, stepId);
}

function restoreAllScales(){
    restoreScale('theta_eq_slider','theta_eq_num','theta_eq_min','theta_eq_max','theta_eq_step');
    restoreScale('kp_theta_slider','kp_theta_num','kp_theta_min','kp_theta_max','kp_theta_step');
    restoreScale('kd_theta_slider','kd_theta_num','kd_theta_min','kd_theta_max','kd_theta_step');
    restoreScale('kp_speed_slider','kp_speed_num','kp_speed_min','kp_speed_max','kp_speed_step');
    restoreScale('kd_speed_slider','kd_speed_num','kd_speed_min','kd_speed_max','kd_speed_step');
    restoreScale('te_slider','te_num','te_min','te_max','te_step');
    restoreScale('tau_slider','tau_num','tau_min','tau_max','tau_step');
    restoreScale('theta_max_slider','theta_max_num','theta_max_min','theta_max_max','theta_max_step');
    restoreScale('c0l_slider','c0l_num','c0l_min','c0l_max','c0l_step');
    restoreScale('c0r_slider','c0r_num','c0r_min','c0r_max','c0r_step');
    restoreScale('ecmax_slider','ecmax_num','ecmax_min','ecmax_max','ecmax_step');
}

function refreshParams(){
    fetch('/params')
    .then(r => r.json())
    .then(d => {
        setValue('theta_eq_slider',  'theta_eq_num',  d.theta_eq);
        setValue('kp_theta_slider',  'kp_theta_num',  d.Kp_theta);
        setValue('kd_theta_slider',  'kd_theta_num',  d.Kd_theta);
        setValue('kp_speed_slider',  'kp_speed_num',  d.Kp_speed);
        setValue('kd_speed_slider',  'kd_speed_num',  d.Kd_speed);
        setValue('te_slider',        'te_num',        d.Te);
        setValue('tau_slider',       'tau_num',       d.Tau);
        setValue('theta_max_slider', 'theta_max_num', d.theta_max_deg);
        setValue('c0l_slider',       'c0l_num',       d.C0_L);
        setValue('c0r_slider',       'c0r_num',       d.C0_R);
        setValue('ecmax_slider',     'ecmax_num',     d.ec_max);
    })
    .catch(err => console.log(err));
}

function sendParam(name, inputId){
    let value = document.getElementById(inputId).value;
    fetch('/set?name=' + encodeURIComponent(name) + '&value=' + encodeURIComponent(value))
    .then(r => r.text())
    .then(t => {
        console.log(t);
        refreshParams();
    })
    .catch(err => console.log(err));
}

restoreAllScales();
refreshParams();
</script>

</body>
</html>
)rawliteral";

/* =========================================================
   基础 handler
   ========================================================= */
static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, control_page, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t tuning_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, tuning_page, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t data_handler(httpd_req_t *req)
{
    char resp[128];
    float angle = (angle_ptr ? *angle_ptr : 0.0f);
    float speed = (speed_ptr ? *speed_ptr : 0.0f);

    WebParams p = {};
    if (web_read_fn) web_read_fn(&p);

    float theta_error = angle - p.theta_eq;

    snprintf(resp, sizeof(resp),
             "{\"angle\":%.4f,\"speed\":%.4f,\"theta_eq\":%.4f,\"theta_error\":%.4f}",
             angle, speed, p.theta_eq, theta_error);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t params_handler(httpd_req_t *req)
{
    if (!web_read_fn)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No callback");
        return ESP_FAIL;
    }

    WebParams p;
    web_read_fn(&p);

    char resp[256];
    snprintf(resp, sizeof(resp),
             "{"
             "\"Te\":%.4f,"
             "\"Tau\":%.4f,"
             "\"Kp_theta\":%.6f,"
             "\"Kd_theta\":%.6f,"
             "\"Kp_speed\":%.6f,"
             "\"Kd_speed\":%.6f,"
             "\"theta_eq\":%.6f,"
             "\"theta_max_deg\":%.4f,"
             "\"C0_L\":%.6f,"
             "\"C0_R\":%.6f,"
             "\"ec_max\":%.6f"
             "}",
             p.Te, p.Tau,
             p.Kp_theta, p.Kd_theta,
             p.Kp_speed, p.Kd_speed,
             p.theta_eq, p.theta_max_deg,
             p.C0_L, p.C0_R, p.ec_max);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t set_handler(httpd_req_t *req)
{
    if (!web_apply_fn)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No callback");
        return ESP_FAIL;
    }

    char query[128];
    char name[32];
    char value[32];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (httpd_query_key_value(query, "name", name, sizeof(name)) != ESP_OK ||
        httpd_query_key_value(query, "value", value, sizeof(value)) != ESP_OK)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    float v = atof(value);
    web_apply_fn(name, v);

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t joy_handler(httpd_req_t *req)
{
    char query[64];
    char buf[16];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        if (joy_x_ptr && httpd_query_key_value(query, "x", buf, sizeof(buf)) == ESP_OK)
            *joy_x_ptr = atoi(buf);

        if (joy_y_ptr && httpd_query_key_value(query, "y", buf, sizeof(buf)) == ESP_OK)
            *joy_y_ptr = atoi(buf);
    }

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

/* =========================================================
   启动函数（6参数版本）
   ========================================================= */
void webserver_begin(volatile int *joy_x, volatile int *joy_y,
                     volatile float *angle_in, volatile float *speed_in,
                     WebApplyParamFn apply_fn, WebReadParamsFn read_fn)
{
    joy_x_ptr = joy_x;
    joy_y_ptr = joy_y;
    angle_ptr = angle_in;
    speed_ptr = speed_in;
    web_apply_fn = apply_fn;
    web_read_fn = read_fn;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t uri_root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };

        httpd_uri_t uri_tuning = {
            .uri = "/tuning",
            .method = HTTP_GET,
            .handler = tuning_handler,
            .user_ctx = NULL
        };

        httpd_uri_t uri_data = {
            .uri = "/data",
            .method = HTTP_GET,
            .handler = data_handler,
            .user_ctx = NULL
        };

        httpd_uri_t uri_params = {
            .uri = "/params",
            .method = HTTP_GET,
            .handler = params_handler,
            .user_ctx = NULL
        };

        httpd_uri_t uri_set = {
            .uri = "/set",
            .method = HTTP_GET,
            .handler = set_handler,
            .user_ctx = NULL
        };

        httpd_uri_t uri_joy = {
            .uri = "/joy",
            .method = HTTP_GET,
            .handler = joy_handler,
            .user_ctx = NULL
        };

        httpd_register_uri_handler(server, &uri_root);
        httpd_register_uri_handler(server, &uri_tuning);
        httpd_register_uri_handler(server, &uri_data);
        httpd_register_uri_handler(server, &uri_params);
        httpd_register_uri_handler(server, &uri_set);
        httpd_register_uri_handler(server, &uri_joy);
    }
}