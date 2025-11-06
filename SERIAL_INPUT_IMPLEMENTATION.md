# Resumen de Implementación: Serial Input Addon para GP2040-CE

## Archivos Creados/Modificados

### Nuevos Archivos

1. **`headers/addons/serial_input.h`**
   - Header del addon con definiciones de clase y constantes
   - Define el protocolo de 16 bits para botones
   - Incluye buffer de 64 bytes para recepción

2. **`src/addons/serial_input.cpp`**
   - Implementación del addon
   - Usa TinyUSB CDC (puerto serial USB)
   - Lee paquetes de 4 bytes (uint32_t) con bitmask de botones
   - Procesa inputs en `preprocess()` antes del procesamiento normal

3. **`docs/serial-input-addon.md`**
   - Documentación completa del addon
   - Protocolo de comunicación
   - Ejemplos de código en Python y C++

4. **`scripts/test_serial_input.py`**
   - Script de prueba en Python
   - 3 modos: test automático, interactivo, spam continuo
   - Útil para debugging y testing

### Archivos Modificados

1. **`proto/config.proto`**
   - Agregado `message SerialInputOptions` (línea ~887)
   - Agregado `optional SerialInputOptions serialInputOptions = 31;` a `AddonOptions`

2. **`src/config_utils.cpp`**
   - Agregada inicialización de `serialInputOptions` (línea ~1049)
   - `INIT_UNSET_PROPERTY(config.addonOptions.serialInputOptions, enabled, !!SERIAL_INPUT_ENABLED);`

3. **`src/gp2040.cpp`**
   - Agregado `#include "addons/serial_input.h"`
   - Agregado `addons.LoadAddon(new SerialInputAddon());` en setup

4. **`CMakeLists.txt`**
   - Agregado `src/addons/serial_input.cpp` a la lista de fuentes

## Protocolo de Comunicación

### Formato
- **4 bytes** (uint32_t, little-endian)
- Cada bit representa un botón (1 = presionado, 0 = soltado)

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
bits 16-31 = Reservados (no usados actualmente)
```

## Características Técnicas

1. **Comunicación USB CDC**
   - Usa el puerto USB principal (no requiere hardware adicional)
   - No interfiere con Web Config ni debugging
   - Velocidad: 115200 baud (configurable)

2. **Procesamiento**
   - Se ejecuta en `preprocess()` (antes del procesamiento normal de botones)
   - Buffer circular de 64 bytes
   - Protección contra overflow

3. **Tasa de Actualización**
   - Soporta hasta 120 Hz (8.33ms entre paquetes)
   - Sin limitación de software, depende del host

## Cómo Habilitar

### Opción 1: BoardConfig.h
```cpp
#define SERIAL_INPUT_ENABLED 1
```

### Opción 2: Web Config (Futuro)
Cuando se implemente la interfaz web, se podrá habilitar desde ahí.

## Compilación

1. Los archivos ya están integrados en CMakeLists.txt
2. Compilar normalmente: `cd build && cmake .. && make`
3. El addon se cargará automáticamente si está habilitado

## Testing

### Usando el script Python
```bash
# Test automático de todos los botones
python3 scripts/test_serial_input.py /dev/ttyACM0 test

# Modo interactivo
python3 scripts/test_serial_input.py /dev/ttyACM0 interactive

# Spam continuo a 120 Hz
python3 scripts/test_serial_input.py /dev/ttyACM0 spam
```

### Ejemplo rápido en Python
```python
import serial
import struct

ser = serial.Serial('/dev/ttyACM0', 115200)

# Presionar botón A
button_mask = (1 << 0)
ser.write(struct.pack('<I', button_mask))

# Soltar todos los botones
ser.write(struct.pack('<I', 0))
```

## Casos de Uso

1. **Control Remoto**: Controlar el gamepad desde PC/Raspberry Pi
2. **Testing Automatizado**: Scripts de QA
3. **Interfaces Custom**: Pedales, sensores, etc.
4. **Bridge Devices**: Convertir otros protocolos a gamepad
5. **Desarrollo**: Debugging sin hardware físico

## Limitaciones Actuales

1. Solo soporta botones digitales (no analógicos como joysticks)
2. No hay confirmación de recepción (one-way communication)
3. Interfaz web para configuración aún no implementada

## Mejoras Futuras Posibles

1. Soporte para joysticks analógicos (usando bytes adicionales)
2. Protocolo bidireccional (confirmaciones, estado)
3. Configuración de baud rate via web
4. Soporte para múltiples interfaces CDC
5. Compresión/optimización de paquetes
6. Interfaz web para habilitar/deshabilitar

## Notas de Implementación

- El addon usa TinyUSB CDC que ya está configurado en GP2040-CE
- No hay conflicto con otros addons
- La lectura es no bloqueante
- Compatible con RP2040 y RP2350
- Testado con Linux, debería funcionar en Windows/macOS también
