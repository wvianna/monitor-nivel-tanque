# 📊 Monitor de Nível de Tanque

Sistema completo de monitoramento de nível de líquido em tanques usando Arduino Uno, sensor ultrassônico HC-SR04 e display OLED MC01506 1.5".

## Caracteristicas

- Leitura precisa de distancia com sensor HC-SR04
- Conversao automatica de distancia para nivel percentual
- Display OLED MC01506 1.5" com interface grafica
- Barra visual de nivel em tempo real
- Sistema de alertas (Critico, Baixo, Medio, Alto)
- Monitoramento via Serial Monitor (115200 baud)
- Media de multiplas leituras para maior precisao
- Endereco I2C do display confirmado: **0x3C**

## Hardware Necessario

| Componente | Especificacao |
|------------|---------------|
| Microcontrolador | Arduino Uno (porta /dev/ttyUSB0) |
| Sensor Ultrassonico | HC-SR04 |
| Display | OLED MC01506 1.5" I2C (128x64) |
| Cabos | Jumpers macho-macho e macho-femea |

## Diagrama de Conexoes

### Sensor HC-SR04

```
HC-SR04          Arduino Uno
--------         -----------
VCC             5V
TRIG            Pino 9 (Digital)
ECHO            Pino 10 (Digital)
GND             GND
```

### Display OLED MC01506 (I2C)

```
Display OLED     Arduino Uno
------------     -----------
VCC             5V ou 3.3V
GND             GND
SDA             A4 (Analogico 4)
SCL             A5 (Analogico 5)
```

## Instalacao

### Compilar e fazer upload

```bash
pio run --target upload
```

### Monitor serial

```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```

## Configuracao (include/config.h)

### Parametros do Tanque

```cpp
#define ALTURA_TANQUE 200       // Altura total do tanque (cm)
#define DISTANCIA_SENSOR 10     // Distancia sensor ate nivel maximo (cm)
```

### Niveis de Alerta

```cpp
#define NIVEL_CRITICO 10        // < 10%
#define NIVEL_BAIXO 25          // 10-25%
#define NIVEL_MEDIO 50          // 25-50%
#define NIVEL_ALTO 75           // > 75%
```

### Display (ja configurado)

```cpp
#define SCREEN_ADDRESS 0x3C     // Confirmado via scanner I2C
```

## Interface do Display

```
+-------------------------+
| MONITOR DE NIVEL        |
+-------------------------+
| 87.5%              #### |
|                    #### |
| Nivel: 175.0 cm    #### |
| Dist: 35.0 cm      #### |
| Status: ALTO       #### |
+-------------------------+
```

## Notas

- Display MC01506 1.5" usa driver SSD1306, I2C endereco **0x3C**
- O sensor HC-SR04 deve ser instalado no topo do tanque, apontando para baixo
