#include <Arduino.h>
#include <stdint.h>

enum State {
  INITIAL,
  FIRST_OPERAND,
  SECOND_OPERAND,
  OPERATION,
  CALC
};

enum State curr_state = INITIAL;

uint8_t op1;
uint8_t op2;
bool opsum;

void printPrompt(const char *prompt) {
  Serial.println("------------------------------------");
  Serial.println(prompt);
}

void printBinary(uint16_t value, uint8_t bits) {
  for (int8_t bit = bits - 1; bit >= 0; bit--) {
    Serial.print((value >> bit) & 1);
  }
}

void printBinary8(uint8_t value) {
  printBinary(value, 8);
  Serial.println();
}

void printValue(const char *label, uint8_t value) {

    Serial.print(label);
    Serial.print(" binário: ");
    printBinary8(value);
    Serial.print(label);
    Serial.print(" decimal: ");
    if (value != 255) 
      Serial.println(value);
    else 
      Serial.println("0 (255)");


}

bool hasOverflow(uint8_t a, uint8_t b, uint8_t result) {
    bool a_negative = a & 0x80;
    bool b_negative = b & 0x80;
    bool result_negative = result & 0x80;

    return (a_negative == b_negative) && (a_negative != result_negative);
}

void printOverflow(bool overflow) {
    Serial.print("Overflow: ");
    Serial.println(overflow ? "sim" : "não");
}

void setup() {

  Serial.begin(115200);
  Serial.setTimeout(20);

}

void loop() {
  switch(curr_state){
    case INITIAL:
      Serial.println("Calculadora complemento de 1");
      printPrompt("Primeiro Operando");
      curr_state = FIRST_OPERAND;
    break;

    case (FIRST_OPERAND): {
      if (Serial.available() == 0) {
        return;
      }

      String operand1 = Serial.readStringUntil('\n');
      operand1.trim();
      op1 = (uint8_t)strtol(operand1.c_str(), NULL, 2);
      printValue("Primeiro Operando", op1);

      printPrompt("Segundo Operando:");
      curr_state = SECOND_OPERAND;
    }

    break;

    case (SECOND_OPERAND): {
      if (Serial.available() == 0) {
        return;
      }

      String operand2 = Serial.readStringUntil('\n');
      operand2.trim();
      op2 = (uint8_t)strtol(operand2.c_str(), NULL, 2);
      printValue("Segundo Operando:", op2);

      printPrompt("Operação 0 para adição e 1 para subtração");
      curr_state = OPERATION;
      }
    break;

    case (OPERATION):{
      if (Serial.available() == 0) {
        return;
      }

      String sum = Serial.readStringUntil('\n');
      sum.trim();
      opsum = (bool)((char)strtol(sum.c_str(), NULL, 2));

      curr_state = CALC;
    }
    break;

    case (CALC):{
        uint8_t operand2_value = opsum ? (uint8_t)~op2 : op2;
        uint16_t result = op1 + operand2_value;

        bool ov = result >> 8;

        uint8_t result_8 = result + ov;
        bool overflow = hasOverflow(op1, operand2_value, result_8);

        Serial.println("------------------------------------");
        printOverflow(overflow);
        Serial.print("Resultado decimal: ");
        Serial.println(result_8);
        Serial.print("Resultado binário: ");
        printBinary8(result_8);
        printPrompt("Primeiro Operando:");

        curr_state = FIRST_OPERAND;
    }
    break;

  }
}
