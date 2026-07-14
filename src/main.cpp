#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_HEIGHT, SCREEN_WIDTH, &Wire, OLED_RESET);

float dist = 0, pct = 0, nvl = 0;
unsigned long last = 0;

float ler() {
    // Garante nivel logico baixo antes do pulso
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(5);
    // Pulso de trigger de 10us
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    // pulseInLong eh mais preciso no ESP8266 (evita interferencia de IRQ)
    long d = pulseInLong(ECHO_PIN, HIGH, TIMEOUT_SENSOR);
    if (d == 0) return -1;
    float r = d * SOUND_SPEED / 2.0;
    return (r > MAX_DISTANCE) ? -1 : r;
}

float media() {
    // Coleta amostras e rejeita outliers (descarta maior e menor)
    float amostras[NUM_LEITURAS];
    int n = 0;
    for (int i = 0; i < NUM_LEITURAS; i++) {
        float r = ler();
        if (r > 0) amostras[n++] = r;
        delay(INTERVALO_LEITURA);
    }
    if (n < 2) return (n == 1) ? amostras[0] : -1;
    // Ordena para encontrar mediana
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (amostras[i] > amostras[j]) {
                float t = amostras[i]; amostras[i] = amostras[j]; amostras[j] = t;
            }
        }
    }
    // Se tem 3+, descarta maior e menor (outliers)
    int inicio = (n > 2) ? 1 : 0;
    int fim = (n > 2) ? n - 1 : n;
    float soma = 0;
    for (int i = inicio; i < fim; i++) soma += amostras[i];
    return soma / (fim - inicio);
}

const char* sts(float p) {
    if (p >= NIVEL_ALTO) return "ALTO";
    if (p >= NIVEL_MEDIO) return "MEDIO";
    if (p >= NIVEL_BAIXO) return "BAIXO";
    if (p >= NIVEL_CRITICO) return "CRITICO";
    return "VAZIO";
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nMonitor Tanque v6.0 - SH1107 OK!");

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(100000);

    if(!display.begin(SCREEN_ADDRESS, true)) {
        Serial.println("FALHA display");
        while(1);
    }
    Serial.println("Display OK!");

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(5, 50);
    display.println("Monitor");
    display.setCursor(5, 75);
    display.println("Nivel OK!");
    display.display();
    delay(2000);

    Serial.println("Pronto!\n");
    last = millis();
}

void loop() {
    unsigned long now = millis();
    if (now - last >= INTERVALO_ATUALIZACAO) {
        last = now;
        dist = media();
        if (dist > 0) {
            float nr = ALTURA_TANQUE - (dist - DISTANCIA_SENSOR);
            if (nr < 0) nr = 0;
            if (nr > ALTURA_TANQUE) nr = ALTURA_TANQUE;
            pct = (nr / ALTURA_TANQUE) * 100.0;
            nvl = nr;
            Serial.printf("Dist: %.2f | Nivel: %.2f (%.0f%%)\n", dist, nvl, pct);
        } else Serial.println("ERRO: sensor");

        display.clearDisplay();
        display.setTextColor(SH110X_WHITE);

        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("MONITOR DE NIVEL");
        display.drawLine(0, 10, 128, 10, SH110X_WHITE);

        display.setTextSize(3);
        display.setCursor(0, 16);
        display.print(pct, 1);
        display.println("%");

        display.setTextSize(1);
        display.setCursor(0, 52);
        display.print("Nivel: "); display.print(nvl, 1); display.println(" cm");
        display.setCursor(0, 66);
        display.print("Dist: "); display.print(dist, 1); display.println(" cm");
        display.setCursor(0, 80);
        display.print("Status: "); display.println(sts(pct));

        display.drawRect(98, 14, 24, 106, SH110X_WHITE);
        int hp = (int)((pct / 100.0) * 102);
        if (hp > 0) display.fillRect(100, 118 - hp, 20, hp, SH110X_WHITE);

        display.display();
    }
}
