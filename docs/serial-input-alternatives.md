# Alternativas de Comunicación: I2C y SPI

Si en el futuro necesitas usar I2C o SPI en lugar de USB Serial, aquí están las pautas de implementación:

## Opción I2C

### Ventajas
- Bajo número de pines (2: SDA + SCL)
- Múltiples dispositivos en el mismo bus
- Bien soportado por el RP2040

### Desventajas
- Velocidad limitada (~400 kHz típico)
- Requiere hardware externo
- Más complejo de implementar

### Ejemplo de Implementación

```cpp
// En serial_input.h
#define SERIAL_INPUT_I2C_ADDRESS 0x42
#define SERIAL_INPUT_I2C_SDA_PIN 4
#define SERIAL_INPUT_I2C_SCL_PIN 5

// En serial_input.cpp
#include "hardware/i2c.h"

void SerialInputAddon::setup() {
    // Inicializar I2C
    i2c_init(i2c0, 400 * 1000);  // 400 kHz
    gpio_set_function(SERIAL_INPUT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SERIAL_INPUT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SERIAL_INPUT_I2C_SDA_PIN);
    gpio_pull_up(SERIAL_INPUT_I2C_SCL_PIN);
}

void SerialInputAddon::preprocess() {
    uint8_t buffer[4];
    
    // Leer 4 bytes del dispositivo I2C
    int bytes_read = i2c_read_blocking(
        i2c0, 
        SERIAL_INPUT_I2C_ADDRESS, 
        buffer, 
        4, 
        false
    );
    
    if (bytes_read == 4) {
        uint32_t inputMask = (uint32_t)buffer[0]
                           | ((uint32_t)buffer[1] << 8)
                           | ((uint32_t)buffer[2] << 16)
                           | ((uint32_t)buffer[3] << 24);
        processInputBitmask(inputMask);
    }
}
```

### Ejemplo de Dispositivo Maestro (Arduino)

```cpp
#include <Wire.h>

#define I2C_ADDRESS 0x42

void setup() {
    Wire.begin(I2C_ADDRESS);
    Wire.onRequest(sendData);
}

void loop() {
    // Tu lógica para generar button_mask
}

void sendData() {
    uint32_t button_mask = getButtonState();
    Wire.write((uint8_t*)&button_mask, 4);
}
```

## Opción SPI

### Ventajas
- Muy rápida (varios MHz)
- Full-duplex (enviar y recibir simultáneamente)
- Simple protocolo

### Desventajas
- Más pines requeridos (4: MOSI, MISO, SCK, CS)
- Requiere hardware externo
- Configuración más compleja

### Ejemplo de Implementación

```cpp
// En serial_input.h
#define SERIAL_INPUT_SPI_RX_PIN  16  // MISO
#define SERIAL_INPUT_SPI_CS_PIN  17  // Chip Select
#define SERIAL_INPUT_SPI_SCK_PIN 18  // Clock
#define SERIAL_INPUT_SPI_TX_PIN  19  // MOSI (si se necesita)

// En serial_input.cpp
#include "hardware/spi.h"

void SerialInputAddon::setup() {
    // Inicializar SPI
    spi_init(spi0, 1000 * 1000);  // 1 MHz
    
    gpio_set_function(SERIAL_INPUT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SERIAL_INPUT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SERIAL_INPUT_SPI_TX_PIN, GPIO_FUNC_SPI);
    
    // CS como GPIO normal
    gpio_init(SERIAL_INPUT_SPI_CS_PIN);
    gpio_set_dir(SERIAL_INPUT_SPI_CS_PIN, GPIO_IN);
    gpio_pull_up(SERIAL_INPUT_SPI_CS_PIN);
}

void SerialInputAddon::preprocess() {
    uint8_t buffer[4];
    
    // Activar CS (activo bajo)
    gpio_put(SERIAL_INPUT_SPI_CS_PIN, 0);
    
    // Leer 4 bytes
    spi_read_blocking(spi0, 0, buffer, 4);
    
    // Desactivar CS
    gpio_put(SERIAL_INPUT_SPI_CS_PIN, 1);
    
    uint32_t inputMask = (uint32_t)buffer[0]
                       | ((uint32_t)buffer[1] << 8)
                       | ((uint32_t)buffer[2] << 16)
                       | ((uint32_t)buffer[3] << 24);
    
    processInputBitmask(inputMask);
}
```

### Ejemplo de Dispositivo Maestro (Arduino)

```cpp
#include <SPI.h>

#define CS_PIN 10

void setup() {
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    SPI.begin();
}

void loop() {
    uint32_t button_mask = getButtonState();
    
    digitalWrite(CS_PIN, LOW);
    SPI.transfer((uint8_t*)&button_mask, 4);
    digitalWrite(CS_PIN, HIGH);
    
    delay(8);  // ~120 Hz
}
```

## Comparación

| Característica | USB Serial | I2C | SPI |
|---------------|------------|-----|-----|
| Velocidad | Alta (115200 baud) | Media (400 kHz) | Muy Alta (varios MHz) |
| Pines requeridos | 0 (USB) | 2 | 4 |
| Hardware externo | No | Sí | Sí |
| Complejidad | Baja | Media | Media-Alta |
| Múltiples dispositivos | No | Sí | Sí (con más CS) |
| Distancia | Corta (USB cable) | Corta (~30cm) | Muy corta (<10cm) |
| Latencia | ~8ms | ~2ms | <1ms |

## Recomendación

Para tu caso de uso (120 inputs/segundo, simplicidad), **USB Serial (CDC) es la mejor opción** porque:

1. ✅ No requiere hardware adicional
2. ✅ Fácil de debuggear
3. ✅ Velocidad suficiente para 120 Hz
4. ✅ Compatible con cualquier PC/Raspberry Pi
5. ✅ Ya implementado y funcionando

I2C o SPI solo serían necesarios si:
- Necesitas conectar el GP2040-CE a otro microcontrolador (no PC)
- Necesitas latencia extremadamente baja (<1ms)
- Tienes requisitos especiales de hardware

## Hybrid Approach (Avanzado)

Podrías hacer un addon que soporte múltiples backends:

```cpp
enum class SerialInputMode {
    USB_CDC,
    I2C_SLAVE,
    SPI_SLAVE
};

class SerialInputAddon : public GPAddon {
private:
    SerialInputMode mode;
    
    void readUSB();
    void readI2C();
    void readSPI();
    
public:
    void preprocess() {
        switch(mode) {
            case SerialInputMode::USB_CDC:
                readUSB();
                break;
            case SerialInputMode::I2C_SLAVE:
                readI2C();
                break;
            case SerialInputMode::SPI_SLAVE:
                readSPI();
                break;
        }
    }
};
```

Esto permitiría seleccionar el método de comunicación via config, pero añadiría complejidad innecesaria para la mayoría de casos de uso.
