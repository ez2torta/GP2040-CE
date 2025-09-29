# ESP32 como Wii Extension Bridge para GP2040-CE

## Resumen

Este documento describe cómo usar un **ESP32** como "puente" que simule un **Wii Classic Controller** para enviar datos de control personalizados al GP2040-CE a través de **I2C**. Esta aproximación permite usar el add-on Wii Extension existente sin modificaciones.

## Arquitectura del Sistema

```
Fuente de Datos → ESP32 (I2C Slave) → GP2040-CE (I2C Master) → Gamepad USB
(Serial, WiFi,    (Simula Wii         (Wii Extension      
 etc.)             Classic Controller)  Add-on)
```

## Protocolo Wii Extension I2C

### Parámetros de Comunicación I2C

```cpp
// Configuración I2C
#define WII_EXTENSION_I2C_ADDR    0x52      // Dirección I2C del dispositivo
#define I2C_FREQUENCY             400000    // 400kHz
#define DATA_PACKET_SIZE          6         // Para DATA_TYPE_1 (más compacto)
// Alternativamente:
#define DATA_PACKET_SIZE          8         // Para DATA_TYPE_2 (más resolución)
```

### Secuencia de Inicialización

El GP2040-CE realiza esta secuencia al detectar un dispositivo Wii:

1. **Desactivar Encriptación**: Escribe `0x55` al registro `0xF0`
2. **Configurar Formato**: Escribe `0x00` al registro `0xFB`
3. **Establecer Tipo de Dato**: Escribe `0x03` al registro `0xFE` (DATA_TYPE_1)
4. **Lectura de ID**: Lee 6 bytes desde registro `0xFA` para identificar el dispositivo
5. **Lectura de Calibración**: Lee 16 bytes desde registro `0x20` (opcional)
6. **Lectura de Datos**: Lee continuamente desde registro `0x00`

### Identificación del Dispositivo

Para que GP2040-CE reconozca el dispositivo como **Classic Controller**, el ESP32 debe responder:

```cpp
// Respuesta a lectura de registro 0xFA (ID del dispositivo)
uint8_t device_id[6] = {
    0x00, 0x00, 0xA4, 0x20, 0x01, 0x01    // Classic Controller ID
};
```

### Formato de Datos - Classic Controller (DATA_TYPE_1)

El Classic Controller envía **6 bytes** cada vez que se lee el registro `0x00`:

```cpp
// Estructura del paquete de datos (6 bytes)
struct WiiClassicData {
    uint8_t byte0;  // Left stick X (6 bits)
    uint8_t byte1;  // Left stick Y (6 bits) 
    uint8_t byte2;  // Right stick data + Left trigger (bits 7-5)
    uint8_t byte3;  // Right trigger (5 bits) + Left trigger (bits 7-5)
    uint8_t byte4;  // Botones: Right, Down, L, Minus, Home, Plus, R, [bit0 no usado]
    uint8_t byte5;  // Botones: ZL, B, Y, A, X, ZR, Left, Up
};
```

### Decodificación de Bits (DATA_TYPE_1)

**Byte 0**: `[LX5][LX4][LX3][LX2][LX1][LX0][RX4][RX5]`
**Byte 1**: `[LY5][LY4][LY3][LY2][LY1][LY0][RX3][RX2]`
**Byte 2**: `[RX1][LT4][LT3][LT2][LT1][LT0][RY4][RY3]`
**Byte 3**: `[LT4][LT3][LT2][RT4][RT3][RT2][RT1][RT0]`
**Byte 4**: `[DR][DD][DL][MIN][HOME][PLUS][R][x]`
**Byte 5**: `[ZL][B][Y][A][X][ZR][DL][UP]`

Donde:
- `LX`/`LY` = Left stick X/Y (6 bits cada uno)
- `RX`/`RY` = Right stick X/Y (5 bits cada uno)  
- `LT`/`RT` = Left/Right trigger (5 bits cada uno)
- Botones = 1 bit cada uno (0=presionado, 1=no presionado)

## Implementación en ESP32

### Código ESP32 (Arduino IDE)

```cpp
#include <Wire.h>

// Configuración I2C
#define I2C_SLAVE_ADDR    0x52
#define SDA_PIN           21
#define SCL_PIN           22

// Estructura de datos del Classic Controller
struct WiiClassicData {
    uint8_t leftStickX;      // 0-63 (6 bits)
    uint8_t leftStickY;      // 0-63 (6 bits)
    uint8_t rightStickX;     // 0-31 (5 bits)
    uint8_t rightStickY;     // 0-31 (5 bits)
    uint8_t leftTrigger;     // 0-31 (5 bits)
    uint8_t rightTrigger;    // 0-31 (5 bits)
    
    // Botones (true = presionado)
    bool btnA, btnB, btnX, btnY;
    bool btnL, btnR, btnZL, btnZR;
    bool btnPlus, btnMinus, btnHome;
    bool dpadUp, dpadDown, dpadLeft, dpadRight;
};

// Variables globales
WiiClassicData gamepadData;
uint8_t currentRegister = 0x00;
bool deviceInitialized = false;

// IDs y datos de respuesta
const uint8_t DEVICE_ID[6] = {0x00, 0x00, 0xA4, 0x20, 0x01, 0x01};
const uint8_t CALIBRATION_DATA[16] = {
    // Calibración por defecto para Classic Controller
    0xFF, 0x00, 0x80,  // Left stick max, min, center
    0xFF, 0x00, 0x80,  // Left stick max, min, center  
    0x1F, 0x00, 0x0F,  // Right stick max, min, center
    0x1F, 0x00, 0x0F,  // Right stick max, min, center
    0x1F, 0x1F,        // Triggers max
    0x00, 0x00         // Padding
};

void setup() {
    Serial.begin(115200);
    
    // Inicializar I2C como slave
    Wire.begin(SDA_PIN, SCL_PIN, I2C_SLAVE_ADDR);
    Wire.onReceive(onI2CReceive);
    Wire.onRequest(onI2CRequest);
    
    // Inicializar datos del gamepad con valores centrados
    initGamepadData();
    
    Serial.println("ESP32 Wii Extension Bridge iniciado");
    Serial.println("Dirección I2C: 0x52");
}

void loop() {
    // Procesamiento continuo para mínima latencia
    updateGamepadData();
    
    // Micro-delay opcional para estabilidad (solo si es necesario)
    delayMicroseconds(100);  // 0.1ms - mucho mejor que delay(10)
}

void initGamepadData() {
    gamepadData.leftStickX = 32;    // Centro (0-63)
    gamepadData.leftStickY = 32;    // Centro (0-63)
    gamepadData.rightStickX = 16;   // Centro (0-31)
    gamepadData.rightStickY = 16;   // Centro (0-31)
    gamepadData.leftTrigger = 0;    // Sin presionar (0-31)
    gamepadData.rightTrigger = 0;   // Sin presionar (0-31)
    
    // Todos los botones liberados
    gamepadData.btnA = false;
    gamepadData.btnB = false;
    gamepadData.btnX = false;
    gamepadData.btnY = false;
    gamepadData.btnL = false;
    gamepadData.btnR = false;
    gamepadData.btnZL = false;
    gamepadData.btnZR = false;
    gamepadData.btnPlus = false;
    gamepadData.btnMinus = false;
    gamepadData.btnHome = false;
    gamepadData.dpadUp = false;
    gamepadData.dpadDown = false;
    gamepadData.dpadLeft = false;
    gamepadData.dpadRight = false;
}

void updateGamepadData() {
    // EJEMPLO: Lee datos del puerto serial
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        parseSerialCommand(command);
    }
    
    // Aquí podrías agregar:
    // - Lectura WiFi/TCP
    // - Lectura de sensores
    // - Procesamiento de datos de otro microcontrolador
}

void parseSerialCommand(String cmd) {
    // Formato ejemplo: "BTN:A:1" o "STICK:LX:45" o "TRIGGER:L:20"
    
    if (cmd.startsWith("BTN:")) {
        // Parsear botones: BTN:A:1 (A presionado)
        int firstColon = cmd.indexOf(':', 4);
        int secondColon = cmd.indexOf(':', firstColon + 1);
        
        if (firstColon != -1 && secondColon != -1) {
            String button = cmd.substring(4, firstColon);
            int value = cmd.substring(secondColon + 1).toInt();
            bool pressed = (value == 1);
            
            setButton(button, pressed);
        }
    }
    else if (cmd.startsWith("STICK:")) {
        // Parsear sticks: STICK:LX:32 (Left X al centro)
        int firstColon = cmd.indexOf(':', 6);
        int secondColon = cmd.indexOf(':', firstColon + 1);
        
        if (firstColon != -1 && secondColon != -1) {
            String stick = cmd.substring(6, firstColon);
            int value = cmd.substring(secondColon + 1).toInt();
            
            setStick(stick, value);
        }
    }
    else if (cmd.startsWith("TRIGGER:")) {
        // Parsear triggers: TRIGGER:L:15
        int firstColon = cmd.indexOf(':', 8);
        int secondColon = cmd.indexOf(':', firstColon + 1);
        
        if (firstColon != -1 && secondColon != -1) {
            String trigger = cmd.substring(8, firstColon);
            int value = cmd.substring(secondColon + 1).toInt();
            
            setTrigger(trigger, value);
        }
    }
}

void setButton(String button, bool pressed) {
    if (button == "A") gamepadData.btnA = pressed;
    else if (button == "B") gamepadData.btnB = pressed;
    else if (button == "X") gamepadData.btnX = pressed;
    else if (button == "Y") gamepadData.btnY = pressed;
    else if (button == "L") gamepadData.btnL = pressed;
    else if (button == "R") gamepadData.btnR = pressed;
    else if (button == "ZL") gamepadData.btnZL = pressed;
    else if (button == "ZR") gamepadData.btnZR = pressed;
    else if (button == "PLUS") gamepadData.btnPlus = pressed;
    else if (button == "MINUS") gamepadData.btnMinus = pressed;
    else if (button == "HOME") gamepadData.btnHome = pressed;
    else if (button == "UP") gamepadData.dpadUp = pressed;
    else if (button == "DOWN") gamepadData.dpadDown = pressed;
    else if (button == "LEFT") gamepadData.dpadLeft = pressed;
    else if (button == "RIGHT") gamepadData.dpadRight = pressed;
    
    Serial.println("Button " + button + ": " + (pressed ? "ON" : "OFF"));
}

void setStick(String stick, int value) {
    if (stick == "LX") gamepadData.leftStickX = constrain(value, 0, 63);
    else if (stick == "LY") gamepadData.leftStickY = constrain(value, 0, 63);
    else if (stick == "RX") gamepadData.rightStickX = constrain(value, 0, 31);
    else if (stick == "RY") gamepadData.rightStickY = constrain(value, 0, 31);
    
    Serial.println("Stick " + stick + ": " + String(value));
}

void setTrigger(String trigger, int value) {
    if (trigger == "L") gamepadData.leftTrigger = constrain(value, 0, 31);
    else if (trigger == "R") gamepadData.rightTrigger = constrain(value, 0, 31);
    
    Serial.println("Trigger " + trigger + ": " + String(value));
}

// Callback cuando el master (GP2040-CE) envía datos
void onI2CReceive(int numBytes) {
    if (numBytes >= 2) {
        uint8_t reg = Wire.read();
        uint8_t value = Wire.read();
        
        handleRegisterWrite(reg, value);
        
        // Descartar bytes adicionales
        while (Wire.available()) {
            Wire.read();
        }
    }
}

// Callback cuando el master (GP2040-CE) solicita datos
void onI2CRequest() {
    uint8_t response[16];
    int responseSize = 0;
    
    switch (currentRegister) {
        case 0x00:  // Datos del gamepad
            responseSize = encodeGamepadData(response);
            break;
            
        case 0xFA:  // ID del dispositivo
            memcpy(response, DEVICE_ID, 6);
            responseSize = 6;
            break;
            
        case 0x20:  // Datos de calibración
            memcpy(response, CALIBRATION_DATA, 16);
            responseSize = 16;
            break;
            
        default:
            // Respuesta por defecto
            memset(response, 0xFF, 6);
            responseSize = 6;
            break;
    }
    
    Wire.write(response, responseSize);
}

void handleRegisterWrite(uint8_t reg, uint8_t value) {
    switch (reg) {
        case 0xF0:  // Desactivar encriptación
            if (value == 0x55) {
                Serial.println("Encriptación desactivada");
            }
            break;
            
        case 0xFB:  // Configurar formato
            if (value == 0x00) {
                Serial.println("Formato configurado");
            }
            break;
            
        case 0xFE:  // Establecer tipo de dato
            if (value == 0x03) {
                Serial.println("Tipo de dato establecido: DATA_TYPE_1");
                deviceInitialized = true;
            }
            break;
            
        default:
            // Cambiar registro actual para próxima lectura
            currentRegister = reg;
            break;
    }
}

int encodeGamepadData(uint8_t* buffer) {
    // Codificar según formato DATA_TYPE_1 del Classic Controller
    
    buffer[0] = (gamepadData.leftStickX & 0x3F) | 
                ((gamepadData.rightStickX & 0x18) << 1);
                
    buffer[1] = (gamepadData.leftStickY & 0x3F) | 
                ((gamepadData.rightStickX & 0x06) << 5);
                
    buffer[2] = ((gamepadData.rightStickX & 0x01) << 7) |
                ((gamepadData.leftTrigger & 0x18) << 2) |
                (gamepadData.rightStickY & 0x1F);
                
    buffer[3] = ((gamepadData.leftTrigger & 0x07) << 5) |
                (gamepadData.rightTrigger & 0x1F);
    
    // Botones (invertidos: 0=presionado, 1=no presionado)
    buffer[4] = (!gamepadData.dpadRight << 7) |
                (!gamepadData.dpadDown << 6) |
                (!gamepadData.btnL << 5) |
                (!gamepadData.btnMinus << 4) |
                (!gamepadData.btnHome << 3) |
                (!gamepadData.btnPlus << 2) |
                (!gamepadData.btnR << 1);
                
    buffer[5] = (!gamepadData.btnZL << 7) |
                (!gamepadData.btnB << 6) |
                (!gamepadData.btnY << 5) |
                (!gamepadData.btnA << 4) |
                (!gamepadData.btnX << 3) |
                (!gamepadData.btnZR << 2) |
                (!gamepadData.dpadLeft << 1) |
                (!gamepadData.dpadUp << 0);
    
    return 6;  // Tamaño del paquete
}
```

### Conexión Física

```
ESP32          GP2040-CE (Raspberry Pi Pico)
-----          --------------------------------
GPIO 21 (SDA) → Pin configurado como SDA (ej: GPIO 2)
GPIO 22 (SCL) → Pin configurado como SCL (ej: GPIO 3)  
3.3V          → 3.3V
GND           → GND
```

## Configuración en GP2040-CE

1. **Habilitar Wii Extension Add-on** en la configuración web
2. **Configurar pines I2C** (SDA/SCL) según el cableado
3. **Seleccionar bloque I2C** (I2C0 o I2C1)
4. **Configurar velocidad** a 400kHz (por defecto)

El add-on detectará automáticamente el ESP32 como un Classic Controller.

## Capacidades del Sistema

### Botones Disponibles (15 botones digitales)
- **Botones principales**: A, B, X, Y (4 botones)
- **Bumpers/Triggers**: L, R, ZL, ZR (4 botones)
- **Sistema**: Plus (+), Minus (-), Home (3 botones)  
- **D-pad**: Up, Down, Left, Right (4 botones)

### Controles Analógicos (6 ejes)
- **Left Stick**: X (6-bit, 0-63), Y (6-bit, 0-63)
- **Right Stick**: X (5-bit, 0-31), Y (5-bit, 0-31)
- **Triggers**: Left (5-bit, 0-31), Right (5-bit, 0-31)

### Frecuencia de Actualización
- **GP2040-CE polling**: Continuo (`uIntervalMS = 0`) - limitado por main loop (~1000Hz)
- **ESP32 update**: Hasta 10kHz (limitado por velocidad de Serial/I2C)
- **Latencia total optimizada**: ~1.6-2.5ms

## Comandos Serial de Ejemplo

```bash
# Botones
BTN:A:1        # Presionar A
BTN:A:0        # Soltar A
BTN:HOME:1     # Presionar Home

# Sticks analógicos  
STICK:LX:32    # Left stick X al centro
STICK:LY:63    # Left stick Y al máximo
STICK:RX:0     # Right stick X al mínimo

# Triggers
TRIGGER:L:31   # Left trigger al máximo
TRIGGER:R:15   # Right trigger al 50%
```

## Ventajas de esta Aproximación

✅ **Sin modificaciones**: Usa el add-on Wii Extension existente  
✅ **Detección automática**: GP2040-CE reconoce el dispositivo automáticamente  
✅ **15 botones + 6 ejes**: Capacidad completa de un gamepad moderno  
✅ **Flexible**: ESP32 puede recibir datos por Serial, WiFi, Bluetooth, etc.  
✅ **Probado**: Protocolo Wii Extension ya implementado y estable  
✅ **Económico**: ESP32 es barato y ampliamente disponible  

## Análisis de Latencia

### Componentes de Latencia del Sistema ESP32

**1. Latencia I2C (muy baja - ~0.5ms):**
- Transmisión de 6 bytes a 400kHz: ~175 microsegundos
- Delay obligatorio del protocolo Wii: 300 microsegundos  
- **Total I2C: ~0.5ms**

**2. Polling del GP2040-CE (variable):**
- `uIntervalMS = 0` en Wii Extension → polling continuo
- GP2040-CE main loop: ~1000Hz (1ms entre ciclos)
- **Latencia de polling: 0-1ms**

**3. Procesamiento ESP32 (configurable):**
- Parseo de comando serial: ~50-100 microsegundos
- Codificación de datos: ~10-20 microsegundos
- Loop delay actual del código: **10ms** (configurable)
- **Total ESP32: 0.1ms + delay configurado**

### Latencia Total Real:

**Con delay de 10ms (código ejemplo):**
- I2C: 0.5ms + GP2040: 1ms + ESP32: 10ms = **~11.5ms total**

**Con delay optimizado de 1ms:**
- I2C: 0.5ms + GP2040: 1ms + ESP32: 1ms = **~2.5ms total**

**Con delay mínimo (sin delay()):**
- I2C: 0.5ms + GP2040: 1ms + ESP32: 0.1ms = **~1.6ms total**

### Código Optimizado para Mínima Latencia:

```cpp
void loop() {
    // Sin delay fijo - procesamiento continuo
    updateGamepadData();
    
    // Solo micro-delay para estabilidad si es necesario  
    delayMicroseconds(100);  // 0.1ms en lugar de 10ms
}
```

## Desventajas (Revisadas)

❌ **Latencia adicional**: ~1.6-2.5ms (optimizado) vs ~10ms (código ejemplo)  
❌ **Complejidad**: Requiere programar y mantener dos dispositivos  
❌ **Consumo**: ESP32 consume más energía que una solución directa  

**Nota**: La estimación original de 10ms era por el `delay(10)` en el código de ejemplo, no por limitaciones inherentes del sistema.  

## Alternativas de Entrada de Datos

El ESP32 puede recibir comandos de múltiples fuentes:

1. **Serial/UART**: Como se muestra en el ejemplo
2. **WiFi TCP/UDP**: Para control remoto por red
3. **Bluetooth Classic/BLE**: Para controles inalámbricos
4. **SPI/I2C**: Desde otro microcontrolador
5. **GPIO**: Botones físicos adicionales
6. **Sensores**: IMU, encoders, potenciómetros, etc.

---

*Esta implementación proporciona una forma completa y compatible de agregar entrada externa al GP2040-CE usando el protocolo Wii Extension probado.*