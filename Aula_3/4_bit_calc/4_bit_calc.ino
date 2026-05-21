/*
  Adaptação do WiFiAccessPoint.ino para servir uma calculadora 4-bits.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>

// Set these to your desired credentials.
const char *ssid = "ESP32";
const char *password = "110010010011";

const int pins[] = {18, 19, 22, 23};

NetworkServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(23, OUTPUT);

  // a valid password must have more than 7 characters
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (1);
  }
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
}

void send_html_page(NetworkClient client, bool warning){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  client.print("<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Calc 4-bits</title></head><body>");
  client.print("<h2>Calculadora 4-bits</h2>");
            
  if(warning){
    client.print("<div class=\"alert\"> Overflow! <\div>");
  }
            
  client.print("<form action=\"/calc\" method=\"GET\">");
  client.print("<label>A: </label>");
  client.print("<input type=\"text\" name=\"a\" maxlength=\"4\" pattern=\"[01]{1,4}\" required><br><br>");
            
  client.print("<label>B: </label>");
  client.print("<input type=\"text\" name=\"b\" maxlength=\"4\" pattern=\"[01]{1,4}\" required><br><br>");
            
  client.print("<button type=\"submit\" name=\"op\" value=\"add\">Soma</button> ");
  client.print("<button type=\"submit\" name=\"op\" value=\"sub\">Subtra&ccedil;&atilde;o</button>");
            
  client.print("</form>");
  client.print("</body></html>");

  client.println();
}

void set_leds(int8_t value){
  digitalWrite(18, value & 0x08);
  digitalWrite(19, value & 0x04);
  digitalWrite(22, value & 0x02);
  digitalWrite(23, value & 0x01);
}

void loop() {
  NetworkClient client = server.accept();

  if (client) {
    Serial.println("New Client.");
    String currentLine = "";

    bool overflow = false;
    while (client.connected()) {    
      if (client.available()) {     
        char c = client.read();
        Serial.write(c);
        
        if (c == '\n') {            
          if (currentLine.indexOf("GET /calc?") >= 0) {
  
            int indexA  = currentLine.indexOf("a=");
            int indexB  = currentLine.indexOf("&b=");
            int indexOp = currentLine.indexOf("&op=");
            
            int indexEnd = currentLine.indexOf(" ", indexOp); 

            if (indexA != -1 && indexB != -1 && indexOp != -1) {
              
              String strA  = currentLine.substring(indexA + 2, indexB);
              String strB  = currentLine.substring(indexB + 3, indexOp);
              String strOp = currentLine.substring(indexOp + 4, indexEnd != -1 ? indexEnd : currentLine.length());

              strOp.trim();

              int8_t intA = strtol(strA.c_str(), NULL, 2);
              int8_t intB = strtol(strB.c_str(), NULL, 2);

              Serial.println("\n--- Nova Operacao Recebida ---");
              Serial.printf("Operacao: %s\n", strOp.c_str());
              Serial.print("A: ");
              Serial.println(intA, BIN);
              Serial.print("B: ");
              Serial.println(intB, BIN);
              intB = strOp == "add" ? intB : ((intB ^ 0x0F) &0x0F) + 1;

              int8_t resultado = (intA + intB) & 0x0F;

              int8_t signA = (intA & 0x08) >> 3;
              int8_t signB = (intB & 0x08) >> 3;
              int8_t signR = (resultado & 0x08) >> 3;
              int8_t ovf = ((~(signA ^ signB)) & (signA ^ signR)) & 0x01;

              Serial.printf("Overflow = %01X \n", ovf);

              Serial.print("Resultado: ");
              Serial.println(resultado, BIN);
              set_leds(resultado);
              Serial.println("-------------------------------\n");

              if(ovf){
                overflow = true;
                Serial.println("Houve Overflow!");
              }
              else{
                overflow = false;
                Serial.println("Nao houve Overflow!");
              }
            }
          }
          if (currentLine.length() == 0) {
            send_html_page(client, overflow);
            break;
            
          } else {  
            currentLine = "";
          }
        } else if (c != '\r') {  
          currentLine += c;
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}