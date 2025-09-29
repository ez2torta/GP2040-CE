/*
 * ESP32 Wii Extension Bridge para GP2040-CE
 * 
 * Este código convierte un ESP32 en un Wii Classic Controller virtual
 * que puede ser controlado remotamente via Serial, WiFi, etc.
 * 
 * Autor: Basado en análisis del protocolo GP2040-CE Wii Extension
 * Versión: 1.0
 * 
 * CONEXIONES FÍSICAS:
 * ESP32          GP2040-CE (Raspberry Pi Pico)
 * -----          --------------------------------
 * GPIO 21 (SDA) → Pin SDA configurado (ej: GPIO 2, 6, 10, 14, 18, 22, 26)
 * GPIO 22 (SCL) → Pin SCL configurado (ej: GPIO 3, 7, 11, 15, 19, 23, 27)
 * 3.3V          → 3.3V (IMPORTANTE: NO usar 5V)
 * GND           → GND
 * 
 * CONFIGURACIÓN GP2040-CE:
 * 1. Habilitar "Wii Extension" en Add-ons
 * 2. Configurar pines I2C (SDA/SCL)
 * 3. Seleccionar bloque I2C (I2C0 o I2C1)  
 * 4. Velocidad: 400kHz (por defecto)
 * 
 * COMANDOS SERIAL (115200 baud):
 * BTN:A:1        - Presionar botón A
 * BTN:A:0        - Soltar botón A  
 * STICK:LX:32    - Left stick X al centro (0-63)
 * TRIGGER:L:31   - Left trigger al máximo (0-31)
 * 
 * BOTONES DISPONIBLES:
 * A, B, X, Y, L, R, ZL, ZR, PLUS, MINUS, HOME, UP, DOWN, LEFT, RIGHT
 */

#include <Wire.h>

//==============================================================================
// CONFIGURACIÓN DE PINES Y CONSTANTES
//==============================================================================

// Pines I2C del ESP32
#define I2C_SDA_PIN       21        // GPIO 21 (SDA)
#define I2C_SCL_PIN       22        // GPIO 22 (SCL)
#define I2C_FREQUENCY     400000    // 400kHz (estándar Wii Extension)

// Protocolo Wii Extension
#define WII_I2C_ADDRESS   0x52      // Dirección I2C fija del Classic Controller
#define LED_BUILTIN_PIN   2         // LED incorporado del ESP32

// IDs del dispositivo (Classic Controller)
const uint8_t DEVICE_ID[6] = {0x00, 0x00, 0xA4, 0x20, 0x01, 0x01};

// Datos de calibración por defecto
const uint8_t CALIBRATION_DATA[16] = {
    0xFF, 0x00, 0x80,  // Left stick: max, min, center
    0xFF, 0x00, 0x80,  // Left stick: max, min, center  
    0x1F, 0x00, 0x0F,  // Right stick: max, min, center
    0x1F, 0x00, 0x0F,  // Right stick: max, min, center
    0x1F, 0x1F,        // Triggers: left_max, right_max
    0x00, 0x00         // Padding
};

//==============================================================================
// ESTRUCTURA DE DATOS DEL GAMEPAD
//==============================================================================

struct GamepadState {
    // Sticks analógicos
    uint8_t leftStickX;      // 0-63 (6 bits)
    uint8_t leftStickY;      // 0-63 (6 bits)
    uint8_t rightStickX;     // 0-31 (5 bits)
    uint8_t rightStickY;     // 0-31 (5 bits)
    
    // Triggers analógicos
    uint8_t leftTrigger;     // 0-31 (5 bits)
    uint8_t rightTrigger;    // 0-31 (5 bits)
    
    // Botones digitales (true = presionado)
    bool btnA, btnB, btnX, btnY;
    bool btnL, btnR, btnZL, btnZR;
    bool btnPlus, btnMinus, btnHome;
    bool dpadUp, dpadDown, dpadLeft, dpadRight;
};

//==============================================================================
// VARIABLES GLOBALES
//==============================================================================

GamepadState gamepad;
uint8_t currentRegister = 0x00;
bool deviceInitialized = false;
bool ledState = false;
unsigned long lastLedToggle = 0;
unsigned long lastStatusPrint = 0;

//==============================================================================
// CONFIGURACIÓN INICIAL
//==============================================================================

void setup() {
    // Inicializar comunicación serial
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n" + String("=").substring(0, 60));
    Serial.println("ESP32 Wii Extension Bridge v1.0");
    Serial.println("Para GP2040-CE - Iniciando...");
    Serial.println(String("=").substring(0, 60));
    
    // Configurar LED indicador
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN_PIN, LOW);
    
    // Inicializar gamepad con valores centrados/neutros
    initializeGamepad();
    
    // Configurar I2C como slave
    bool i2cOk = Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, WII_I2C_ADDRESS);
    
    if (i2cOk) {
        Wire.setClock(I2C_FREQUENCY);
        Wire.onReceive(onI2CReceive);
        Wire.onRequest(onI2CRequest);
        
        Serial.println("✓ I2C inicializado correctamente");
        Serial.println("  - Dirección: 0x" + String(WII_I2C_ADDRESS, HEX));
        Serial.println("  - SDA: GPIO" + String(I2C_SDA_PIN));
        Serial.println("  - SCL: GPIO" + String(I2C_SCL_PIN));
        Serial.println("  - Velocidad: " + String(I2C_FREQUENCY/1000) + " kHz");
    } else {
        Serial.println("✗ ERROR: No se pudo inicializar I2C");
        while(1) {
            digitalWrite(LED_BUILTIN_PIN, !digitalRead(LED_BUILTIN_PIN));
            delay(100);
        }
    }
    
    Serial.println("\nDispositivo listo - Esperando conexión GP2040-CE...");
    Serial.println("Envía comandos por Serial (ejemplo: BTN:A:1)");
    printHelp();
}

//==============================================================================
// BUCLE PRINCIPAL
//==============================================================================

void loop() {
    // Procesar comandos seriales
    processSerialCommands();
    
    // Actualizar LED indicador
    updateStatusLED();
    
    // Imprimir estado periódicamente
    printStatus();
    
    // Micro-delay para estabilidad (latencia mínima)
    delayMicroseconds(100);
}

//==============================================================================
// INICIALIZACIÓN DEL GAMEPAD
//==============================================================================

void initializeGamepad() {
    // Sticks al centro
    gamepad.leftStickX = 32;     // Centro de rango 0-63
    gamepad.leftStickY = 32;     // Centro de rango 0-63
    gamepad.rightStickX = 16;    // Centro de rango 0-31
    gamepad.rightStickY = 16;    // Centro de rango 0-31
    
    // Triggers sueltos
    gamepad.leftTrigger = 0;     // Rango 0-31
    gamepad.rightTrigger = 0;    // Rango 0-31
    
    // Todos los botones liberados
    gamepad.btnA = false;
    gamepad.btnB = false;
    gamepad.btnX = false;
    gamepad.btnY = false;
    gamepad.btnL = false;
    gamepad.btnR = false;
    gamepad.btnZL = false;
    gamepad.btnZR = false;
    gamepad.btnPlus = false;
    gamepad.btnMinus = false;
    gamepad.btnHome = false;
    gamepad.dpadUp = false;
    gamepad.dpadDown = false;
    gamepad.dpadLeft = false;
    gamepad.dpadRight = false;
    
    Serial.println("✓ Gamepad inicializado (estado neutro)");
}

//==============================================================================
// PROCESAMIENTO DE COMANDOS SERIALES
//==============================================================================

void processSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            parseCommand(command);
        }
    }
}

void parseCommand(String cmd) {
    cmd.toUpperCase();
    
    if (cmd.startsWith("BTN:")) {
        handleButtonCommand(cmd);
    }
    else if (cmd.startsWith("STICK:")) {
        handleStickCommand(cmd);
    }
    else if (cmd.startsWith("TRIGGER:")) {
        handleTriggerCommand(cmd);
    }
    else if (cmd == "STATUS") {
        printDetailedStatus();
    }
    else if (cmd == "HELP") {
        printHelp();
    }
    else if (cmd == "RESET") {
        initializeGamepad();
        Serial.println("✓ Gamepad reseteado a estado neutro");
    }
    else if (cmd == "TEST") {
        runTestSequence();
    }
    else {
        Serial.println("✗ Comando desconocido: " + cmd);
        Serial.println("  Envía 'HELP' para ver comandos disponibles");
    }
}

void handleButtonCommand(String cmd) {
    // Formato: BTN:BUTTON:VALUE
    int firstColon = cmd.indexOf(':', 4);
    int secondColon = cmd.indexOf(':', firstColon + 1);
    
    if (firstColon == -1 || secondColon == -1) {
        Serial.println("✗ Formato incorrecto. Usa: BTN:A:1 o BTN:A:0");
        return;
    }
    
    String button = cmd.substring(4, firstColon);
    int value = cmd.substring(secondColon + 1).toInt();
    bool pressed = (value != 0);
    
    if (setButton(button, pressed)) {
        Serial.println("✓ " + button + ": " + (pressed ? "ON" : "OFF"));
    } else {
        Serial.println("✗ Botón desconocido: " + button);
    }
}

void handleStickCommand(String cmd) {
    // Formato: STICK:AXIS:VALUE
    int firstColon = cmd.indexOf(':', 6);
    int secondColon = cmd.indexOf(':', firstColon + 1);
    
    if (firstColon == -1 || secondColon == -1) {
        Serial.println("✗ Formato incorrecto. Usa: STICK:LX:32");
        return;
    }
    
    String axis = cmd.substring(6, firstColon);
    int value = cmd.substring(secondColon + 1).toInt();
    
    if (setStickAxis(axis, value)) {
        Serial.println("✓ Stick " + axis + ": " + String(value));
    } else {
        Serial.println("✗ Eje desconocido: " + axis);
    }
}

void handleTriggerCommand(String cmd) {
    // Formato: TRIGGER:SIDE:VALUE
    int firstColon = cmd.indexOf(':', 8);
    int secondColon = cmd.indexOf(':', firstColon + 1);
    
    if (firstColon == -1 || secondColon == -1) {
        Serial.println("✗ Formato incorrecto. Usa: TRIGGER:L:15");
        return;
    }
    
    String side = cmd.substring(8, firstColon);
    int value = cmd.substring(secondColon + 1).toInt();
    
    if (setTrigger(side, value)) {
        Serial.println("✓ Trigger " + side + ": " + String(value));
    } else {
        Serial.println("✗ Trigger desconocido: " + side);
    }
}

//==============================================================================
// FUNCIONES DE CONTROL DEL GAMEPAD
//==============================================================================

bool setButton(String button, bool pressed) {
    if (button == "A") gamepad.btnA = pressed;
    else if (button == "B") gamepad.btnB = pressed;
    else if (button == "X") gamepad.btnX = pressed;
    else if (button == "Y") gamepad.btnY = pressed;
    else if (button == "L") gamepad.btnL = pressed;
    else if (button == "R") gamepad.btnR = pressed;
    else if (button == "ZL") gamepad.btnZL = pressed;
    else if (button == "ZR") gamepad.btnZR = pressed;
    else if (button == "PLUS" || button == "+") gamepad.btnPlus = pressed;
    else if (button == "MINUS" || button == "-") gamepad.btnMinus = pressed;
    else if (button == "HOME") gamepad.btnHome = pressed;
    else if (button == "UP") gamepad.dpadUp = pressed;
    else if (button == "DOWN") gamepad.dpadDown = pressed;
    else if (button == "LEFT") gamepad.dpadLeft = pressed;
    else if (button == "RIGHT") gamepad.dpadRight = pressed;
    else return false;
    
    return true;
}

bool setStickAxis(String axis, int value) {
    if (axis == "LX") {
        gamepad.leftStickX = constrain(value, 0, 63);
    }
    else if (axis == "LY") {
        gamepad.leftStickY = constrain(value, 0, 63);
    }
    else if (axis == "RX") {
        gamepad.rightStickX = constrain(value, 0, 31);
    }
    else if (axis == "RY") {
        gamepad.rightStickY = constrain(value, 0, 31);
    }
    else return false;
    
    return true;
}

bool setTrigger(String side, int value) {
    if (side == "L" || side == "LEFT") {
        gamepad.leftTrigger = constrain(value, 0, 31);
    }
    else if (side == "R" || side == "RIGHT") {
        gamepad.rightTrigger = constrain(value, 0, 31);
    }
    else return false;
    
    return true;
}

//==============================================================================
// CALLBACKS I2C
//==============================================================================

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

void onI2CRequest() {
    uint8_t response[16];
    int responseSize = 0;
    
    switch (currentRegister) {
        case 0x00:  // Datos del gamepad (lectura principal)
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
            // Respuesta por defecto (datos neutros)
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
                Serial.println("I2C: Encriptación desactivada");
            }
            break;
            
        case 0xFB:  // Configurar formato
            if (value == 0x00) {
                Serial.println("I2C: Formato configurado");
            }
            break;
            
        case 0xFE:  // Establecer tipo de dato
            if (value == 0x03) {
                Serial.println("I2C: Tipo DATA_TYPE_1 establecido");
                deviceInitialized = true;
            }
            break;
            
        default:
            // Cambiar registro actual para próxima lectura
            currentRegister = reg;
            break;
    }
}

//==============================================================================
// CODIFICACIÓN DE DATOS WII CLASSIC CONTROLLER
//==============================================================================

int encodeGamepadData(uint8_t* buffer) {
    // Codificar según formato DATA_TYPE_1 del Classic Controller
    // Referencia: GP2040-CE/lib/WiiExtension/extensions/ClassicExtension.cpp
    
    // Byte 0: [LX5-LX0][RX4-RX3]
    buffer[0] = (gamepad.leftStickX & 0x3F) | 
                ((gamepad.rightStickX & 0x18) << 1);
    
    // Byte 1: [LY5-LY0][RX2-RX1]  
    buffer[1] = (gamepad.leftStickY & 0x3F) | 
                ((gamepad.rightStickX & 0x06) << 5);
    
    // Byte 2: [RX0][LT4-LT2][RY4-RY0]
    buffer[2] = ((gamepad.rightStickX & 0x01) << 7) |
                ((gamepad.leftTrigger & 0x1C) << 2) |
                (gamepad.rightStickY & 0x1F);
    
    // Byte 3: [LT1-LT0][RT4-RT0]
    buffer[3] = ((gamepad.leftTrigger & 0x03) << 5) |
                (gamepad.rightTrigger & 0x1F);
    
    // Byte 4: Botones (invertidos: 0=presionado, 1=no presionado)
    // [DR][DD][DL][MINUS][HOME][PLUS][R][0]
    buffer[4] = (!gamepad.dpadRight << 7) |
                (!gamepad.dpadDown << 6) |
                (!gamepad.btnL << 5) |
                (!gamepad.btnMinus << 4) |
                (!gamepad.btnHome << 3) |
                (!gamepad.btnPlus << 2) |
                (!gamepad.btnR << 1) |
                (0 << 0);  // Bit no usado
    
    // Byte 5: Más botones
    // [ZL][B][Y][A][X][ZR][DL][DU]
    buffer[5] = (!gamepad.btnZL << 7) |
                (!gamepad.btnB << 6) |
                (!gamepad.btnY << 5) |
                (!gamepad.btnA << 4) |
                (!gamepad.btnX << 3) |
                (!gamepad.btnZR << 2) |
                (!gamepad.dpadLeft << 1) |
                (!gamepad.dpadUp << 0);
    
    return 6;  // Tamaño del paquete DATA_TYPE_1
}

//==============================================================================
// FUNCIONES DE ESTADO Y DEBUG
//==============================================================================

void updateStatusLED() {
    unsigned long now = millis();
    
    if (deviceInitialized) {
        // Parpadeo lento cuando está inicializado
        if (now - lastLedToggle > 1000) {
            ledState = !ledState;
            digitalWrite(LED_BUILTIN_PIN, ledState);
            lastLedToggle = now;
        }
    } else {
        // Parpadeo rápido cuando no está inicializado
        if (now - lastLedToggle > 200) {
            ledState = !ledState;
            digitalWrite(LED_BUILTIN_PIN, ledState);
            lastLedToggle = now;
        }
    }
}

void printStatus() {
    unsigned long now = millis();
    
    if (now - lastStatusPrint > 5000) {  // Cada 5 segundos
        Serial.println("Estado: " + String(deviceInitialized ? "CONECTADO" : "ESPERANDO") + 
                      " | Uptime: " + String(now/1000) + "s");
        lastStatusPrint = now;
    }
}

void printDetailedStatus() {
    Serial.println("\n" + String("=").substring(0, 50));
    Serial.println("ESTADO DETALLADO DEL GAMEPAD");
    Serial.println(String("=").substring(0, 50));
    
    Serial.println("Conexión I2C: " + String(deviceInitialized ? "ACTIVA" : "INACTIVA"));
    Serial.println("Registro actual: 0x" + String(currentRegister, HEX));
    
    Serial.println("\nSticks Analógicos:");
    Serial.println("  Left  X: " + String(gamepad.leftStickX) + "/63  Y: " + String(gamepad.leftStickY) + "/63");
    Serial.println("  Right X: " + String(gamepad.rightStickX) + "/31  Y: " + String(gamepad.rightStickY) + "/31");
    
    Serial.println("\nTriggers:");
    Serial.println("  Left: " + String(gamepad.leftTrigger) + "/31  Right: " + String(gamepad.rightTrigger) + "/31");
    
    Serial.println("\nBotones Principales:");
    Serial.println("  A:" + String(gamepad.btnA ? "ON " : "OFF") + 
                  " B:" + String(gamepad.btnB ? "ON " : "OFF") +
                  " X:" + String(gamepad.btnX ? "ON " : "OFF") +
                  " Y:" + String(gamepad.btnY ? "ON " : "OFF"));
    
    Serial.println("\nBumpers/Triggers:");
    Serial.println("  L:" + String(gamepad.btnL ? "ON " : "OFF") + 
                  " R:" + String(gamepad.btnR ? "ON " : "OFF") +
                  " ZL:" + String(gamepad.btnZL ? "ON " : "OFF") +
                  " ZR:" + String(gamepad.btnZR ? "ON " : "OFF"));
    
    Serial.println("\nSistema:");
    Serial.println("  +:" + String(gamepad.btnPlus ? "ON " : "OFF") + 
                  " -:" + String(gamepad.btnMinus ? "ON " : "OFF") +
                  " HOME:" + String(gamepad.btnHome ? "ON " : "OFF"));
    
    Serial.println("\nD-pad:");
    Serial.println("  ↑:" + String(gamepad.dpadUp ? "ON " : "OFF") + 
                  " ↓:" + String(gamepad.dpadDown ? "ON " : "OFF") +
                  " ←:" + String(gamepad.dpadLeft ? "ON " : "OFF") +
                  " →:" + String(gamepad.dpadRight ? "ON " : "OFF"));
    
    Serial.println(String("=").substring(0, 50) + "\n");
}

void printHelp() {
    Serial.println("\n" + String("-").substring(0, 40));
    Serial.println("COMANDOS DISPONIBLES:");
    Serial.println(String("-").substring(0, 40));
    Serial.println("BOTONES:");
    Serial.println("  BTN:A:1      - Presionar A");
    Serial.println("  BTN:A:0      - Soltar A");
    Serial.println("  BTN:HOME:1   - Presionar Home");
    Serial.println("  (A,B,X,Y,L,R,ZL,ZR,PLUS,MINUS,HOME,UP,DOWN,LEFT,RIGHT)");
    Serial.println("");
    Serial.println("STICKS:");
    Serial.println("  STICK:LX:32  - Left X al centro (0-63)");
    Serial.println("  STICK:RY:16  - Right Y al centro (0-31)");
    Serial.println("  (LX,LY=0-63, RX,RY=0-31)");
    Serial.println("");
    Serial.println("TRIGGERS:");
    Serial.println("  TRIGGER:L:31 - Left trigger máximo (0-31)");
    Serial.println("  TRIGGER:R:0  - Right trigger suelto");
    Serial.println("");
    Serial.println("UTILIDADES:");
    Serial.println("  STATUS       - Estado detallado");
    Serial.println("  RESET        - Resetear a neutro");
    Serial.println("  TEST         - Secuencia de prueba");
    Serial.println("  HELP         - Esta ayuda");
    Serial.println(String("-").substring(0, 40) + "\n");
}

void runTestSequence() {
    Serial.println("Iniciando secuencia de prueba...");
    
    // Test botones principales
    Serial.println("Test: Botones A,B,X,Y");
    setButton("A", true); delay(300); setButton("A", false); delay(200);
    setButton("B", true); delay(300); setButton("B", false); delay(200);
    setButton("X", true); delay(300); setButton("X", false); delay(200);
    setButton("Y", true); delay(300); setButton("Y", false); delay(200);
    
    // Test D-pad
    Serial.println("Test: D-pad circular");
    setButton("UP", true); delay(300); setButton("UP", false);
    setButton("RIGHT", true); delay(300); setButton("RIGHT", false);
    setButton("DOWN", true); delay(300); setButton("DOWN", false);
    setButton("LEFT", true); delay(300); setButton("LEFT", false);
    
    // Test sticks
    Serial.println("Test: Sticks analógicos");
    setStickAxis("LX", 0); delay(200);   // Izquierda
    setStickAxis("LX", 63); delay(200);  // Derecha  
    setStickAxis("LX", 32); delay(200);  // Centro
    setStickAxis("LY", 0); delay(200);   // Abajo
    setStickAxis("LY", 63); delay(200);  // Arriba
    setStickAxis("LY", 32); delay(200);  // Centro
    
    // Test triggers
    Serial.println("Test: Triggers");
    setTrigger("L", 31); delay(300); setTrigger("L", 0);
    setTrigger("R", 31); delay(300); setTrigger("R", 0);
    
    Serial.println("✓ Secuencia de prueba completada");
    Serial.println("El gamepad debería haber respondido en GP2040-CE");
}

//==============================================================================
// FIN DEL CÓDIGO
//==============================================================================