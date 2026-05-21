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

NetworkServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

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

void loop() {
  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                     // if you get a client,
    Serial.println("New Client.");
    String currentLine = "";        // make a String to hold incoming data from the client

    while (client.connected()) {    
      if (client.available()) {     
        char c = client.read();
        Serial.write(c);            // print it out the serial monitor
        
        if (c == '\n') {            
          // Se a linha atual contém a nossa requisição alvo, podemos processá-la aqui
          if (currentLine.indexOf("GET /calc?") >= 0) {
  
            // Encontra as posições iniciais de cada parâmetro na string
            int indexA  = currentLine.indexOf("a=");
            int indexB  = currentLine.indexOf("&b=");
            int indexOp = currentLine.indexOf("&op=");
            
            // Encontra o espaço que separa a URL do protocolo "HTTP/1.1"
            int indexEnd = currentLine.indexOf(" ", indexOp); 

            // Verifica se todos os parâmetros foram encontrados com sucesso
            if (indexA != -1 && indexB != -1 && indexOp != -1) {
              
              // Extrai as substrings correspondentes a cada valor
              String strA  = currentLine.substring(indexA + 2, indexB);
              String strB  = currentLine.substring(indexB + 3, indexOp);
              String strOp = currentLine.substring(indexOp + 4, indexEnd != -1 ? indexEnd : currentLine.length());

              // Remove eventuais caracteres de quebra de linha residuais na string da operação
              strOp.trim();

              // Converte as strings binárias de 4 bits para inteiros decimais (Base 2)
              int intA = strtol(strA.c_str(), NULL, 2);
              int intB = strtol(strB.c_str(), NULL, 2);

              // Exibe os valores convertidos no Monitor Serial para validação
              Serial.println("\n--- Nova Operacao Recebida ---");
              Serial.printf("String original A: %s -> Inteiro: %d\n", strA.c_str(), intA);
              Serial.printf("String original B: %s -> Inteiro: %d\n", strB.c_str(), intB);
              Serial.printf("Operacao detectada: %s\n", strOp.c_str());

              // Executa a lógica matemática baseada na string 'strOp'
              int resultado = 0;
              if (strOp == "add") {
                resultado = intA + intB;
                Serial.printf("Resultado da Soma: %d\n", resultado);
              } 
              else if (strOp == "sub") {
                resultado = intA - intB;
                Serial.printf("Resultado da Subtracao: %d\n", resultado);
              }
              Serial.println("-------------------------------\n");
            }
          }

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // Conteúdo HTML da página com o formulário:
            client.print("<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Calc 4-bits</title></head><body>");
            client.print("<h2>Calculadora 4-bits</h2>");
            
            // O formulário usa o método GET e aponta para a rota /calc
            client.print("<form action=\"/calc\" method=\"GET\">");
            
            // Campos de input limitados a 4 caracteres. A tag pattern exige que sejam apenas 0s e 1s.
            client.print("<label>A: </label>");
            client.print("<input type=\"text\" name=\"a\" maxlength=\"4\" pattern=\"[01]{1,4}\" required><br><br>");
            
            client.print("<label>B: </label>");
            client.print("<input type=\"text\" name=\"b\" maxlength=\"4\" pattern=\"[01]{1,4}\" required><br><br>");
            
            // Botões que enviam o parâmetro "op" com o valor correspondente
            client.print("<button type=\"submit\" name=\"op\" value=\"add\">Soma</button> ");
            client.print("<button type=\"submit\" name=\"op\" value=\"sub\">Subtra&ccedil;&atilde;o</button>");
            
            client.print("</form>");
            client.print("</body></html>");

            // The HTTP response ends with another blank line:
            client.println();
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