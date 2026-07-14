#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_HEIGHT, SCREEN_WIDTH, &Wire, OLED_RESET);

float dist = 0, pct = 0, nvl = 0;
unsigned long last = 0;

float ler() {
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long d = pulseIn(ECHO_PIN, HIGH, TIMEOUT_SENSOR);
    float r = d * SOUND_SPEED / 2;
    return (r == 0 || r > MAX_DISTANCE) ? -1 : r;
}

float media() {
    float s = 0; int v = 0;
    for (int i = 0; i < NUM_LEITURAS; i++) {
        float r = ler();
        if (r > 0) { s += r; v++; }
        delay(INTERVALO_LEITURA);
    }
    return v > 0 ? s / v : -1;
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
