/**
 * @file config.h
 * @brief Configurações do sistema de monitoramento de nível
 * 
 * Define os pinos, parâmetros do tanque e configurações do display
 * Display: MC01506 1.5" SH1107 128x128 I2C 0x3C - NodeMCU ESP8266
 */

#ifndef CONFIG_H
#define CONFIG_H

// ========== CONFIGURAÇÕES DO SENSOR HC-SR04 ==========
#define TRIG_PIN 14             // Pino Trigger HC-SR04 (NodeMCU D5 = GPIO14)
#define ECHO_PIN 12             // Pino Echo HC-SR04 (NodeMCU D6 = GPIO12)
#define MAX_DISTANCE 400        // Distância máxima de medição (cm)
#define SOUND_SPEED 0.034       // Velocidade do som em cm/μs

// ========== CONFIGURAÇÕES DO TANQUE ==========
#define ALTURA_TANQUE 200       // Altura total do tanque em cm (AJUSTAR CONFORME SEU TANQUE)
#define DISTANCIA_SENSOR 10     // Distância do sensor até o topo do líquido quando cheio (cm)
#define NIVEL_MINIMO 5          // Nível mínimo considerado (cm)

// ========== CONFIGURAÇÕES DO DISPLAY OLED ==========
#define SCREEN_WIDTH 128        // Largura do display OLED em pixels
#define SCREEN_HEIGHT 128       // Altura do display OLED em pixels (SH1107 1.5")
#define OLED_RESET -1           // Pino de reset (não usado)
#define SCREEN_ADDRESS 0x3C     // Endereço I2C do display (confirmado via scanner)
#define PIN_SDA 4               // SDA (NodeMCU D2 = GPIO4)
#define PIN_SCL 5               // SCL (NodeMCU D1 = GPIO5)

// ========== CONFIGURAÇÕES DE LEITURA ==========
#define NUM_LEITURAS 5          // Número de leituras para média
#define INTERVALO_LEITURA 100   // Intervalo entre leituras em ms
#define TIMEOUT_SENSOR 30000    // Timeout para leitura do sensor (μs)

// ========== CONFIGURAÇÕES DE ATUALIZAÇÃO ==========
#define INTERVALO_ATUALIZACAO 500  // Intervalo de atualização do display (ms)

// ========== NÍVEIS DE ALERTA ==========
#define NIVEL_CRITICO 10        // Percentual considerado crítico (%)
#define NIVEL_BAIXO 25          // Percentual considerado baixo (%)
#define NIVEL_MEDIO 50          // Percentual considerado médio (%)
#define NIVEL_ALTO 75           // Percentual considerado alto (%)

#endif // CONFIG_H
