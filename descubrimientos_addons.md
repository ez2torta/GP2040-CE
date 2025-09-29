# GP2040-CE Add-ons: Análisis de Funcionalidades de Entrada Externa

## Resumen de Investigación

Este documento analiza las capacidades de GP2040-CE para recibir entrada externa y controlar el estado del gamepad a través de diferentes mecanismos de comunicación.

## Add-ons Existentes para Entrada Externa

### 1. USB Host Add-ons

#### Gamepad USB Host
- **Archivos**: `gamepad_usb_host.cpp`, `gamepad_usb_host_listener.cpp`
- **Funcionalidad**: Permite conectar un gamepad USB externo para controlar el estado del GP2040-CE
- **Compatibilidad**: Soporta múltiples tipos de controles:
  - Sony DualShock 4 (PS4)
  - Sony DualSense (PS5)  
  - Google Stadia Controller
  - Ultrastik 360
  - Logitech Driving Force wheels
  - Y otros controles genéricos
- **Implementación**: Usa TinyUSB HID host para recibir reportes de entrada y mapearlos al estado interno del gamepad

#### Keyboard Host
- **Archivos**: `keyboard_host.cpp`, `keyboard_host_listener.cpp`
- **Funcionalidad**: Permite conectar un teclado USB para controlar el gamepad
- **Mapeo**: Convierte teclas del teclado a botones y direcciones del gamepad

### 2. I2C Add-ons

#### ✨ Wii Extension Controller (WiiExt) - HALLAZGO IMPORTANTE
- **Archivos**: `wiiext.cpp`, `wiiext.h`, `lib/WiiExtension/`
- **Funcionalidad**: **Lee controles de Nintendo Wii mediante comunicación I2C**
- **Protocolo**: I2C con detección automática de dispositivos en diferentes bloques I2C
- **Controles Soportados**:
  - **Nunchuck**: Joystick analógico + botones C y Z
  - **Classic Controller**: Control completo con dos joysticks analógicos, D-pad, triggers
  - **Guitar Hero Guitar**: Botones de colores + whammy bar
  - **Rock Band Drums**: Pads de batería + pedal
  - **DJ Hero Turntable**: Controles de DJ con ruedas
  - **Taiko Drum Controller**: Don/Kat para juegos de tambor
  - **uDraw/DrawSome Tablets**: Tabletas de dibujo

##### Detalles Técnicos del Add-on Wii:

**Direcciones I2C**:
- Wii Extension Controller: `0x52`
- Wii Motion Plus: `0x53`

**Configuración I2C**:
```cpp
#define WII_EXTENSION_I2C_BLOCK i2c0
#define WII_EXTENSION_I2C_SPEED 400000  // 400kHz
#define WII_EXTENSION_I2C_ADDR 0x52
#define WII_MOTIONPLUS_I2C_ADDR 0x53
```

**Proceso de Detección**:
```cpp
// El dispositivo retorna múltiples direcciones posibles
std::vector<uint8_t> getDeviceAddresses() const override {
    return {WII_EXTENSION_I2C_ADDR, WII_MOTIONPLUS_I2C_ADDR};
}

// Escaneo automático en diferentes bloques I2C
PeripheralI2CScanResult result = PeripheralManager::getInstance().scanForI2CDevice(wii->getDeviceAddresses());

// Configuración automática del bloque I2C encontrado
if (result.address > -1) {
    wii->setAddress(result.address);
    wii->setI2C(PeripheralManager::getInstance().getI2C(result.block));
}
```

**Inicialización sin Encriptación**:
```cpp
// Desactiva encriptación para simplificar comunicación
regWrite[0] = 0xF0; regWrite[1] = 0x55;  // Disable encryption
regWrite[0] = 0xFB; regWrite[1] = 0x00;  // Set format
regWrite[0] = 0xFE; regWrite[1] = 0x03;  // Set data type
```

##### Mapeo de Botones (Classic Controller ejemplo):
- A/B/X/Y → Botones principales del gamepad
- L/ZL/R/ZR → Triggers y bumpers
- D-pad → Direcciones
- Joysticks analógicos → Sticks del gamepad
- Detección de acelerómetro y giroscopio

#### I2C Analog (ADS1219)
- **Archivos**: `i2canalog1219.cpp`
- **Funcionalidad**: Lee valores analógicos a través de I2C para joysticks

#### I2C GPIO Expander
- **Archivos**: `i2c_gpio_pcf8575.cpp`
- **Funcionalidad**: Expande pines GPIO a través de I2C

### 3. SPI Add-ons

#### SPI Analog (ADS1256)
- **Archivos**: `spi_analog_ads1256.cpp`
- **Funcionalidad**: Lee valores analógicos de alta precisión vía SPI

### 4. Otros Add-ons de Entrada

#### Input Macro
- **Archivos**: `input_macro.cpp`
- **Funcionalidad**: Ejecuta secuencias de botones predefinidas
- **Activación**: Por pines GPIO físicos, no por comunicación externa

#### SNES Input
- **Archivos**: `snes_input.cpp`
- **Funcionalidad**: Lee controles SNES nativos

#### TurboGrafx-16 Input
- **Archivos**: `tg16_input.cpp`
- **Funcionalidad**: Lee controles TG16 nativos

## Comunicación Web/HTTP

### Web Configuration
- **Archivo**: `webconfig.cpp`
- **Funcionalidad**: Interfaz web HTTP para configuración
- **Limitación**: Solo para configuración, no para control en tiempo real del gamepad
- **Endpoints**: Maneja rutas como `/cgi/action` para configuración

## Detalle del Sistema I2C y Wii Extension

### Capacidades del Add-on Wii Extension

El add-on de Wii Extension es particularmente interesante porque:

1. **Detección Automática**: Escanea automáticamente los bloques I2C disponibles para encontrar dispositivos Wii
2. **Múltiples Controladores**: Soporta una amplia gama de accesorios de Wii
3. **Configuración Dinámica**: Se adapta automáticamente al tipo de control detectado
4. **Entrada Analógica Completa**: Maneja joysticks, triggers, acelerómetros
5. **Procesamiento en Tiempo Real**: Actualiza el estado del gamepad continuamente

### Biblioteca WiiExtension

La biblioteca `lib/WiiExtension/` proporciona:
- Comunicación I2C de bajo nivel con dispositivos Wii
- Decodificación de datos de diferentes tipos de controles
- Calibración automática de rangos analógicos
- Detección automática del tipo de dispositivo conectado

### Uso Práctico

Este add-on permite conectar **cualquier accesorio oficial de Nintendo Wii** al GP2040-CE mediante:

**Conexión Física**:
- **Pin SDA**: Configurable (por defecto GPIO 2 o 6)
- **Pin SCL**: Configurable (por defecto GPIO 3 o 7) 
- **VCC**: 3.3V (importante: NO usar 5V)
- **GND**: Tierra común

**Configuración**:
- **Automática**: Detección automática del tipo de dispositivo
- **Sin código**: No requiere programación adicional
- **Múltiples dispositivos**: Puede usar ambos bloques I2C simultáneamente
- **Velocidad**: 400kHz por defecto (configurable)

**Controles Compatibles Verificados**:
- ✅ Nunchuck oficial Nintendo
- ✅ Classic Controller/Classic Controller Pro
- ✅ Guitar Hero/Rock Band guitars oficiales
- ✅ Rock Band drums
- ✅ DJ Hero turntables
- ✅ Taiko no Tatsujin controllers
- ✅ uDraw/DrawSome tablets

## Limitaciones Identificadas

### No hay soporte nativo para:
1. **Comunicación UART/Serial**: No se encontraron bibliotecas UART del Pico SDK incluidas
2. **Control remoto en tiempo real vía web**: La interfaz web solo sirve para configuración
3. **TCP/UDP sockets**: No hay networking más allá del servidor HTTP básico

## Arquitectura de Add-ons

### Clase Base GPAddon
```cpp
class GPAddon {
public:
    virtual bool available() = 0;      // Determina si el add-on está disponible
    virtual void setup() = 0;          // Inicialización
    virtual void process() = 0;        // Procesamiento principal
    virtual void preprocess() = 0;     // Pre-procesamiento
    virtual void postprocess(bool) = 0; // Post-procesamiento
    virtual std::string name() = 0;    // Nombre del add-on
    virtual void reinit() = 0;         // Reinicialización
};
```

### USB Listener (para add-ons que requieren USB Host)
Los add-ons que manejan dispositivos USB heredan también de `USBListener` que proporciona callbacks para:
- `mount()`: Cuando se conecta un dispositivo
- `unmount()`: Cuando se desconecta un dispositivo  
- `report_received()`: Cuando se recibe un reporte HID

### I2C Peripheral Manager

El sistema de gestión de periféricos I2C proporciona:

**Configuración de Bloques I2C**:
```cpp
// Soporte para 2 bloques I2C independientes
PeripheralI2C blockI2C0;  // i2c0
PeripheralI2C blockI2C1;  // i2c1

// Configuración individual por bloque
void initI2C() {
    if (peripheralOptions.blockI2C0.enabled) 
        blockI2C0.setConfig(0, sda_pin, scl_pin, speed);
    if (peripheralOptions.blockI2C1.enabled) 
        blockI2C1.setConfig(1, sda_pin, scl_pin, speed);
}
```

**Funciones de Escaneo**:
- `scanForI2CDevice(addressList)`: Escanea múltiples direcciones en todos los bloques I2C
- `getI2C(block)`: Obtiene la instancia I2C del bloque específico (0 o 1)
- `isI2CEnabled(block)`: Verifica si un bloque I2C está habilitado
- Detección automática de dispositivos en ambos bloques I2C

**Resultado del Escaneo**:
```cpp
typedef struct {
    int8_t address;  // Dirección encontrada (-1 si no se encuentra)
    uint8_t block;   // Bloque I2C donde se encontró (0 o 1)
} PeripheralI2CScanResult;
```

## Conclusiones

### Funcionalidades Existentes
- ✅ **USB Host**: Gamepad y teclado externos
- ✅ **I2C**: Múltiples dispositivos (Wii controllers, ADC, GPIO expander)
- ✅ **SPI**: ADC de alta precisión
- ✅ **Web HTTP**: Configuración (no control en tiempo real)
- ✅ **Controles Retro**: SNES, TurboGrafx-16
- ✅ **Wii Controllers**: Amplio soporte vía I2C ⭐

### Funcionalidades Faltantes
- ❌ **UART/Serial**: No implementado
- ❌ **Control remoto en tiempo real**: No disponible vía web
- ❌ **TCP/UDP**: No implementado

### Descubrimiento Clave: Wii Extension

El add-on de **Wii Extension es el más versátil para entrada externa** disponible actualmente:
- Soporte para múltiples tipos de controles
- Comunicación I2C estable y probada
- Configuración automática
- Entrada analógica completa
- Fácil conexión física (solo 4 cables)

### Recomendaciones

#### Para Control Serial
Para implementar control por puerto serial, se requiere crear un add-on personalizado que:
1. Incluya las bibliotecas UART del Pico SDK
2. Herede de `GPAddon`
3. Implemente lectura serial en `process()`
4. Modifique el estado del gamepad según comandos recibidos
5. Se agregue al sistema de construcción CMake

#### Para Control I2C Personalizado
Basándose en el add-on Wii Extension, se podría crear un dispositivo I2C personalizado que:
1. Use el mismo sistema de detección automática
2. Implemente un protocolo de comunicación personalizado  
3. Aproveche la infraestructura I2C existente
4. Utilice direcciones I2C no conflictivas (evitar 0x52, 0x53)

#### Alternativa: Extender el Add-on Wii
Una opción más simple sería modificar el add-on Wii Extension para:
1. Reconocer dispositivos I2C personalizados con direcciones específicas
2. Implementar decodificación personalizada para el protocolo deseado
3. Mapear los datos recibidos a botones/sticks del gamepad
4. Mantener compatibilidad con dispositivos Wii existentes

**Ventajas de esta aproximación**:
- ✅ Infraestructura I2C ya probada y estable
- ✅ Sistema de detección automática funcional
- ✅ Manejo de errores implementado
- ✅ Configuración flexible de pines
- ✅ Soporte para múltiples dispositivos (ambos bloques I2C)

## Resumen Ejecutivo

### Para implementar control externo del gamepad, las mejores opciones son:

1. **🎯 Recomendado: Modificar Add-on Wii Extension**
   - Agregar soporte para dispositivo I2C personalizado
   - Aprovechar infraestructura I2C robusta y probada
   - Conexión simple (4 cables)
   - Detección automática
   - Configuración flexible

2. **💻 USB Host Add-on personalizado**
   - Para dispositivos USB existentes
   - Usa infraestructura TinyUSB
   - Requiere implementar driver HID específico

3. **⚡ UART Add-on desde cero**
   - Requiere más desarrollo
   - Necesita incluir bibliotecas UART del Pico SDK
   - Más complejo pero más flexible para protocolos personalizados

### El add-on Wii Extension es la funcionalidad más versátil disponible actualmente para entrada externa, con soporte nativo para múltiples tipos de controles a través de I2C.

---

*Documento creado: Diciembre 2024*  
*Investigación de: GP2040-CE Add-ons para entrada externa*  
*Estado: Análisis completo - Wii Extension identificado como mejor opción existente*