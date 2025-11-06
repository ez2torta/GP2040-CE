#ifndef _SerialInput_H
#define _SerialInput_H

#include "gpaddon.h"
#include "pico/stdlib.h"
#include "storagemanager.h"
#include "gamepad/GamepadState.h"

#ifndef SERIAL_INPUT_ENABLED
#define SERIAL_INPUT_ENABLED 0
#endif

// SerialInput Module Name
#define SerialInputName "SerialInput"

// Serial Input Configuration
#define SERIAL_INPUT_BUFFER_SIZE 64
#define SERIAL_INPUT_BAUD_RATE 115200
#define SERIAL_INPUT_EXPECTED_RATE_HZ 120

// Protocol: 4-byte uint32_t bitmask
// bit 0  = B1/A        bit 8  = D-Up  
// bit 1  = B2/B        bit 9  = D-Down
// bit 2  = B3/X        bit 10 = D-Left
// bit 3  = B4/Y        bit 11 = D-Right
// bit 4  = L1/LB       bit 12 = S1/Back/Select
// bit 5  = R1/RB       bit 13 = S2/Start
// bit 6  = L2/LT       bit 14 = L3/Left Stick
// bit 7  = R2/RT       bit 15 = R3/Right Stick

class SerialInputAddon : public GPAddon {
public:
    virtual bool available();
    virtual void setup();
    virtual void process() {}
    virtual void preprocess();
    virtual void postprocess(bool sent) {}
    virtual void reinit() {}
    virtual std::string name() { return SerialInputName; }
private:
    void processInputBitmask(uint32_t inputMask);
    uint8_t receiveBuffer[SERIAL_INPUT_BUFFER_SIZE];
    uint32_t bufferIndex;
    uint32_t lastValidInput;
};

#endif  // _SerialInput_H