# Serial Input Addon

El addon Serial Input permite recibir inputs de gamepad a través del puerto USB Serial (CDC) en lugar de botones físicos.

## Características

- **Comunicación USB CDC**: Usa el mismo puerto USB sin necesidad de hardware adicional
- **Velocidad**: Soporta hasta 120 actualizaciones por segundo (120 Hz)
- **Protocolo simple**: Envía un uint32_t (4 bytes) con los botones presionados

## Protocolo de Comunicación

El addon espera recibir 4 bytes que forman un `uint32_t` (little-endian) donde cada bit representa un botón:

### Mapeo de Bits

```
bit 0  = B1/A           bit 8  = D-Up  
bit 1  = B2/B           bit 9  = D-Down
bit 2  = B3/X           bit 10 = D-Left
bit 3  = B4/Y           bit 11 = D-Right
bit 4  = L1/LB          bit 12 = S1/Back/Select
bit 5  = R1/RB          bit 13 = S2/Start
bit 6  = L2/LT          bit 14 = L3/Left Stick
bit 7  = R2/RT          bit 15 = R3/Right Stick
```

### Ejemplo de Código Python

```python
import serial
import struct
import time

# Conectar al puerto serial del GP2040-CE
ser = serial.Serial('/dev/ttyACM0', 115200)

# Ejemplo: Presionar botón A (bit 0)
button_mask = 0b0000000000000001
data = struct.pack('<I', button_mask)  # Little-endian uint32
ser.write(data)

# Ejemplo: Presionar A + B + Start (bits 0, 1, 13)
button_mask = (1 << 0) | (1 << 1) | (1 << 13)
data = struct.pack('<I', button_mask)
ser.write(data)

# Enviar a 120 Hz
while True:
    # Tu lógica aquí para generar button_mask
    data = struct.pack('<I', button_mask)
    ser.write(data)
    time.sleep(1/120)  # ~8.33ms entre envíos
```

### Ejemplo de Código C/C++ (Arduino)

```cpp
#include <stdint.h>

void setup() {
    Serial.begin(115200);
}

void loop() {
    uint32_t buttonMask = 0;
    
    // Ejemplo: Presionar A + B
    buttonMask |= (1 << 0);  // A
    buttonMask |= (1 << 1);  // B
    
    // Enviar como 4 bytes little-endian
    Serial.write((uint8_t*)&buttonMask, 4);
    
    delay(8);  // ~120 Hz
}
```

## Configuración

1. Compilar GP2040-CE con el addon habilitado
2. En el archivo de configuración de tu board (ej: `BoardConfig.h`):
   ```cpp
   #define SERIAL_INPUT_ENABLED 1
   ```

3. Alternativamente, habilitar via Web Config (cuando se implemente la interfaz web)

## Notas Técnicas

- El addon lee del puerto CDC (USB Serial) estándar
- No interfiere con la configuración web o debugging serial
- Los datos se procesan en `preprocess()` antes del procesamiento normal de botones
- Buffer interno de 64 bytes para manejar ráfagas de datos
- Compatible con cualquier dispositivo que pueda enviar datos serial sobre USB

## Casos de Uso

1. **Control remoto**: Controlar el gamepad desde una PC o Raspberry Pi
2. **Testing automatizado**: Scripts de prueba para verificar funcionalidad
3. **Interfaz custom**: Crear controles personalizados (pedales, sensores, etc.)
4. **Bridge devices**: Convertir otros protocolos a inputs de gamepad
