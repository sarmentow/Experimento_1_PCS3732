#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "esp32-b7";
const char* password = "12345678";

// --- Configurações do LED ---
const int ledPin = 5;        
int pwmFreq = 5000;          
const int pwmResolution = 8;
int dutyCycle = 0;          

// --- Configurações do Servo Motor ---
const int servoPin = 6;            
const int servoFreq = 50;          
const int servoResolution = 12;    
int servoAngle = 90;               

WebServer server(80);

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Painel de Controle - ESP32</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 40px; background-color: #f0f2f5; color: #333; }
    .container { background: white; padding: 30px; border-radius: 12px; box-shadow: 0px 4px 12px rgba(0,0,0,0.15); display: inline-block; max-width: 400px; width: 100%; margin-bottom: 20px;}
    input[type="range"] { width: 100%; margin: 15px 0; }
    select { padding: 10px; font-size: 16px; width: 100%; border-radius: 6px; border: 1px solid #ccc; }
    h1 { font-size: 22px; margin-bottom: 5px; color: #222; }
    h3 { font-size: 16px; font-weight: normal; margin-top: 15px; text-align: left; }
    .val-display { font-weight: bold; color: #0066cc; }
    hr { border: 0; height: 1px; background: #ddd; margin: 25px 0; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Painel de Controle</h1>
    <p>ESP32-C3 DevKit</p>
    
    <hr>
    
    <h1 style="font-size: 18px; text-align: left;">1. Controle do LED</h1>
    <h3>Intensidade (0-255): <span class="val-display" id="dutyLabel">0</span></h3>
    <input type="range" id="dutySlider" min="0" max="255" value="0" oninput="updateLED()">
    
    <h3>Frequência (Hz):</h3>
    <select id="freqSelect" onchange="updateLED()">
      <option value="10">10 Hz (Piscar visível)</option>
      <option value="50">50 Hz</option>
      <option value="100">100 Hz</option>
      <option value="1000">1 kHz</option>
      <option value="5000" selected>5 kHz (Padrão)</option>
      <option value="10000">10 kHz</option>
    </select>

    <hr>

    <h1 style="font-size: 18px; text-align: left;">2. Controle do Servo Motor</h1>
    <h3>Ângulo: <span class="val-display" id="servoLabel" style="color: #e65c00;">90</span>°</h3>
    <input type="range" id="servoSlider" min="0" max="180" value="90" oninput="updateServo()">
    
  </div>

  <script>
    function updateLED() {
      var duty = document.getElementById("dutySlider").value;
      var freq = document.getElementById("freqSelect").value;
      document.getElementById("dutyLabel").innerText = duty;
      
      fetch(`/setLED?duty=${duty}&freq=${freq}`)
        .catch(error => console.error("Erro LED:", error));
    }

    function updateServo() {
      var angle = document.getElementById("servoSlider").value;
      document.getElementById("servoLabel").innerText = angle;
      
      fetch(`/setServo?angle=${angle}`)
        .catch(error => console.error("Erro Servo:", error));
    }
  </script>
</body>
</html>
)rawliteral";

void setServoAngle(int angle) {

  if(angle < 0) angle = 0;
  if(angle > 180) angle = 180;

  int duty = map(angle, 0, 180, 102, 512); 
  
  ledcWrite(servoPin, duty);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nIniciando sistema...");

  ledcAttach(ledPin, pwmFreq, pwmResolution);
  ledcWrite(ledPin, dutyCycle);
  
  ledcAttach(servoPin, servoFreq, servoResolution);
  setServoAngle(servoAngle); // Move para o centro (90°)

  WiFi.softAP(ssid, password);
  Serial.println("AP Criado! Conecte ao IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });

  server.on("/setLED", HTTP_GET, []() {
    if (server.hasArg("duty") && server.hasArg("freq")) {
      dutyCycle = server.arg("duty").toInt();
      int newFreq = server.arg("freq").toInt();

      if (newFreq != pwmFreq) {
        pwmFreq = newFreq;
        ledcAttach(ledPin, pwmFreq, pwmResolution);
      }
      ledcWrite(ledPin, dutyCycle);
      Serial.printf("LED -> Duty: %d | Freq: %d Hz\n", dutyCycle, pwmFreq);
      server.send(200, "text/plain", "OK");
    }
  });

  server.on("/setServo", HTTP_GET, []() {
    if (server.hasArg("angle")) {
      servoAngle = server.arg("angle").toInt();
      setServoAngle(servoAngle);
      Serial.printf("Servo -> Ângulo: %d°\n", servoAngle);
      server.send(200, "text/plain", "OK");
    }
  });

  server.begin();
  Serial.println("Servidor HTTP rodando.");
}

void loop() {
  server.handleClient();
}