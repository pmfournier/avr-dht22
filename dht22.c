#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <stdlib.h>
#include <util/delay.h>
#include <string.h>
#include "usb_serial.h"

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

#define USB_SERIAL_PRINT(a) usb_serial_write((uint8_t *)a, sizeof(a)-1)

void
usb_serial_write_u16(uint16_t i)
{
	uint8_t result[20];

	utoa(i, result, 10);
	usb_serial_write(result, strlen(result));
	usb_serial_putchar('\n');
}

void
usb_serial_write_u16_nocr(uint16_t i)
{
	uint8_t result[20];

	utoa(i, result, 10);
	usb_serial_write(result, strlen(result));
}

void delay_s(int s)
{
	for (; s > 0; s--) {
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
	}
}

static inline void wait_for_zero(void)
{
	while ((PINC & 1) != 0) {}
}

static inline void wait_for_zero_while_counting(uint16_t *pcnt)
{
	uint16_t cnt = 0;
	while ((PINC & 1) != 0) {
		cnt++;
	}

	*pcnt = cnt;
}

static inline void wait_for_one(void)
{
	while ((PINC & 1) != 1) {}
}

int main(void)
{
	// set for 16 MHz clock, and make sure the LED is off
	CPU_PRESCALE(0);

	usb_init();

	/* Setup port at hi-pot, no pull up */
	DDRC &= ~1; // read mode
	PORTC &= ~1; // no pull up

	for (;;) {
		/* Wait 5 seconds */
		delay_s(2);

		/* Set port low for 80 us */
		PORTC &= ~1; // no pull up
		DDRC |= 1; //output
		_delay_ms(1);
		DDRC &= ~1; // read mode
		PORTC &= ~1; // no pull up

		/* Our job is done. Start frantic reading of the data */
		uint8_t bytes[5];
		memset(bytes, 0, sizeof(bytes));

		/* Just to be sure the logic level went back to one */
		wait_for_one();
		/* Wait for sensor to assert low */
		wait_for_zero();
		/* Wait for sensor to let go */
		wait_for_one();
		/* Wait for preamble to first real data byte */
		wait_for_zero();
		/* Wait for the first real data byte to be asserted */
		wait_for_one();

		int8_t i,j;
		for (i = 0; i < 5; i++) {
			for (j=7; j>=0; j--) {
				/* Wait for preamble to next byte */
				uint16_t count;
				wait_for_zero_while_counting(&count);

				if (count > 128) {
					bytes[i] |= (1<<j);
				} else {
				}
				wait_for_one();
			}
		}

		uint8_t checksum = bytes[0] + bytes[1] + bytes[2] + bytes[3];
		if (checksum != bytes[4]) {
			USB_SERIAL_PRINT("invalid\n");
			continue;
		}

		uint16_t temp = bytes[0];
		temp = temp * 256 + bytes[1];

		uint16_t rh = bytes[2];
		rh = rh * 256 + bytes[3];

		usb_serial_write_u16_nocr(temp);
		usb_serial_putchar(' ');
		usb_serial_write_u16_nocr(rh);
		usb_serial_putchar('\n');

		usb_serial_flush_output();
	}
}
