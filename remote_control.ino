#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include "DHT.h"

//Setup Webserver
const char* ssid = "Salterra";
const char* password = "salterra123";

AsyncWebServer  server(80);

const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, minimal-ui, height=device-height, viewport-fit=cover">
  <style>
    /* Disable scrolling and zooming */
    html, body {
      height: 100%; /* Ensure full-height */
      margin: 0; /* Remove default margins */
      padding: 0; /* Remove default padding */
      overflow: auto; /* Prevent both horizontal and vertical scrolling */
      touch-action: auto; /* Disable touch gestures like pinch-to-zoom */
    }

    body {
      font-family: Arial, sans-serif;
      text-align: center;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      background: linear-gradient(to bottom, #202020, #121212);
      flex-direction: column;
    }

    /* Prevent text selection and pop-up issues */
    * {
      user-select: none;
      -webkit-user-select: none; /* Safari */
      -moz-user-select: none; /* Firefox */
      -ms-user-select: none; /* IE/Edge */
    }

    /* Prevent long-press popups on buttons */
    button {
      -webkit-touch-callout: none;
    }
    
    .title {
      color: white;
      font-size: 32px;
      font-weight: bold;
      margin-top: 70px;
      text-align: center;
    }
    .terra {
  color: #69e0c1; /* Green color */
  }

    /* Joystick Container */
    .controller {
      display: flex;
      justify-content: space-between;
      margin-bottom: 50px;
      width: 100%;
    }

    .joystick-container {
      width: 250px;
      height: 150px;
      display: flex;
      justify-content: center;
      align-items: center;
      position: relative;
    }

    .joystick-base {
      width: 150px;
      height: 150px;
      background: radial-gradient(circle, #555, #222);
      border-radius: 50%;
      box-shadow: inset 0 5px 10px rgba(105, 224, 193, 0.7), 0 10px 20px rgba(105, 224, 193, 0.5);
      position: absolute;
    }

    .joystick-handle {
      width: 40px;
      height: 40px;
      background: radial-gradient(circle, #eee, #888);
      border-radius: 50%;
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      box-shadow: inset 0 5px 8px rgba(0, 0, 0, 0.5), 0 5px 15px rgba(0, 0, 0, 0.3);
      transition: transform 0.1s linear;
    }

    /* Info Container */
    .info-container {
      margin-bottom: 20px;
      color: white;
      font-size: 18px;
    }

    /* Servo Button */
    .servo-button {
      position: fixed;
      top: 50px;
      right: 50px;
      padding: 12px 20px;
      font-size: 18px;
      background-color: #007BFF;
      color: white;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      transition: background 0.3s;
    }

    .servo-button:hover {
      background-color: #0056b3;
    }

    /* Mode Toggle Button */
    .mode-toggle-button {
      position: fixed;
      top: 50px;
      left: 50px;
      padding: 12px 20px;
      font-size: 18px;
      background-color: #28a745;
      color: white;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      transition: background 0.3s;
      width: 100px;
      text-align: center;
    }

    .mode-toggle-button:hover {
      background-color: #218838;
    }
  </style>
  <script>

    let joysticks = {
      joystickL: { x: 0, y: 0, touchId: null },
      joystickR: { x: 0, y: 0, touchId: null }
    };

    let maxRadius = 50;
    let currentMode = 'manual';

    function getJoystickId(touch) {
      const element = document.elementFromPoint(touch.clientX, touch.clientY);
      return element ? element.closest('.joystick-container')?.id : null;
    }

    function updateJoystick(event) {
      for (let touch of event.touches) {
        let joystickId = getJoystickId(touch);
        if (!joystickId || !joysticks[joystickId]) continue;

        let joystick = joysticks[joystickId];
        if (joystick.touchId === null) joystick.touchId = touch.identifier;
        if (joystick.touchId !== touch.identifier) continue;

        const base = document.getElementById(joystickId);
        const handle = base.querySelector('.joystick-handle');
        const rect = base.getBoundingClientRect();
        const offsetX = touch.clientX - rect.left - rect.width / 2;
        const offsetY = touch.clientY - rect.top - rect.height / 2;

        const distance = Math.min(Math.sqrt(offsetX ** 2 + offsetY ** 2), maxRadius);
        const angle = Math.atan2(offsetY, offsetX);

        joystick.x = Math.round((distance * Math.cos(angle)) / maxRadius * 100);
        joystick.y = Math.round((distance * Math.sin(angle)) / maxRadius * 100);

        handle.style.transform = `translate(${joystick.x * 0.8}px, ${joystick.y * 0.8}px)`;
        sendJoystickData(joystickId, joystick.x, joystick.y);
      }
    }

    function resetJoystick(event) {
      for (let touch of event.changedTouches) {
        for (let joystickId in joysticks) {
          let joystick = joysticks[joystickId];
          if (joystick.touchId === touch.identifier) {
            joystick.x = 0;
            joystick.y = 0;
            joystick.touchId = null;
            const handle = document.getElementById(joystickId).querySelector('.joystick-handle');
            handle.style.transform = `translate(-50%, -50%)`;
            sendJoystickData(joystickId, 0, 0);
          }
        }
      }
    }

    function sendJoystickData(id, x, y) {
      fetch(`/joystick?id=${id}&x=${x}&y=${y}`);
    }

    function onServoPress() {
      fetch(`/servo?state=true`);
    }

    function onServoRelease() {
      fetch(`/servo?state=false`);
    }

    function updateTempHumidity() {
      fetch('/sensor')
        .then(response => response.json())
        .then(data => {
          document.getElementById('temp').innerText = `Temperature: ${data.temp}°C`;
          document.getElementById('humidity').innerText = `Humidity: ${data.humidity}%`;
        });
    }

    function toggleMode() {
      currentMode = currentMode === 'manual' ? 'auto' : 'manual';
      document.getElementById('mode-toggle').innerText = currentMode === 'manual' ? 'Auto' : 'Manual';
      document.getElementById('mode-toggle').style.backgroundColor = currentMode === 'manual' ? '#28a745' : '#dc3545';
      fetch(`/mode?state=${currentMode}`);
    }

    document.addEventListener('DOMContentLoaded', () => {
      document.querySelectorAll('.joystick-container').forEach(container => {
        container.addEventListener('touchstart', updateJoystick);
        container.addEventListener('touchmove', updateJoystick);
        container.addEventListener('touchend', resetJoystick);
        container.addEventListener('touchcancel', resetJoystick);
      });
      setInterval(updateTempHumidity, 5000); // Update every 5 seconds
    });
    function disableScroll() {
    document.body.style.overflow = "hidden";
    document.documentElement.style.overflow = "hidden";
    }
    function enableScroll() {
    document.body.style.overflow = "auto";
    document.documentElement.style.overflow = "auto";
    }

// Add event listeners to disable scrolling when touching the joystick
    document.querySelectorAll('.joystick-container').forEach(container => {
    container.addEventListener('touchstart', disableScroll);
    container.addEventListener('touchend', enableScroll);
    container.addEventListener('touchcancel', enableScroll);
    });

// Also prevent scrolling when dragging the joystick
    document.addEventListener('touchmove', (event) => {
    if (event.target.closest('.joystick-container')) {
    event.preventDefault(); // Stops scrolling when moving joystick
    }
    }, { passive: false }); 
  
    
  </script>
</head>
<body>
  <h1 class="title">Salt<span class="terra">Terra</span></h1>
  <div class="info-container">
    <p id="temp">Temperature: --°C</p>
    <p id="humidity">Humidity: --%</p>
  </div>
  <div class="controller">
    <div id="joystickL" class="joystick-container">
      <div class="joystick-base"></div>
      <div class="joystick-handle"></div>
    </div>

    <div id="joystickR" class="joystick-container">
      <div class="joystick-base"></div>
      <div class="joystick-handle"></div>
    </div>
  </div>

  <button id="mode-toggle" class="mode-toggle-button" onclick="toggleMode()">Auto</button>
  <button class="servo-button" onmousedown="onServoPress()" onmouseup="onServoRelease()" ontouchstart="onServoPress()" ontouchend="onServoRelease()">Drop Salt</button>
</body>
</html>
)rawliteral";

//Initialize Motor Pins
int left_motor_en = 32;
int left_motor_pin1 = 33;
int left_motor_pin2 = 25;
int left_motor_channel = 1;

int right_motor_en = 14;
int right_motor_pin1 = 26;
int right_motor_pin2 = 27;
int right_motor_channel = 2;

int max_speed = 255-73;
int pwm_frequency = 1000;
int pwm_resolution = 8;

//Initialize System Parameters
float temperature = 0.0;
float humidity = 0.0;
bool servo_state = false;  // false = closed, true = open
bool prev_servo_state = false;
String mode = "manual"; // Default mode

//Setup Joystick
int joystickL_X = 0, joystickL_Y = 0;
int joystickR_X = 0, joystickR_Y = 0;
int joystick_upper = 100;
int joystick_lower = -100;

//Setup Servo
Servo myServo;
int servoPin = 21;
int servo_active_angle = 90;
int servo_inactive_angle = 0;

//Setup Temperature and Humidity Sensor
#define DHTTYPE DHT11
int dhtPin = 22;
DHT dht(dhtPin, DHTTYPE);

//Unblock Timers
unsigned long prev_time = 0;
const long time_delay = 100;
unsigned long prev_servo_time = 0;
int step = 0;
int manual_step = 0;

void setup() {
  Serial.begin(115200);

  ledcSetup(left_motor_channel, pwm_frequency, pwm_resolution);
  ledcAttachPin(left_motor_en, left_motor_channel);

  ledcSetup(right_motor_channel, pwm_frequency, pwm_resolution);
  ledcAttachPin(right_motor_en, right_motor_channel);

  // Setup Motor Pins
  //pinMode(left_motor_en,  OUTPUT); 
  pinMode(left_motor_pin1, OUTPUT);
  pinMode(left_motor_pin2, OUTPUT);

  //pinMode(right_motor_en, OUTPUT);
  pinMode(right_motor_pin1, OUTPUT);
  pinMode(right_motor_pin2, OUTPUT);

  myServo.attach(servoPin);

  // dht.begin();

  // Setup Web Server
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", html);
  });

  server.on("/joystick", HTTP_GET, [](AsyncWebServerRequest *request){
      if (mode == "manual" && request->hasParam("id") && request->hasParam("x") && request->hasParam("y")) {
          String id = request->getParam("id")->value();
          int x = request->getParam("x")->value().toInt();
          int y = request->getParam("y")->value().toInt();

          if (id == "joystickL") {
              joystickL_X = x;
              joystickL_Y = y;
          } else if (id == "joystickR") {
              joystickR_X = x;
              joystickR_Y = y;
          }
          //Serial.printf("Joystick %s -> X: %d, Y: %d\n", id.c_str(), x, y);
          request->send(200, "text/plain", "OK");
      } else {
          request->send(400, "text/plain", "Bad Request");
      }
  });

  server.on("/servo", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("state")) {
        String state = request->getParam("state")->value();
        servo_state = (state == "true");  // Convert to boolean
        //Serial.print("Servo State: ");
        //Serial.println(servo_state);
        request->send(200, "text/plain", "Servo updated");
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
  });

  // Sensor route
  server.on("/sensor", HTTP_GET, handle_sensor_data);

  // Mode route
  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("state")) {
        mode = request->getParam("state")->value();
        //Serial.print("Robot Mode: "); 
        //Serial.println(mode);
        request->send(200, "text/plain", "Mode updated to " + mode);
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
  });

  server.begin();
  Serial.println("Server started");
}

void loop() {
  unsigned long current_time = millis();
  if (current_time - prev_time >= time_delay) {
    prev_time = current_time;

    if (mode == "manual") {
      handle_salt_drop();
      remote_controller();
    } else if (mode == "auto") {
      handle_auto();
    }
  }
}

// ===[Misc Functions]===
void handle_auto() {
  unsigned long current_time = millis();

  set_motor_speed(-max_speed, -max_speed);
  switch (step) {
    case 0: // Wait 500ms before moving servo
      if (current_time - prev_servo_time >= 500) {
        myServo.write(servo_active_angle);
        prev_servo_time = current_time;
        step = 1;
      }
      break;

    case 1: // Wait 100ms before moving servo back
      if (current_time - prev_servo_time >= 100) {
        myServo.write(servo_inactive_angle);
        prev_servo_time = current_time;
        step = 0; // Restart cycle
      }
      break;
  }
}

void handle_sensor_data(AsyncWebServerRequest *request) {
  // Read temperature and humidity from the sensor
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  //Serial.print("Humidity: ");
  //Serial.print(humidity);
  //Serial.print(" || Temp: ");
  //Serial.println(temperature);

  // Check if the readings failed
  if (isnan(humidity) || isnan(temperature)) {
    request->send(500, "text/plain", "Failed to read from sensor");
    return;
  }

  // Create a JSON response
  String jsonResponse = "{\"temp\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";

  // Send the response back as JSON
  request->send(200, "application/json", jsonResponse);
}

void handle_salt_drop() {
  if (prev_servo_state != servo_state) {
    Serial.println("New State Matched");
    prev_servo_state = servo_state;
    if (servo_state == 1) {
      Serial.println("GO 90");
      myServo.write(servo_active_angle);
    } else {
      Serial.println("GO 0");
      myServo.write(servo_inactive_angle);
    }
  }
}

// ===[Motor Driver Functions]===
// Motor Control
void remote_controller() {
  float speed_signal = map(joystickL_Y, joystick_lower, joystick_upper, -max_speed, max_speed); //Map left joystick to control speed
  float turning_signal = map(joystickR_X, joystick_lower, joystick_upper, -100, 100); //Map right joystick to control turning
  float turning_percent = 1.0 - abs(turning_signal/100.0);
  //Serial.println(turning_percent);
  // Set speed signal
  float right_signal = speed_signal;
  float left_signal = speed_signal;

  if (speed_signal == 0 && turning_signal != 0) {
    speed_signal = map(joystickR_X, joystick_lower, joystick_upper, -max_speed, max_speed);

    if (turning_signal > 0) {
      //Turn in place right
      right_signal = speed_signal;
      left_signal = -speed_signal;
    } else {
      //Turn in place right
      right_signal = speed_signal;
      left_signal = -speed_signal;
    }
  } else if (speed_signal != 0) {
    if (turning_signal < 0) { // Turn Left
      left_signal = left_signal * turning_percent;
    } else if (turning_signal > 0){ // Turn Right
      right_signal = right_signal * turning_percent;
    }
  }

  set_motor_speed(left_signal, right_signal);
}
// Rotate Both Motors
void set_motor_speed(int left_signal, float right_signal) {
  Serial.print("Left: ");
  Serial.print(left_signal);
  Serial.print(" || Right: ");
  Serial.println(right_signal);
  left_motor_move(left_signal, abs(right_signal));
  right_motor_move(right_signal, abs(right_signal));
}

// Rotate the Left Motor
void left_motor_move(int state, int pwm) {
  if (pwm == 0) {
    ledcWrite(left_motor_channel, 0);
  } else {
    ledcWrite(left_motor_channel, pwm+73);
  }
  //analogWrite(left_motor_en, pwm);
  if (state > 1) {
    digitalWrite(left_motor_pin1,  HIGH);
    digitalWrite(left_motor_pin2, LOW);
  } else if (state < -1) {
    digitalWrite(left_motor_pin1,  LOW);
    digitalWrite(left_motor_pin2, HIGH);
  } else {
    digitalWrite(left_motor_pin1, LOW);
    digitalWrite(left_motor_pin2, LOW);
  }
}

// Rotate the Right Motor
void right_motor_move(int state, int pwm) {
  ledcWrite(right_motor_channel, pwm);
  //analogWrite(right_motor_en, pwm);
  if (state > 1) {
    digitalWrite(right_motor_pin1,  HIGH);
    digitalWrite(right_motor_pin2, LOW);
  } else if (state < -1) {
    digitalWrite(right_motor_pin1,  LOW);
    digitalWrite(right_motor_pin2, HIGH);
  } else {
    digitalWrite(right_motor_pin1, LOW);
    digitalWrite(right_motor_pin2, LOW);
  }
}
