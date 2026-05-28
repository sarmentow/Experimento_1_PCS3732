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
    <title>4-Bit Signed Binary Calculator</title>
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
            /* height: 120px; */
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
    <div class="result-field" id="result">0000</div>

    <form method="GET" action="">
        <div class="main-interface">
            <div class="inputs-column">
                <div>
                    <div class="label">Binary Num 1</div>
                    <input type="text" name="num1" id="num1" placeholder="0000" required title="Must be a 4-bit binary number (e.g., 0101)">
                </div>
                <div>
                    <div class="label">Binary Num 2</div>
                    <input type="text" name="num2" id="num2" placeholder="0000" title="Must be a 4-bit binary number (e.g., 1110)">
                </div>
            </div>
                <div>
                    <div class="label">Bits number</div>
                    <input type="text" name="bits" id="bits" maxlength="1" placeholder="0" title="Must be equal or less then 8">
                </div>
            <div class="buttons-square">
                <button type="submit" name="op" value="add" title="Addition">+</button>
                <button type="submit" name="op" value="sub" title="Subtraction">-</button>
                <button type="submit" name="op" value="mul" title="Multiplication">*</button>
                <button type="submit" name="op" value="fact" title="Factorial (Num 1 only)">!</button>
            </div>
        </div>
    </form>
</div>

<script>
    window.addEventListener('DOMContentLoaded', () => {
        // Parse GET parameters from URL
        const urlParams = new URLSearchParams(window.location.search);
        const num1Str = urlParams.get('num1');
        const num2Str = urlParams.get('num2');
        const op = urlParams.get('op');
        const bitsStr = urlParams.get('bits');

        const resultDisplay = document.getElementById('result');

        function getBitConfig(bitsValue) {
            const bits = bitsValue ? parseInt(bitsValue, 10) : 4;
            if (!Number.isInteger(bits) || bits < 1 || bits > 8) return null;

            const width = 1 << bits;
            return {
                bits,
                width,
                mask: width - 1,
                signMask: 1 << (bits - 1),
                min: -(1 << (bits - 1)),
                max: (1 << (bits - 1)) - 1
            };
        }

        // Helper: Convert n-bit signed binary (Two's Complement) to Decimal
        function binToSignedInt(bin, config) {
            const val = parseInt(bin, 2) & config.mask;
            return (val & config.signMask) ? val - config.width : val;
        }

        function signedIntToUnsigned(val, config) {
            return val & config.mask;
        }

        function unsignedToBin(val, config) {
            return (val & config.mask).toString(2).padStart(config.bits, '0');
        }

        function addBits(A, B, config) {
            const result = (A + B) & config.mask;
            const overflow = ((A & config.signMask) === (B & config.signMask)) &&
                             ((result & config.signMask) !== (A & config.signMask));
            return { result, overflow };
        }

        function twosComplement(value, config) {
            return ((value ^ config.mask) + 1) & config.mask;
        }

        function multiplyBits(A, B, config) {
            let result = 0;
            let overflow = false;

            if (B & config.signMask) {
                B = twosComplement(B, config);
                A = twosComplement(A, config);
            }

            for (let i = 0; i < B; i++) {
                const sum = addBits(result, A, config);
                result = sum.result;
                overflow = sum.overflow;
                if (overflow) break;
            }

            return { result, overflow };
        }

        // If the form was submitted via GET
        if (num1Str !== null && op !== null) {
            // Repopulate fields for seamless UX
            document.getElementById('num1').value = num1Str;
            if (num2Str) document.getElementById('num2').value = num2Str;
            if (bitsStr) document.getElementById('bits').value = bitsStr;

            const config = getBitConfig(bitsStr);

            // Simple validation regex
            const binRegex = config ? new RegExp(`^[01]{${config.bits}}$`) : null;
            if (!config || !binRegex.test(num1Str) || (op !== 'fact' && !binRegex.test(num2Str))) {
                resultDisplay.innerText = "ERROR";
                resultDisplay.classList.add('overflow');
                return;
            }

            const n1 = binToSignedInt(num1Str, config);
            const n2 = num2Str ? binToSignedInt(num2Str, config) : 0;
            const A = signedIntToUnsigned(n1, config);
            const B = signedIntToUnsigned(n2, config);
            
            let bitResult = 0;
            let isOverflow = false;

            // Perform chosen operation
            switch(op) {
                case 'add':
                    {
                        const result = addBits(A, B, config);
                        bitResult = result.result;
                        isOverflow = result.overflow;
                    }
                    break;
                case 'sub':
                    {
                        const result = addBits(A, twosComplement(B, config), config);
                        bitResult = result.result;
                        const decimalResult = n1 - n2;
                        isOverflow = decimalResult < config.min || decimalResult > config.max;
                    }
                    break;
                case 'mul':
                    {
                        const result = multiplyBits(A, B, config);
                        bitResult = result.result;
                        const decimalResult = n1 * n2;
                        isOverflow = result.overflow || decimalResult < config.min || decimalResult > config.max;
                    }
                    break;
                case 'fact':
                    if (n1 < 0) {
                        isOverflow = true;
                    } else {
                        bitResult = 1;
                        for (let i = 2; i <= n1; i++) {
                            const result = multiplyBits(bitResult, signedIntToUnsigned(i, config), config);
                            bitResult = result.result;
                            isOverflow = result.overflow;
                            if (isOverflow) break;
                        }
                    }
                    break;
                default:
                    return;
            }

            // Determine final output string
            let finalOutput = "";
            if (isOverflow) {
                finalOutput = "OVERFLOW";
            } else {
                finalOutput = unsignedToBin(bitResult, config);
            }

            // Display results
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

void set_leds(int16_t value){
  digitalWrite(18, value & 0x08);
  digitalWrite(19, value & 0x04);
  digitalWrite(22, value & 0x02);
  digitalWrite(23, value & 0x01);
}

void get_operation_params(const String& request, String* op, int16_t* A, int16_t* B, int16_t* bits){
  int indexA  = request.indexOf("a=");
  int indexB  = request.indexOf("&b=");
  int indexOp = request.indexOf("&op=");
  int opBits  = request.indexOf("&bits=");
  
  int indexEnd = request.indexOf(" ", indexOp); 

  if (indexA != -1 && indexB != -1 && indexOp != -1) {
    String strA  = request.substring(indexA + 2, indexB);
    String strB  = request.substring(indexB + 3, indexOp);
    String strOp = request.substring(indexOp + 4, indexEnd != -1 ? indexEnd : request.length());

    strOp.trim();

    *A = strtol(strA.c_str(), NULL, 2);
    *B = strtol(strB.c_str(), NULL, 2);
    *bits = strtol(strB.c_str(), NULL, 10);
    *op = strOp;
  }
}

void print_operation_params(const String& op, int16_t A, int16_t B){
  Serial.println("\n--- Nova Operacao Recebida ---");
  Serial.printf("Operacao: %s\n", op.c_str());
  Serial.print("A: ");
  Serial.println(A, BIN);
  Serial.print("B: ");
  Serial.println(B, BIN);
}

int16_t handle_addition(int16_t A, int16_t B, bool* overflow, int16_t bits){
  int16_t result = A + B;
  unsigned int16_t mask = 1U << (bits - 1);
  *overflow = (((A & mask) == (B & mask)) && ((result & mask) != (A & mask)));
  return result;
}

int16_t handle_multiplication(int16_t A, int16_t B, bool* overflow, int16_t bits){
  int16_t result = 0;
  unsigned int16_t mask = (1U << bits) - 1;
  if (B & 0x08){
    B = ((B ^ mask) +1) & mask;
    A = ((A ^ mask) +1) & mask;
  }
  for(int i = 0; i < B; ++i){
    result = handle_addition(result, A, overflow, bits);
    if(*overflow) break;
  }
  return result;
}

int16_t handle_factorial(int16_t A, bool* overflow, int16_t bits){
  int16_t result = 0;
  int16_t mask = 1 << (bits - 1);

  if((A & mask == mask) || (A == 0)){
    *overflow = false;
    return 0;
  }
  result = 1;
  for(int i = 2; i <= A; ++i){
    result = handle_multiplication(result, i, overflow, bits);
    if(*overflow) break;
  }
  return result;
}

int16_t handle_operation(const String& op, int16_t A, int16_t B, bool* overflow, int16_t bits){
  int16_t result;
  int16_t mask = 1 << (bits - 1);

  if(op == "add"){
    result =  handle_addition(A, B, overflow, bits);
  }
  else if(op == "sub"){
    B = ((B^mask) + 1) & mask;
    result = handle_addition(A, B, overflow, bits);
  }
  else if(op == "mul"){
    result = handle_multiplication(A, B, overflow, bits);
  }
  else{
    result = handle_factorial(A, overflow, bits);
  }
  return result & mask;
}

void loop() {
  NetworkClient client = server.accept();
  if(client){
    Serial.println("New Client.");
    String currentLine = "";

    while (client.connected()){    
      if(client.available()){     
        char c = client.read();
        Serial.write(c);    
        if(c == '\n'){            
          if(currentLine.indexOf("GET /calc?") >= 0){
            bool overflow = false;  
            int16_t A, B, result, bits;
            String op;
            
            get_operation_params(currentLine, &op, &A, &B, &bits);
            print_operation_params(op, A, B);
            result = handle_operation(op, A, B, &overflow);
            
            Serial.print("Resultado: ");
            Serial.println(result, BIN);
            if(overflow){
              Serial.println("Ocorreu Overflow na operacao!");
              set_leds(0x00);
            }
            set_leds(result);
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
