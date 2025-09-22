// Ejecución principal: captura y hexdump de reportes HID de un joystick USB

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "pico/binary_info.h"

#include "pio_usb.h"

// Variable global para habilitar/deshabilitar el debug hexdump
bool debug_hexdump_enabled = false;

static usb_device_t *usb_device = NULL;

// Configura los pines GPIO que deseas controlar
const uint GPIO_PIN_UP = 2;
const uint GPIO_PIN_DOWN = 3;
const uint GPIO_PIN_LEFT = 4;
const uint GPIO_PIN_RIGHT = 5;

const uint GPIO_PIN_BUTTON_1 = 6;
const uint GPIO_PIN_BUTTON_2 = 7;
const uint GPIO_PIN_BUTTON_3 = 8;
const uint GPIO_PIN_BUTTON_4 = 9;
const uint GPIO_PIN_BUTTON_5 = 10;
const uint GPIO_PIN_BUTTON_6 = 11;
const uint GPIO_PIN_BUTTON_7 = 12;
const uint GPIO_PIN_BUTTON_8 = 13;
const uint GPIO_PIN_BUTTON_9 = 18;   // Select
const uint GPIO_PIN_BUTTON_10 = 19;  // Start
const uint GPIO_PIN_HOME = 20;       // Home (botón sistema)

// Helper para poner los 4 pines del D-Pad en "no pulsado" (alto)
static inline void dpad_neutral()
{
	gpio_put(GPIO_PIN_UP, 1);
	gpio_put(GPIO_PIN_DOWN, 1);
	gpio_put(GPIO_PIN_LEFT, 1);
	gpio_put(GPIO_PIN_RIGHT, 1);
}

// El D-Pad viene como un HAT en el byte 2:
// 0=Up, 1=Up-Right, 2=Right, 3=Down-Right, 4=Down, 5=Down-Left, 6=Left, 7=Up-Left, 8=Neutral
void procesar_dpad_hat(uint8_t hat)
{
	// Por defecto, soltar todas las direcciones
	dpad_neutral();

	switch (hat)
	{
	case 0x00: // Arriba
		gpio_put(GPIO_PIN_UP, 0);
		break;
	case 0x01: // Arriba-Derecha
		gpio_put(GPIO_PIN_UP, 0);
		gpio_put(GPIO_PIN_RIGHT, 0);
		break;
	case 0x02: // Derecha
		gpio_put(GPIO_PIN_RIGHT, 0);
		break;
	case 0x03: // Abajo-Derecha
		gpio_put(GPIO_PIN_RIGHT, 0);
		gpio_put(GPIO_PIN_DOWN, 0);
		break;
	case 0x04: // Abajo
		gpio_put(GPIO_PIN_DOWN, 0);
		break;
	case 0x05: // Abajo-Izquierda
		gpio_put(GPIO_PIN_DOWN, 0);
		gpio_put(GPIO_PIN_LEFT, 0);
		break;
	case 0x06: // Izquierda
		gpio_put(GPIO_PIN_LEFT, 0);
		break;
	case 0x07: // Arriba-Izquierda
		gpio_put(GPIO_PIN_UP, 0);
		gpio_put(GPIO_PIN_LEFT, 0);
		break;
	case 0x08: // Neutro
	default:
		// Nada, ya están en alto
		break;
	}
}

// Mapea botones según el estudio del reporte:
// Byte0:
//  bit0=Triángulo(0x01) bit1=Círculo(0x02) bit2=X(0x04) bit3=Cuadrado(0x08)
//  bit4=L1(0x10) bit5=R1(0x20) bit6=L2(0x40) bit7=R2(0x80)
// Byte1:
//  bit0=Select(0x01) bit1=Start(0x02) bit4=Home(0x10)
void procesar_botones(uint8_t b0, uint8_t b1)
{
	bool tri   = (b0 >> 0) & 0x01; // Triángulo
	bool circ  = (b0 >> 1) & 0x01; // Círculo
	bool equis = (b0 >> 2) & 0x01; // X
	bool cuad  = (b0 >> 3) & 0x01; // Cuadrado
	bool l1    = (b0 >> 4) & 0x01;
	bool r1    = (b0 >> 5) & 0x01;
	bool l2    = (b0 >> 6) & 0x01;
	bool r2    = (b0 >> 7) & 0x01;

	bool select_btn = (b1 >> 0) & 0x01;
	bool start_btn  = (b1 >> 1) & 0x01;
	bool home_btn   = (b1 >> 4) & 0x01;

	// Las salidas están en pull-up, así que "pulsado" = LOW
	gpio_put(GPIO_PIN_BUTTON_1, !tri);
	gpio_put(GPIO_PIN_BUTTON_2, !circ);
	gpio_put(GPIO_PIN_BUTTON_3, !equis);
	gpio_put(GPIO_PIN_BUTTON_4, !cuad);
	gpio_put(GPIO_PIN_BUTTON_5, !l1);
	gpio_put(GPIO_PIN_BUTTON_6, !r1);
	gpio_put(GPIO_PIN_BUTTON_7, !l2);
	gpio_put(GPIO_PIN_BUTTON_8, !r2);
	gpio_put(GPIO_PIN_BUTTON_9, !select_btn);
	gpio_put(GPIO_PIN_BUTTON_10, !start_btn);
	gpio_put(GPIO_PIN_HOME, !home_btn);
}

// Procesa el paquete HID recibido y actualiza los pines
void procesar_paquete_hid(const uint8_t *data, int len)
{
	// Esperamos al menos 3 bytes: b0(botones principales), b1(botones secundarios), b2(dpad hat)
	if (len < 3) return;

	uint8_t b0 = data[0];
	uint8_t b1 = data[1];
	uint8_t hat = data[2];

	procesar_dpad_hat(hat);
	procesar_botones(b0, b1);
}

// Imprime un buffer en formato hexdump (hex + ASCII)
void print_hexdump(const uint8_t *data, int len)
{
	const int bytes_per_line = 16;
	for (int i = 0; i < len; i += bytes_per_line)
	{
		printf("  %04x: ", i);
		for (int j = 0; j < bytes_per_line; ++j)
		{
			if (i + j < len) printf("%02x ", data[i + j]); else printf("   ");
		}
		printf(" ");
		for (int j = 0; j < bytes_per_line; ++j)
		{
			if (i + j < len)
			{
				char c = data[i + j];
				printf("%c", isprint((unsigned char)c) ? c : '.');
			}
			else
			{
				printf(" ");
			}
		}
		printf("\n");
	}
}

void core1_main()
{
	sleep_ms(10);

	// To run USB SOF interrupt in core1, create alarm pool in core1.
	static pio_usb_configuration_t config = PIO_USB_DEFAULT_CONFIG;
	config.alarm_pool = (void *)alarm_pool_create(2, 1);
	usb_device = pio_usb_host_init(&config);

	while (true)
	{
		pio_usb_host_task();
	}
}

int main()
{
	// default 125MHz is not appropreate. Sysclock should be multiple of 12MHz.
	set_sys_clock_khz(120000, true);

	stdio_init_all();

	// Configura los pines GPIO
	gpio_init(GPIO_PIN_UP);
	gpio_init(GPIO_PIN_DOWN);
	gpio_init(GPIO_PIN_LEFT);
	gpio_init(GPIO_PIN_RIGHT);
	gpio_init(GPIO_PIN_BUTTON_1);
	gpio_init(GPIO_PIN_BUTTON_2);
	gpio_init(GPIO_PIN_BUTTON_3);
	gpio_init(GPIO_PIN_BUTTON_4);
	gpio_init(GPIO_PIN_BUTTON_5);
	gpio_init(GPIO_PIN_BUTTON_6);
	gpio_init(GPIO_PIN_BUTTON_7);
	gpio_init(GPIO_PIN_BUTTON_8);
	gpio_init(GPIO_PIN_BUTTON_9);
	gpio_init(GPIO_PIN_BUTTON_10);
	gpio_init(GPIO_PIN_HOME);

	gpio_set_dir(GPIO_PIN_UP, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_DOWN, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_LEFT, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_RIGHT, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_1, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_2, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_3, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_4, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_5, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_6, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_7, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_8, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_9, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_BUTTON_10, GPIO_OUT);
	gpio_set_dir(GPIO_PIN_HOME, GPIO_OUT);

	gpio_pull_up(GPIO_PIN_UP);
	gpio_pull_up(GPIO_PIN_DOWN);
	gpio_pull_up(GPIO_PIN_LEFT);
	gpio_pull_up(GPIO_PIN_RIGHT);
	gpio_pull_up(GPIO_PIN_BUTTON_1);
	gpio_pull_up(GPIO_PIN_BUTTON_2);
	gpio_pull_up(GPIO_PIN_BUTTON_3);
	gpio_pull_up(GPIO_PIN_BUTTON_4);
	gpio_pull_up(GPIO_PIN_BUTTON_5);
	gpio_pull_up(GPIO_PIN_BUTTON_6);
	gpio_pull_up(GPIO_PIN_BUTTON_7);
	gpio_pull_up(GPIO_PIN_BUTTON_8);
	gpio_pull_up(GPIO_PIN_BUTTON_9);
	gpio_pull_up(GPIO_PIN_BUTTON_10);
	gpio_pull_up(GPIO_PIN_HOME);

	sleep_ms(10);

	multicore_reset_core1();
	multicore_launch_core1(core1_main);

	while (true)
	{
		if (usb_device != NULL)
		{
			for (int dev_idx = 0; dev_idx < PIO_USB_DEVICE_CNT; dev_idx++)
			{
				usb_device_t *device = &usb_device[dev_idx];
				if (!device->connected) continue;

				for (int ep_idx = 0; ep_idx < PIO_USB_DEV_EP_CNT; ep_idx++)
				{
					endpoint_t *ep = pio_usb_get_endpoint(device, ep_idx);
					if (ep == NULL) break;

					uint8_t temp[64];
					int len = pio_usb_get_in_data(ep, temp, sizeof(temp));
					if (len > 0)
					{
						if (debug_hexdump_enabled)
						{
							printf("%04x:%04x EP 0x%02x (%d bytes):\n", device->vid, device->pid, ep->ep_num, len);
							print_hexdump(temp, len);
						}
						procesar_paquete_hid(temp, len);
					}
				}
			}
		}
		stdio_flush();
		sleep_us(10);
	}
}
