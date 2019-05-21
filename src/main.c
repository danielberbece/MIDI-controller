#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "light_ws2812.h"
#include "usart.h"

#define PIANO 0x01

struct cRGB led[16];
unsigned char btnPressed[16];

unsigned char noteOn = 0x90;
unsigned char noteOff = 0x80;
unsigned char baseVelocity = 64;
unsigned char instrumentSelect = 0xC0;

void checkPress(int PAx, int PCx) {
    // Set PCx to ground
    PORTC &= ~(1 << PCx);
    _delay_us(1);

    int led_idx;
    char note;

    if (PAx == 0 || PAx == 2) {
        led_idx = PAx * 4 + PCx;
    } else {
        led_idx = PAx * 4 + (3 - PCx);
    }
    // note = led_idx + 1;
    note = 60 + led_idx;

    // Check if button is pressed
    if(!(PINA & (1 << PAx)) && !btnPressed[led_idx]) {  // Light up the led
        led[led_idx].b = 100;
        ws2812_setleds(led, 16);
        btnPressed[led_idx] = 1;

        USART0_transmit(noteOn);
        USART0_transmit(note);
        USART0_transmit(baseVelocity);
    } 
    if ((PINA & (1 << PAx)) && btnPressed[led_idx]) {
        led[led_idx].b = 0;
        ws2812_setleds(led, 16);
        btnPressed[led_idx] = 0;

        USART0_transmit(noteOff);
        USART0_transmit(note);
        USART0_transmit(baseVelocity);
    }

    // Reset PCx to vcc
    PORTC |= (1 << PCx);
    _delay_us(10);
}

int main(void) {

 // configurare porturi folosite
    DDRB &= ~(1 << PB2);
    PORTB |= (1 << PB2);

    DDRA &= ~((1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3));
    PORTA |= (1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3);    // Citirea (cu rezistenta de pullup)

    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);  // Scrierea
    PORTC |= ((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));  // toate sunt pe vcc initial

    // Initializare seriala
    USART0_init();

	// int8_t PB2_flag = 0;
	while (1) {
        USART0_transmit(instrumentSelect);
        USART0_transmit(PIANO);

        // Loop through the buttons and check if pressed
        for (int i = PA0; i <= PA3; i++) {
            for (int j = PC0; j <= PC3; j++) {
                checkPress(i, j);
            }
        }
	}
}
