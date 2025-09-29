# Guía de Instalación - ESP32 Wii Extension Bridge

## 📋 Materiales Necesarios

- **ESP32 DevKit** (cualquier modelo con pines GPIO 21/22)
- **4 cables jumper** macho-macho
- **GP2040-CE** funcionando en Raspberry Pi Pico
- **Cables micro-USB** para programación

## 🔧 Instalación del Software

### 1. Preparar Arduino IDE

1. **Instalar Arduino IDE** (versión 2.0 o superior)
2. **Agregar soporte ESP32**:
   - Ir a `Archivo` → `Preferencias`
   - En "URLs adicionales para el gestor de tarjetas":
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
   - Ir a `Herramientas` → `Placa` → `Gestor de tarjetas`
   - Buscar "esp32" e instalar "ESP32 by Espressif Systems"

### 2. Configurar la Placa

1. **Seleccionar placa**: `Herramientas` → `Placa` → `ESP32 Arduino` → `ESP32 Dev Module`
2. **Configurar parámetros**:
   - **Upload Speed**: 921600
   - **CPU Frequency**: 240MHz (WiFi/BT)
   - **Flash Frequency**: 80MHz
   - **Flash Mode**: DIO
   - **Flash Size**: 4MB (32Mb)
   - **Partition Scheme**: Default 4MB with spiffs
   - **Core Debug Level**: None
   - **PSRAM**: Disabled

### 3. Programar el ESP32

1. **Abrir el archivo** `ESP32_WiiExtension_Bridge.ino`
2. **Conectar ESP32** via USB al PC
3. **Seleccionar puerto** correcto en `Herramientas` → `Puerto`
4. **Subir código** (botón ↑ o Ctrl+U)

## 🔌 Conexiones Físicas

### Tabla de Conexiones

| ESP32 Pin | Función | GP2040-CE Pin | Notas |
|-----------|---------|---------------|--------|
| GPIO 21   | SDA     | SDA configurado | Ver pines disponibles abajo |
| GPIO 22   | SCL     | SCL configurado | Ver pines disponibles abajo |
| 3.3V      | VCC     | 3.3V          | ⚠️ **NO usar 5V** |
| GND       | Ground  | GND           | Cualquier pin GND |

### Pines I2C Disponibles en GP2040-CE

**Bloques I2C disponibles en Raspberry Pi Pico:**

**I2C0 (Bloque 0):**
- SDA: GPIO 0, 4, 8, 12, 16, 20
- SCL: GPIO 1, 5, 9, 13, 17, 21

**I2C1 (Bloque 1):**
- SDA: GPIO 2, 6, 10, 14, 18, 22, 26
- SCL: GPIO 3, 7, 11, 15, 19, 23, 27

### Conexión Recomendada

```
ESP32          Cable      GP2040-CE (Pi Pico)
-----          -----      ------------------
GPIO 21 (SDA)  <------>   GPIO 2 (SDA)
GPIO 22 (SCL)  <------>   GPIO 3 (SCL)
3.3V           <------>   3.3V (Pin 36)
GND            <------>   GND (Pin 38)
```

## ⚙️ Configuración GP2040-CE

### 1. Acceder a la Configuración Web

1. **Conectar GP2040-CE** al PC via USB
2. **Mantener presionado** el botón de configuración web al conectar
3. **Abrir navegador** en `192.168.7.1`

### 2. Habilitar Wii Extension

1. Ir a **"Add-Ons"**
2. Buscar **"Wii Extension"**
3. **Habilitar** el add-on
4. **Guardar configuración**

### 3. Configurar I2C

1. Ir a **"Peripheral Mapping"**
2. **Seleccionar bloque I2C** (I2C0 o I2C1)
3. **Configurar pines**:
   - **SDA Pin**: Según tu conexión (ej: GPIO 2)
   - **SCL Pin**: Según tu conexión (ej: GPIO 3)
4. **Velocidad**: 400000 Hz (400 kHz)
5. **Guardar configuración**
6. **Reiniciar** GP2040-CE

## 🧪 Pruebas y Verificación

### 1. Monitor Serial

1. **Abrir Monitor Serial** en Arduino IDE (Ctrl+Shift+M)
2. **Configurar velocidad**: 115200 baud
3. **Verificar mensajes**:
   ```
   ✓ I2C inicializado correctamente
   Dispositivo listo - Esperando conexión GP2040-CE...
   ```

### 2. Probar Conexión I2C

Cuando GP2040-CE detecte el dispositivo, verás:
```
I2C: Encriptación desactivada
I2C: Formato configurado  
I2C: Tipo DATA_TYPE_1 establecido
```

### 3. Probar Comandos

Envía estos comandos en el Monitor Serial:

```
TEST           # Secuencia automática de prueba
BTN:A:1        # Presionar botón A
BTN:A:0        # Soltar botón A
STICK:LX:0     # Stick izquierdo a la izquierda
STATUS         # Ver estado detallado
```

### 4. Verificar en PC

1. **Conectar GP2040-CE** al PC como gamepad
2. **Abrir configurador de gamepad** de Windows/Linux/Mac
3. **Enviar comandos** desde el Monitor Serial
4. **Verificar que los botones/sticks respondan**

## 🔍 Solución de Problemas

### Problema: ESP32 no se conecta

**Síntomas**: Monitor Serial muestra error I2C
**Soluciones**:
- Verificar conexiones físicas
- Comprobar que GP2040-CE tenga configuración I2C habilitada
- Revisar que los pines SDA/SCL sean correctos

### Problema: GP2040-CE no detecta dispositivo

**Síntomas**: No aparecen mensajes "I2C: ..." en Monitor Serial
**Soluciones**:
- Verificar que Wii Extension esté habilitado en GP2040-CE
- Comprobar configuración de pines I2C en GP2040-CE
- Reiniciar ambos dispositivos
- Verificar conexión 3.3V y GND

### Problema: Comandos no funcionan

**Síntomas**: Comandos se envían pero gamepad no responde
**Soluciones**:
- Verificar sintaxis de comandos (mayúsculas/minúsculas)
- Comprobar que GP2040-CE esté conectado al PC como gamepad
- Usar comando `STATUS` para verificar estado
- Probar secuencia `TEST`

### Problema: Latencia alta

**Síntomas**: Respuesta lenta del gamepad
**Soluciones**:
- Eliminar o reducir delays en el código
- Verificar velocidad I2C (debe ser 400kHz)
- Optimizar frecuencia de envío de comandos

## 📊 Comandos Disponibles

### Botones (15 disponibles)
```
BTN:A:1/0        BTN:B:1/0        BTN:X:1/0        BTN:Y:1/0
BTN:L:1/0        BTN:R:1/0        BTN:ZL:1/0       BTN:ZR:1/0  
BTN:PLUS:1/0     BTN:MINUS:1/0    BTN:HOME:1/0
BTN:UP:1/0       BTN:DOWN:1/0     BTN:LEFT:1/0     BTN:RIGHT:1/0
```

### Sticks Analógicos
```
STICK:LX:0-63    # Left stick X (0=izquierda, 63=derecha, 32=centro)
STICK:LY:0-63    # Left stick Y (0=abajo, 63=arriba, 32=centro)  
STICK:RX:0-31    # Right stick X (0=izquierda, 31=derecha, 16=centro)
STICK:RY:0-31    # Right stick Y (0=abajo, 31=arriba, 16=centro)
```

### Triggers Analógicos
```
TRIGGER:L:0-31   # Left trigger (0=suelto, 31=presionado al máximo)
TRIGGER:R:0-31   # Right trigger (0=suelto, 31=presionado al máximo)
```

### Utilidades
```
STATUS           # Estado detallado del gamepad
RESET            # Resetear a estado neutro
TEST             # Secuencia automática de prueba
HELP             # Lista de comandos
```

## 🚀 Uso Avanzado

### Control por WiFi

Para recibir comandos por WiFi en lugar de Serial:

```cpp
#include <WiFi.h>
#include <WiFiServer.h>

const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";
WiFiServer server(80);

// En setup():
WiFi.begin(ssid, password);
server.begin();

// En loop():
WiFiClient client = server.available();
if (client) {
    String command = client.readStringUntil('\n');
    parseCommand(command);
}
```

### Control por Bluetooth

```cpp
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// En setup():
SerialBT.begin("ESP32_Gamepad");

// En loop():
if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    parseCommand(command);
}
```

---

¡Con esta configuración tendrás un gamepad completamente controlable remotamente! 🎮