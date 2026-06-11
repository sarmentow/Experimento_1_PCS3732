#include <Arduino.h>
#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>

// Set these to your desired credentials.
const char *ssid = "ESP32";
const char *password = "110010010011";

const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>8-Bit Signed Binary Calculator</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #eef2f3;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }

        .calculator {
            background-color: #333;
            padding: 20px;
            border-radius: 12px;
            box-shadow: 0 8px 24px rgba(0,0,0,0.3);
            width: 320px;
        }

        /* Result field at the top */
        .result-field {
            background-color: #aed581;
            color: #1b5e20;
            font-family: 'Courier New', Courier, monospace;
            font-size: 2rem;
            font-weight: bold;
            text-align: right;
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
            min-height: 40px;
            box-shadow: inset 0 2px 5px rgba(0,0,0,0.2);
            word-wrap: break-word;
        }

        .result-field.overflow {
            background-color: #ff8a80;
            color: #b71c1c;
            font-size: 1.5rem;
            text-align: center;
        }

        .main-interface {
            display: flex;
            gap: 15px;
        }

        /* Left Column: Stacked Inputs */
        .inputs-column {
            display: flex;
            flex-direction: column;
            justify-content: space-between;
            flex: 1;
        }

        .inputs-column input {
            width: 100%;
            box-sizing: border-box;
            font-family: 'Courier New', Courier, monospace;
            font-size: 1.6rem;
            padding: 10px;
            text-align: center;
            border: 2px solid #555;
            background-color: #222;
            color: #fff;
            border-radius: 6px;
        }

        .inputs-column input:focus {
            border-color: #aed581;
            outline: none;
        }

        /* Right Column: Square Pad with 4 Buttons */
        .buttons-square {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 10px;
            width: 120px;
            padding-top: 20px;
        }

        .buttons-square button {
            background-color: #444;
            color: #fff;
            border: none;
            border-radius: 6px;
            font-size: 1.4rem;
            font-weight: bold;
            cursor: pointer;
            transition: background 0.1s;
        }

        .buttons-square button:hover {
            background-color: #555;
        }

        .buttons-square button:active {
            background-color: #aed581;
            color: #222;
        }

        .label {
            color: #aaa;
            font-size: 0.8rem;
            margin-bottom: 2px;
        }
    </style>
</head>
<body>

<div class="calculator">
    <div class="result-field" id="result">00000000</div>

    <form method="GET" action="/calc">
        <div class="main-interface">
            <div class="inputs-column">
                <div>
                    <div class="label">Binary Num 1</div>
                    <input type="text" name="a" id="num1" pattern="[01]{8}" maxlength="8" placeholder="00000000" required title="Must be an 8-bit binary number (e.g., 01010101)">
                </div>
                <div>
                    <div class="label">Binary Num 2</div>
                    <input type="text" name="b" id="num2" pattern="[01]{8}" maxlength="8" placeholder="00000000" title="Must be an 8-bit binary number (e.g., 11101110)">
                </div>
            </div>

            <div class="buttons-square">
                <button type="submit" name="op" value="add" title="Addition">+</button>
                <button type="submit" name="op" value="sub" title="Subtraction">-</button>
                <button type="submit" name="op" value="mul" title="Multiplication">*</button>
                <button type="submit" name="op" value="div" title="Division">/</button>
            </div>
        </div>
    </form>
</div>

<script>
    window.addEventListener('DOMContentLoaded', () => {
        const urlParams = new URLSearchParams(window.location.search);
        const num1Str = urlParams.get('a'); 
        const num2Str = urlParams.get('b');
        const op = urlParams.get('op');

        const resultDisplay = document.getElementById('result');

        function binToSignedInt(bin) {
            let val = parseInt(bin, 2);
            if (bin[0] === '1') { 
                val -= 256; // CORRIGIDO: 256 para representação correta de 8 bits assinalados
            }
            return val;
        }

        // Converte Decimal de volta para binário de 8 bits (Complemento de 2)
        function signedIntToBin(val) {
            if (val < -128 || val > 127) return "OVERFLOW";
            if (val < 0) {
                val = 256 + val;
            }
            return val.toString(2).padStart(8, '0');
        }

        if (num1Str !== null && op !== null) {
            document.getElementById('num1').value = num1Str;
            if (num2Str) document.getElementById('num2').value = num2Str;

            const binRegex = /^[01]{8}$/;
            if (!binRegex.test(num1Str) || (!binRegex.test(num2Str))) {
                resultDisplay.innerText = "ERROR";
                resultDisplay.classList.add('overflow');
                return;
            }

            const n1 = binToSignedInt(num1Str);
            const n2 = (num2Str && num2Str !== "") ? binToSignedInt(num2Str) : 0;
            
            let decimalResult = 0;
            let isOverflow = false;

            switch(op) {
                case 'add':
                    decimalResult = n1 + n2;
                    break;
                case 'sub':
                    decimalResult = n1 - n2;
                    break;
                case 'mul':
                    decimalResult = n1 * n2;
                    break;
                case 'div': // MODIFICADO: Lógica de divisão com proteção contra overflow
                    if (n2 === 0) {
                        isOverflow = true; // Divisão por zero
                    } else if (n1 === -128 && n2 === -1) {
                        isOverflow = true; // -128 / -1 = +128 (Estoura o limite de +127)
                    } else {
                        decimalResult = Math.trunc(n1 / n2); // Trunca em direção ao zero idêntico ao C++
                    }
                    break;
                default:
                    return;
            }

            let finalOutput = "";
            if (isOverflow) {
                finalOutput = "OVERFLOW";
            } else {
                finalOutput = signedIntToBin(decimalResult);
            }

            resultDisplay.innerText = finalOutput;
            if (finalOutput === "OVERFLOW") {
                resultDisplay.classList.add('overflow');
            } else {
                resultDisplay.classList.remove('overflow');
            }
        }
    });
</script>

</body>
</html>
)rawliteral";

NetworkServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);

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

void send_html_page(NetworkClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  client.print(html_page);
  client.println();
}

void set_leds(uint16_t value){
  digitalWrite(4, value & 0x08);
  digitalWrite(5, value & 0x04);
  digitalWrite(6, value & 0x02);
  digitalWrite(7, value & 0x01);
}

void get_operation_params(const String& request, String* op, uint16_t* A, uint16_t* B){
  int indexA  = request.indexOf("a=");
  int indexB  = request.indexOf("&b=");
  int indexOp = request.indexOf("&op=");
  
  int indexEnd = request.indexOf(" ", indexOp);
  if (indexA != -1 && indexB != -1 && indexOp != -1) {
    String strA  = request.substring(indexA + 2, indexB);
    String strB  = request.substring(indexB + 3, indexOp);
    String strOp = request.substring(indexOp + 4, indexEnd != -1 ? indexEnd : request.length());
    strOp.trim();

    *A = strtol(strA.c_str(), NULL, 2);
    *B = strtol(strB.c_str(), NULL, 2);
    *op = strOp;
  }
}

void print_operation_params(const String& op, uint16_t A, uint16_t B){
  Serial.println("\n--- Nova Operacao Recebida ---");
  Serial.printf("Operacao: %s\n", op.c_str());
  Serial.print("A: ");
  Serial.println(A, BIN);
  Serial.print("B: ");
  Serial.println(B, BIN);
}

uint16_t handle_addition(uint16_t A, uint16_t B, bool* overflow){
  uint16_t result = A + B;
  *overflow = (((A & 0x80) == (B & 0x80)) && ((result & 0x80) != (A & 0x80)));
  return result;
}

uint16_t handle_multiplication(uint16_t A, uint16_t B, bool* overflow){
  uint16_t result = 0;
  if (B & 0x80){
    B = ((B ^ 0xFF) +1) & 0xFF;
    A = ((A ^ 0xFF) +1) & 0xFF;
  }
  
  uint32_t start_time = micros();
  for(int i = 0; i < B; ++i){
    result = handle_addition(result, A, overflow);
    if(*overflow) break;
  }

  uint32_t end_time = micros();
  uint32_t elapsed_time = end_time - start_time;
  Serial.printf("Multiplication Time: %u microseconds\n", elapsed_time);

  return result;
}

// MODIFICADO: Função de fatorial substituída por handle_division
uint16_t handle_division(uint16_t A, uint16_t B, bool* overflow){
  // Faz o cast dos valores brutos para inteiros de 8 bits assinalados nativos do C++
  int8_t signedA = (int8_t)(A & 0xFF);
  int8_t signedB = (int8_t)(B & 0xFF);
  
  uint32_t start_time = micros();

  // Proteção contra divisão por zero
  if (signedB == 0) {
    *overflow = true;
    return 0;
  }
  
  // Proteção contra o único estouro possível em divisão de complemento de dois: -128 / -1 = +128
  if (signedA == -128 && signedB == -1) {
    *overflow = true;
    return 0;
  }

  *overflow = false;
  int8_t result = signedA / signedB;

  uint32_t end_time = micros();
  uint32_t elapsed_time = end_time - start_time;
  Serial.printf("Division Time: %u microseconds\n", elapsed_time);

  return (uint16_t)(uint8_t)result;
}

uint16_t handle_operation(const String& op, uint16_t A, uint16_t B, bool* overflow){
  uint16_t result;
  if(op == "add"){
    result =  handle_addition(A, B, overflow);
  }
  else if(op == "sub"){
    B = ((B^0xFF) + 1) & 0xFF;
    result = handle_addition(A, B, overflow);
  }
  else if(op == "mul"){
    result = handle_multiplication(A, B, overflow);
  }
  else if(op == "div"){ // MODIFICADO: Redireciona para a nova função
    result = handle_division(A, B, overflow);
  }
  else {
    result = 0;
  }
  return result & 0xFF;
}

void loop() {
  NetworkClient client = server.accept();
  if(client){
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()){    
      if(client.available()){     
        char c = client.read();
        if(c == '\n'){            
          if(currentLine.indexOf("GET /calc?") >= 0){
            bool overflow = false;
            uint16_t A, B, result;
            String op;
            
            get_operation_params(currentLine, &op, &A, &B);
            print_operation_params(op, A, B);
            result = handle_operation(op, A, B, &overflow);
            Serial.print("Resultado: ");
            Serial.println(result, BIN);
            if(overflow){
              Serial.println("Ocorreu Overflow ou Erro na operacao!");
              set_leds(0x00);
            } else {
              set_leds(result);
            }
            Serial.println("-------------------------------\n");
          }
          if (currentLine.length() == 0) {
            send_html_page(client);
            break;  
          }
          else {  
            currentLine = "";
          }
        }
        else if (c != '\r') {  
          currentLine += c;
        }
      }
    } 
    client.stop();
    Serial.println("Client Disconnected.");
  }
}