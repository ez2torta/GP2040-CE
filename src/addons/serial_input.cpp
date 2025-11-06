#include "addons/serial_input.h"
#include "storagemanager.h"

// Usando USB CDC (ya configurado en TinyUSB por GP2040-CE)
#include "tusb.h"

bool SerialInputAddon::available() {
    return Storage::getInstance().getAddonOptions().serialInputOptions.enabled;
}

void SerialInputAddon::setup() {
    bufferIndex = 0;
    lastValidInput = 0;
    
    // USB CDC ya está inicializado por GP2040-CE
    // No necesitamos inicializar nada adicional
}

void SerialInputAddon::preprocess() {
    // Verificar si hay datos disponibles en el puerto CDC
    // CDC interface 0 es el puerto serial estándar en GP2040-CE
    if (!tud_cdc_available()) {
        return;
    }

    // Leer datos disponibles
    while (tud_cdc_available()) {
        uint8_t byte;
        uint32_t count = tud_cdc_read(&byte, 1);
        
        if (count == 0) {
            break;
        }

        receiveBuffer[bufferIndex++] = byte;

        // Si tenemos 4 bytes completos (uint32_t)
        if (bufferIndex >= 4) {
            // Reconstruir el uint32_t (little-endian)
            uint32_t inputMask = (uint32_t)receiveBuffer[0]
                               | ((uint32_t)receiveBuffer[1] << 8)
                               | ((uint32_t)receiveBuffer[2] << 16)
                               | ((uint32_t)receiveBuffer[3] << 24);

            // Procesar el input
            processInputBitmask(inputMask);
            lastValidInput = inputMask;

            // Reset buffer
            bufferIndex = 0;
        }

        // Protección contra overflow del buffer
        if (bufferIndex >= SERIAL_INPUT_BUFFER_SIZE) {
            bufferIndex = 0;
        }
    }
}

void SerialInputAddon::processInputBitmask(uint32_t inputMask) {
    Gamepad * gamepad = Storage::getInstance().GetGamepad();
    
    // Mapeo de bits según la especificación:
    // bit 0  = B1/A        bit 8  = D-Up  
    // bit 1  = B2/B        bit 9  = D-Down
    // bit 2  = B3/X        bit 10 = D-Left
    // bit 3  = B4/Y        bit 11 = D-Right
    // bit 4  = L1/LB       bit 12 = S1/Back/Select
    // bit 5  = R1/RB       bit 13 = S2/Start
    // bit 6  = L2/LT       bit 14 = L3/Left Stick
    // bit 7  = R2/RT       bit 15 = R3/Right Stick

    // Botones de acción (B1-B4)
    if (inputMask & (1 << 0)) gamepad->state.buttons |= GAMEPAD_MASK_B1;
    if (inputMask & (1 << 1)) gamepad->state.buttons |= GAMEPAD_MASK_B2;
    if (inputMask & (1 << 2)) gamepad->state.buttons |= GAMEPAD_MASK_B3;
    if (inputMask & (1 << 3)) gamepad->state.buttons |= GAMEPAD_MASK_B4;

    // Bumpers (L1/R1)
    if (inputMask & (1 << 4)) gamepad->state.buttons |= GAMEPAD_MASK_L1;
    if (inputMask & (1 << 5)) gamepad->state.buttons |= GAMEPAD_MASK_R1;

    // Triggers (L2/R2)
    if (inputMask & (1 << 6)) gamepad->state.buttons |= GAMEPAD_MASK_L2;
    if (inputMask & (1 << 7)) gamepad->state.buttons |= GAMEPAD_MASK_R2;

    // D-Pad
    if (inputMask & (1 << 8))  gamepad->state.dpad |= GAMEPAD_MASK_UP;
    if (inputMask & (1 << 9))  gamepad->state.dpad |= GAMEPAD_MASK_DOWN;
    if (inputMask & (1 << 10)) gamepad->state.dpad |= GAMEPAD_MASK_LEFT;
    if (inputMask & (1 << 11)) gamepad->state.dpad |= GAMEPAD_MASK_RIGHT;

    // Select/Start (S1/S2)
    if (inputMask & (1 << 12)) gamepad->state.buttons |= GAMEPAD_MASK_S1;
    if (inputMask & (1 << 13)) gamepad->state.buttons |= GAMEPAD_MASK_S2;

    // Stick buttons (L3/R3)
    if (inputMask & (1 << 14)) gamepad->state.buttons |= GAMEPAD_MASK_L3;
    if (inputMask & (1 << 15)) gamepad->state.buttons |= GAMEPAD_MASK_R3;
}