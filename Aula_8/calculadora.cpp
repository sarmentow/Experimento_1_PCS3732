#include <iostream>
#include <string>
#include <stdint.h>
#include <stdexcept>

using namespace std;

uint32_t mask_n_bits(int bits) {
    if (bits >= 32) {
        return 0xFFFFFFFFu;
    }

    return (1u << bits) - 1u;
}

uint32_t sign_bit(int bits) {
    return 1u << (bits - 1);
}

int32_t to_signed(uint32_t x, int bits) {
    uint32_t mask = mask_n_bits(bits);
    x &= mask;

    if (x & sign_bit(bits)) {
        return (int32_t)(x | ~mask);
    }

    return (int32_t)x;
}

uint32_t from_signed(int32_t x, int bits) {
    return ((uint32_t)x) & mask_n_bits(bits);
}

uint32_t parse_binary(const string& s, int bits) {
    if ((int)s.size() > bits) {
        throw runtime_error("Entrada possui mais bits do que o permitido.");
    }

    uint32_t value = 0;

    for (char c : s) {
        if (c != '0' && c != '1') {
            throw runtime_error("Entrada invalida. Use apenas 0 e 1.");
        }

        value = (value << 1) | (c - '0');
    }

    return value & mask_n_bits(bits);
}

void print_binary(uint32_t x, int bits) {
    x &= mask_n_bits(bits);

    for (int i = bits - 1; i >= 0; i--) {
        cout << ((x >> i) & 1u);
    }
}


uint32_t add_bits(uint32_t a, uint32_t b, int bits) {
    uint32_t mask = mask_n_bits(bits);

    a &= mask;
    b &= mask;

    while (b != 0) {
        uint32_t carry = (a & b) << 1;
        a = (a ^ b) & mask;
        b = carry & mask;
    }

    return a & mask;
}

uint32_t twos_complement(uint32_t x, int bits) {
    uint32_t mask = mask_n_bits(bits);

    return add_bits((~x) & mask, 1u, bits);
}

uint32_t sub_bits(uint32_t a, uint32_t b, int bits) {
    return add_bits(a, twos_complement(b, bits), bits);
}

uint32_t abs_bits(uint32_t x, int bits) {
    x &= mask_n_bits(bits);

    if (x & sign_bit(bits)) {
        return twos_complement(x, bits);
    }

    return x;
}

uint32_t mul_bits(uint32_t a, uint32_t b, int bits) {
    uint32_t mask = mask_n_bits(bits);

    a &= mask;
    b &= mask;

    bool neg_a = (a & sign_bit(bits)) != 0;
    bool neg_b = (b & sign_bit(bits)) != 0;
    bool result_negative = neg_a ^ neg_b;

    uint32_t abs_a = abs_bits(a, bits);
    uint32_t abs_b = abs_bits(b, bits);

    uint32_t result = 0;

    for (uint32_t i = 0; i < abs_b; i++) {
        result = add_bits(result, abs_a, bits);
    }

    if (result_negative) {
        result = twos_complement(result, bits);
    }

    return result & mask;
}

bool factorial_bits(uint32_t a, int bits, uint32_t* out) {
    a &= mask_n_bits(bits);

    if (a & sign_bit(bits)) {
        return false;
    }

    uint32_t n = a;
    uint32_t result = 1;

    for (uint32_t i = 2; i <= n; i++) {
        result = mul_bits(result, i, bits);
    }

    *out = result & mask_n_bits(bits);
    return true;
}

bool div_bits(uint32_t a, uint32_t b, int bits, uint32_t* quotient, uint32_t* remainder) {
    uint32_t mask = mask_n_bits(bits);

    a &= mask;
    b &= mask;

    if (b == 0) {
        return false;
    }

    bool neg_a = (a & sign_bit(bits)) != 0;
    bool neg_b = (b & sign_bit(bits)) != 0;
    bool quotient_negative = neg_a ^ neg_b;

    uint32_t dividend = abs_bits(a, bits);
    uint32_t divisor = abs_bits(b, bits);

    uint32_t q = 0;
    uint32_t r = 0;

    for (int i = bits - 1; i >= 0; i--) {
        r = (r << 1) & mask;
        r |= (dividend >> i) & 1u;

        if (to_signed(r, bits) >= to_signed(divisor, bits)) {
            r = sub_bits(r, divisor, bits);
            q |= (1u << i);
        }
    }

    if (quotient_negative) {
        q = twos_complement(q, bits);
    }

    /*
        Convenção usada:
        o resto mantém o sinal do dividendo.
    */
    if (neg_a) {
        r = twos_complement(r, bits);
    }

    *quotient = q & mask;
    *remainder = r & mask;

    return true;
}

bool overflow_add(uint32_t a, uint32_t b, uint32_t result, int bits) {
    bool sign_a = (a & sign_bit(bits)) != 0;
    bool sign_b = (b & sign_bit(bits)) != 0;
    bool sign_r = (result & sign_bit(bits)) != 0;

    /*
        Overflow na soma em complemento de 2 ocorre quando:
        - os operandos têm o mesmo sinal;
        - o resultado tem sinal diferente.
    */
    return (sign_a == sign_b) && (sign_a != sign_r);
}

bool overflow_sub(uint32_t a, uint32_t b, uint32_t result, int bits) {
    bool sign_a = (a & sign_bit(bits)) != 0;
    bool sign_b = (b & sign_bit(bits)) != 0;
    bool sign_r = (result & sign_bit(bits)) != 0;

    /*
        Overflow na subtração A - B ocorre quando:
        - A e B têm sinais diferentes;
        - o resultado tem sinal diferente de A.
    */
    return (sign_a != sign_b) && (sign_a != sign_r);
}

// ============================================================
// Interface da calculadora
// ============================================================

void print_value(const string& name, uint32_t x, int bits) {
    cout << name << " = ";
    print_binary(x, bits);
    cout << " = " << to_signed(x, bits) << endl;
}

void run_calculator() {
    const int bits = 4;

    cout << "\n========================================\n";
    cout << "Calculadora binaria bit a bit - 4 bits\n";
    cout << "Representacao: complemento de 2\n";
    cout << "Faixa: -8 ate 7\n";
    cout << "========================================\n\n";

    cout << "Operacoes:\n";
    cout << "1 - Soma\n";
    cout << "2 - Subtracao\n";
    cout << "3 - Multiplicacao por somas sucessivas\n";
    cout << "4 - Fatorial\n";
    cout << "5 - Divisao inteira\n";
    cout << "0 - Sair\n\n";

    cout << "Escolha: ";

    int op;
    cin >> op;

    if (op == 0) {
        return;
    }

    string sa;
    string sb;

    uint32_t a;
    uint32_t b = 0;
    uint32_t result;

    cout << "Digite A em binario com ate 4 bits: ";
    cin >> sa;

    a = parse_binary(sa, bits);

    if (op != 4) {
        cout << "Digite B em binario com ate 4 bits: ";
        cin >> sb;

        b = parse_binary(sb, bits);
    }

    cout << "\nEntradas interpretadas em complemento de 2:\n";
    print_value("A", a, bits);

    if (op != 4) {
        print_value("B", b, bits);
    }

    cout << "\nResultado:\n";

    switch (op) {
        case 1:
            result = add_bits(a, b, bits);
            print_value("A + B", result, bits);

            if (overflow_add(a, b, result, bits)) {
                cout << "Aviso: ocorreu overflow na soma em 4 bits.\n";
            }

            break;

        case 2:
            result = sub_bits(a, b, bits);
            print_value("A - B", result, bits);

            if (overflow_sub(a, b, result, bits)) {
                cout << "Aviso: ocorreu overflow na subtracao em 4 bits.\n";
            }

            break;

        case 3:
            result = mul_bits(a, b, bits);
            print_value("A * B", result, bits);
            cout << "Observacao: a multiplicacao foi feita por loop de somas sucessivas.\n";
            cout << "Aviso: se o resultado matematico nao couber em 4 bits, o valor exibido fica truncado.\n";
            break;

        case 4:
            if (factorial_bits(a, bits, &result)) {
                print_value("A!", result, bits);
                cout << "Aviso: se o resultado matematico nao couber em 4 bits, o valor exibido fica truncado.\n";
            } else {
                cout << "Erro: fatorial nao e definido para numero negativo.\n";
            }

            break;

        case 5: {
            uint32_t q;
            uint32_t r;

            if (!div_bits(a, b, bits, &q, &r)) {
                cout << "Erro: divisao por zero.\n";
                cout << "Tratamento: a operacao e recusada antes do calculo.\n";
            } else {
                print_value("Quociente", q, bits);
                print_value("Resto", r, bits);
            }

            break;
        }

        default:
            cout << "Operacao invalida.\n";
            break;
    }
}

int main() {
    while (true) {
        run_calculator();

        cout << "\nDeseja realizar outra operacao? (s/n): ";

        char answer;
        cin >> answer;

        if (answer != 's' && answer != 'S') {
            cout << "Encerrando.\n";
            break;
        }
    }

    return 0;
}
