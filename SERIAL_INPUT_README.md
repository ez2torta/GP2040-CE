# Serial Input Addon - Guía de Inicio Rápido

## ✅ Estado de Implementación

El addon **Serial Input** está completamente implementado y listo para usar. Todos los archivos han sido creados y modificados correctamente.

## 📋 Checklist de Implementación

- [x] Header file (`headers/addons/serial_input.h`)
- [x] Implementation file (`src/addons/serial_input.cpp`)
- [x] Protobuf config (`proto/config.proto`)
- [x] Config initialization (`src/config_utils.cpp`)
- [x] Addon registration (`src/gp2040.cpp`)
- [x] CMakeLists.txt updated
- [x] Documentation (`docs/serial-input-addon.md`)
- [x] Python test script (`scripts/test_serial_input.py`)

## 🚀 Compilación

### 1. Habilitar el addon en tu configuración de board

Edita el archivo de tu board, por ejemplo `/configs/Pico/BoardConfig.h`:

```cpp
#define SERIAL_INPUT_ENABLED 1
```

### 2. Compilar el proyecto

```bash
cd build
cmake ..
make
```

### 3. Flashear el .uf2

El archivo generado estará en `build/GP2040-CE_X.X.X_YourBoard.uf2`

## 🧪 Pruebas

### Prueba Rápida con Python

```bash
# Asegúrate de tener pyserial instalado
pip3 install pyserial

# Encuentra el puerto (Linux/Mac)
ls /dev/ttyACM*

# Ejecutar test automático
python3 scripts/test_serial_input.py /dev/ttyACM0 test

# Modo interactivo
python3 scripts/test_serial_input.py /dev/ttyACM0 interactive
```

### Ejemplo Mínimo en Python

```python
import serial
import struct
import time

ser = serial.Serial('/dev/ttyACM0', 115200)

# Presionar botón A
data = struct.pack('<I', 1 << 0)  # bit 0 = A
ser.write(data)
time.sleep(0.5)

# Soltar
data = struct.pack('<I', 0)
ser.write(data)

ser.close()
```

## 📖 Protocolo

Envía 4 bytes (uint32_t little-endian) donde cada bit representa un botón:

```
Bit  | Botón          | Bit  | Botón
-----|----------------|------|---------------
0    | A/B1          | 8    | D-Up
1    | B/B2          | 9    | D-Down
2    | X/B3          | 10   | D-Left
3    | Y/B4          | 11   | D-Right
4    | LB/L1         | 12   | Select/S1
5    | RB/R1         | 13   | Start/S2
6    | LT/L2         | 14   | L3
7    | RT/R2         | 15   | R3
```

## 🔍 Debugging

### Verificar que el addon está cargado

En los logs de arranque deberías ver algo como:
```
[SerialInput] Setup complete
```

### Verificar puerto serial

**Linux:**
```bash
ls -la /dev/ttyACM*
```

**macOS:**
```bash
ls -la /dev/cu.usbmodem*
```

**Windows:**
- Abre el Administrador de Dispositivos
- Busca "Puertos (COM y LPT)"
- El dispositivo aparecerá como "USB Serial Device (COMX)"

## 📚 Documentación Completa

Ver:
- `docs/serial-input-addon.md` - Documentación detallada
- `SERIAL_INPUT_IMPLEMENTATION.md` - Detalles de implementación

## 🎯 Casos de Uso

1. **Testing Automatizado**: Ejecutar suites de prueba
2. **Control Remoto**: Controlar desde PC/Raspberry Pi
3. **Input Custom**: Pedales, sensores, etc.
4. **Bridge**: Convertir otros protocolos a gamepad

## ⚠️ Notas Importantes

1. El addon usa el puerto USB CDC principal
2. No interfiere con Web Config ni debugging serial
3. Funciona a 115200 baud
4. Soporta hasta 120 Hz de actualización
5. Compatible con RP2040 y RP2350

## 🐛 Troubleshooting

### "No such file or directory /dev/ttyACM0"
- Verifica que el dispositivo esté conectado
- En Linux, asegúrate de tener permisos: `sudo usermod -a -G dialout $USER`
- Cierra sesión y vuelve a entrar

### "Permission denied"
```bash
sudo chmod 666 /dev/ttyACM0
```

### El addon no responde
- Verifica que `SERIAL_INPUT_ENABLED` esté en 1
- Recompila completamente: `make clean && make`
- Verifica que el .uf2 se haya flasheado correctamente

## 📝 Contribuciones Futuras

Ideas para mejorar:
- [ ] Soporte para joysticks analógicos
- [ ] Protocolo bidireccional
- [ ] Interfaz web para configuración
- [ ] Múltiples canales CDC
- [ ] Compresión de datos

## 📄 Licencia

Mismo que GP2040-CE (MIT License)
