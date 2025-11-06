#!/usr/bin/env python3
"""
Script de prueba para el Serial Input Addon de GP2040-CE

Este script envía comandos de botones al GP2040-CE vía USB Serial.
Útil para testing y debugging del addon.
"""

import serial
import struct
import time
import sys

# Mapeo de botones
BUTTONS = {
    'A':      1 << 0,   # B1
    'B':      1 << 1,   # B2
    'X':      1 << 2,   # B3
    'Y':      1 << 3,   # B4
    'LB':     1 << 4,   # L1
    'RB':     1 << 5,   # R1
    'LT':     1 << 6,   # L2
    'RT':     1 << 7,   # R2
    'UP':     1 << 8,   # D-Up
    'DOWN':   1 << 9,   # D-Down
    'LEFT':   1 << 10,  # D-Left
    'RIGHT':  1 << 11,  # D-Right
    'SELECT': 1 << 12,  # S1/Back
    'START':  1 << 13,  # S2/Start
    'L3':     1 << 14,  # Left Stick
    'R3':     1 << 15,  # Right Stick
}

def send_buttons(ser, button_mask):
    """Envía un mask de botones al GP2040-CE"""
    data = struct.pack('<I', button_mask)
    ser.write(data)
    ser.flush()

def test_sequence(ser):
    """Ejecuta una secuencia de prueba"""
    print("Iniciando secuencia de prueba...")
    
    # Test individual de cada botón
    for name, mask in BUTTONS.items():
        print(f"Presionando {name}...")
        send_buttons(ser, mask)
        time.sleep(0.5)
        send_buttons(ser, 0)  # Soltar
        time.sleep(0.2)
    
    print("\nTest de combo: A+B+START")
    combo = BUTTONS['A'] | BUTTONS['B'] | BUTTONS['START']
    send_buttons(ser, combo)
    time.sleep(1)
    send_buttons(ser, 0)
    
    print("\nTest de D-Pad circular")
    dpad_sequence = ['UP', 'RIGHT', 'DOWN', 'LEFT']
    for direction in dpad_sequence:
        print(f"D-Pad {direction}")
        send_buttons(ser, BUTTONS[direction])
        time.sleep(0.3)
    send_buttons(ser, 0)
    
    print("\nTest completado!")

def interactive_mode(ser):
    """Modo interactivo para enviar botones"""
    print("\n=== Modo Interactivo ===")
    print("Botones disponibles:")
    for name in BUTTONS.keys():
        print(f"  {name}", end="  ")
    print("\n\nEscribe los nombres de botones separados por espacio (ej: A B START)")
    print("Escribe 'quit' para salir\n")
    
    try:
        while True:
            user_input = input("> ").strip().upper()
            
            if user_input == 'QUIT':
                break
            
            if not user_input:
                # Sin input = soltar todos los botones
                send_buttons(ser, 0)
                continue
            
            button_names = user_input.split()
            mask = 0
            
            for name in button_names:
                if name in BUTTONS:
                    mask |= BUTTONS[name]
                else:
                    print(f"Botón desconocido: {name}")
            
            send_buttons(ser, mask)
            print(f"Enviado: 0x{mask:04X}")
            
    except KeyboardInterrupt:
        print("\nInterrumpido por usuario")
    finally:
        # Soltar todos los botones al salir
        send_buttons(ser, 0)

def continuous_spam(ser, rate_hz=120):
    """Envía inputs continuamente a una tasa específica"""
    print(f"\n=== Modo Spam Continuo a {rate_hz} Hz ===")
    print("Presionando A continuamente. Ctrl+C para detener\n")
    
    delay = 1.0 / rate_hz
    count = 0
    
    try:
        start_time = time.time()
        while True:
            send_buttons(ser, BUTTONS['A'])
            time.sleep(delay)
            count += 1
            
            if count % rate_hz == 0:  # Cada segundo
                elapsed = time.time() - start_time
                actual_rate = count / elapsed
                print(f"Enviados: {count} paquetes, Rate actual: {actual_rate:.1f} Hz")
    
    except KeyboardInterrupt:
        print("\nDetenido")
    finally:
        send_buttons(ser, 0)

def main():
    if len(sys.argv) < 2:
        print("Uso: python3 test_serial_input.py <puerto_serial> [modo]")
        print("\nEjemplos:")
        print("  python3 test_serial_input.py /dev/ttyACM0")
        print("  python3 test_serial_input.py /dev/ttyACM0 test")
        print("  python3 test_serial_input.py /dev/ttyACM0 interactive")
        print("  python3 test_serial_input.py /dev/ttyACM0 spam")
        sys.exit(1)
    
    port = sys.argv[1]
    mode = sys.argv[2] if len(sys.argv) > 2 else 'test'
    
    try:
        print(f"Conectando a {port}...")
        ser = serial.Serial(port, 115200, timeout=1)
        print("Conectado!\n")
        
        # Dar tiempo al dispositivo
        time.sleep(0.5)
        
        if mode == 'test':
            test_sequence(ser)
        elif mode == 'interactive':
            interactive_mode(ser)
        elif mode == 'spam':
            continuous_spam(ser)
        else:
            print(f"Modo desconocido: {mode}")
            print("Modos disponibles: test, interactive, spam")
        
        ser.close()
        print("\nConexión cerrada")
        
    except serial.SerialException as e:
        print(f"Error al abrir puerto serial: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
