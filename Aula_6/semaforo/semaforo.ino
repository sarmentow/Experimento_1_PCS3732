#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

#define LDR_PIN          1 
#define BUTTON_PIN       3  
#define RGB_LED_PIN      8   
#define NUM_PIXELS       1

const int LDR_THRESHOLD = 2500;    
const unsigned long SOS_DURATION = 3000; 
const unsigned long DEBOUNCE_TIME = 200;

Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

volatile bool sosActive = false;
volatile unsigned long lastButtonPress = 0;
int currentLdrValue = 0;
unsigned long lastLdrRead = 0;
unsigned long lastBlinkTime = 0;
bool yellowBlinkState = false;

enum TrafficState { GREEN, RED, YELLOW, NIGHT_MODE, SOS_EMERGENCY };
TrafficState currentState = GREEN;
TrafficState savedState = GREEN; // Guarda o estado anterior ao SOS
unsigned long stateStartTime = 0;

void IRAM_ATTR handleSOS() {
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_TIME) {
    sosActive = true;
    lastButtonPress = now;
  }
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void handleRoot() {
  String html = "<html><head><meta http-equiv='refresh' content='1'></head>";
  html += "<body style='font-family:Arial; text-align:center;'>";
  html += "<h1>ESP32-C3 Monitoramento SOS/LDR</h1>";
  html += "<h2>Valor LDR (ADC 12-bits): " + String(currentLdrValue) + "</h2>";
  html += "<p>Status: " + String(currentLdrValue < LDR_THRESHOLD ? "Normal" : "Baixa luminosidade") + "</p>";
  if(currentState == SOS_EMERGENCY) html += "<h2 style='color:red;'>EMERGENCIA ATIVADA!</h2>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  pixel.begin();
  setLEDColor(0, 0, 0);

  analogReadResolution(12);

  WiFi.softAP("ESP32C3_SOS_ALERTA", "12345678");
  server.on("/", handleRoot);
  server.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleSOS, FALLING);

  Serial.println("Sistema Iniciado. IP do AP: 192.168.4.1");
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  if (now - lastLdrRead > 1000) {
    currentLdrValue = analogRead(LDR_PIN);
    lastLdrRead = now;
    Serial.printf("LDR: %d | Estado Atual: %d\n", currentLdrValue, currentState);
  }

  if (sosActive) {
    sosActive = false; 
    if (currentState != SOS_EMERGENCY) {
      savedState = currentState; 
      currentState = SOS_EMERGENCY;
      stateStartTime = now;
      setLEDColor(255, 0, 0); 
    }
  }

  if (currentState == SOS_EMERGENCY) {
    setLEDColor(255, 0, 0); 
    if (now - stateStartTime > SOS_DURATION) {
      currentState = savedState != NIGHT_MODE ? GREEN : savedState; 
      stateStartTime = now;
    }
  } 
  else if (currentLdrValue >= LDR_THRESHOLD) {
    currentState = NIGHT_MODE;
    if (now - lastBlinkTime > 1000) { 
      yellowBlinkState = !yellowBlinkState;
      if (yellowBlinkState) setLEDColor(255, 150, 0); 
      else setLEDColor(0, 0, 0);
      lastBlinkTime = now;
    }
  } 
  else {
    if (currentState == NIGHT_MODE) {
      currentState = GREEN;
      stateStartTime = now;
    }

    unsigned long elapsed = now - stateStartTime;
    switch (currentState) {
      case GREEN:
        setLEDColor(0, 255, 0);
        if (elapsed > 3000) { currentState = YELLOW; stateStartTime = now; }
        break;
      case RED:   
        setLEDColor(255, 0, 0);
        if (elapsed > 4000) { currentState = GREEN; stateStartTime = now; }
        break;
      case YELLOW:
        setLEDColor(255, 150, 0);
        if (elapsed > 1000) { currentState = RED; stateStartTime = now; }
        break;
      default:
        currentState = RED;
        stateStartTime = now;
        break;
    }
  }
}
