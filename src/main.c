#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "light_ws2812.h"
#include "usart.h"

#define PIANO 0x01
#define C4 60
#define FADE_STEP 2
#define COLS_BTN 4
#define LINES_BTN 4
#define NUM_BTN LINES_BTN * COLS_BTN
#define NUM_LED NUM_BTN

uint8_t mode = 0;

struct cRGB led[NUM_LED];
struct cRGB ledColors[NUM_LED];
uint8_t btnPressed[NUM_BTN];

uint8_t maxTime = 16 * 1;
uint8_t soundMatrix[32][NUM_BTN];
uint8_t time;

uint8_t noteOn = 0x90;
uint8_t noteOff = 0x80;
uint8_t instrumentSelect = 0xC0;

void IO_init() {
    // Butonul PB2 pentru schimbarea modului
    DDRB &= ~(1 << PB2);
    PORTB |= (1 << PB2);
    // Activez PIN CHANGE INTERRUPT ENABLE pentru PCINT15:8 (portul B)
    PCICR |= (1 << PCIE1);
    // Activez intreruperea pe butonul PB2
    PCMSK1 |= (1 << PCINT10);

    // Ledul PD7 pentru a semnala modul curent 
    // (stins pentru modul normal si aprins pentru modul looping)
    DDRD |= (1 << PD7);
    // Initial suntem in modul normal, deci ledul e stins
    PORTD &= ~(1 << PD7);

    // Liniile matricei, citire cu rezistenta de pull-up
    DDRA &= ~((1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3));
    PORTA |= (1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3);

    // Coloanele matricei, scriere VCC/GND
    // toate coloanele sunt pe VCC initial, fiind puse pe GND pe rand
    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);
    PORTC |= ((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));
}

/* initializeaza timer-ul 1
 * mod CTC
 */
void timer1_init() {
    // Setam OCR1A pe valoarea la care va intrerupe ceasul
    // pentru 16Hz
    OCR1A = 15625;

    // activeaza intreruperea pe OCR1A.
    TIMSK1 = (1 << OCIE1A);
    // mod de functionare in CTC cu TOP la OCR1A
    // Prescalerul este setat la 64
    TCCR1B = (3 << CS10) | (1 << WGM12);
}

struct cRGB color(uint8_t r, uint8_t g, uint8_t b) {
    struct cRGB rgb;
    rgb.r = r;
    rgb.g = g;
    rgb.b = b;

    return rgb;
}

void sendMIDI(uint8_t type, uint8_t channel) {
    uint8_t note = C4;
    uint8_t baseVelocity = 64;

    USART0_transmit(type | channel);
    USART0_transmit(note);
    USART0_transmit(baseVelocity);
}

void setLed(uint8_t ledIdx) {
    led[ledIdx] = ledColors[ledIdx];
    ws2812_setleds(led, NUM_LED);
}

void unsetLed(uint8_t ledIdx) {
    led[ledIdx] = color(0, 0, 0);
    ws2812_setleds(led, NUM_LED);   
}

void resetLeds(struct cRGB ledColor) {
    for (uint8_t i = 0; i < NUM_LED; i++) {
        if ((mode == 0 && btnPressed[i] == 0) || mode == 1)
            led[i] = ledColor;
    }
    ws2812_setleds(led, NUM_LED);
}

void setLedBtn(uint8_t btnIdx) {
    uint8_t ledIdx;    
    if (btnIdx > 3 && btnIdx < 8) {
        ledIdx = 4 + (7 - btnIdx);
    } else if (btnIdx > 11) {
        ledIdx = 12 + (15 - btnIdx);
    } else {
        ledIdx = btnIdx;
    }
    setLed(ledIdx);
}

void checkSound() {
    // Itereaza prin fiecare buton si vezi care
    // semnal midi trebuie trimis
    if (time == 0) {
        resetLeds(color(0, 0, 0));
    }

    for (uint8_t i = 0; i < NUM_BTN; i++) {
        if(soundMatrix[time][i] != 0) {
            sendMIDI(noteOn, i);
            setLedBtn(i);
        }
    }
}

ISR(PCINT1_vect) {
    // Daca PB2 este apasat, schimb modul de operare
    if ((PINB & (1 << PB2)) == 0) {
        mode = (mode + 1) % 2;
        // Setam ledul PD7 daca trecem la modul looping
        if (mode == 1) {
            PORTD |= (1 << PD7);
        } else {
            PORTD &= ~(1 << PD7);
        }

        // Such debounce
        _delay_us(10);
    }
}

ISR(TIMER1_COMPA_vect) {
    // Incrementeaza timpul
    time ++;
    time %= maxTime;

    // Verifica daca trebuie sa trimitem un semnal MIDI
    checkSound();

    // Stinge ledurile treptat
    if (mode == 0) {
        for(uint8_t i = 0; i < NUM_LED; i++) {
            if (btnPressed[i] == 0) {
                if (led[i].r <= FADE_STEP) {
                    led[i].r = 0;
                } else {
                    led[i].r -= FADE_STEP;
                }
                if (led[i].g <= FADE_STEP) {
                    led[i].g = 0;
                } else {
                    led[i].g -= FADE_STEP;
                }
                if (led[i].b <= FADE_STEP) {
                    led[i].b = 0;
                } else {
                    led[i].b -= FADE_STEP;
                }
            }
        }
        ws2812_setleds(led, NUM_LED);
    }
}

void checkPress(uint8_t PAx, uint8_t PCx) {
    uint8_t ledIdx;
    uint8_t btnIdx;

    // Banda de led e pusa putin diferit fata de cum sunt adresate butoanele
    if (PAx == 0 || PAx == 2) {
        ledIdx = PAx * 4 + PCx;
    } else {
        ledIdx = PAx * 4 + (3 - PCx);
    }

    btnIdx = PAx * 4 + PCx;

    // Verificam daca butonul dat este apasat sau lasat
    if(!(PINA & (1 << PAx)) && !btnPressed[ledIdx]) {  // Buton apasat
        btnPressed[ledIdx] = 1;
        if (mode == 0) {
        } else {
            soundMatrix[time][btnIdx] = 1;
        }
        sendMIDI(noteOn, btnIdx);
        setLed(ledIdx);
    } else if((PINA & (1 << PAx)) && btnPressed[ledIdx]) { // Buton lasat
        btnPressed[ledIdx] = 0;
        if (mode == 0) {
            sendMIDI(noteOff, btnIdx);
            unsetLed(ledIdx);
        }
    }
    _delay_us(10);
}

void color_seq_init() {
    for (uint8_t i = 0; i < 4; i++) {
        ledColors[i] = color(0, 15, 80);
    }
    for (uint8_t i = 4; i < 8; i++) {
        ledColors[i] = color(80, 20, 0);
    }
    for (uint8_t i = 8; i < 12; i++) {
        ledColors[i] = color(70, 70, 0);
    }
    for (uint8_t i = 12; i < 16; i++) {
        ledColors[i] = color(0, 80, 2);
    }
}

int main(void) {

    // Initializare secventa culori
    color_seq_init();

    // Configurare porturi folosite
    IO_init();

    // Initializare seriala
    USART0_init();

    // Activam intreruperile
    sei();

    // Initializare timer
    timer1_init();

	while (1) {
        // Iteram prin fiecare linie si coloana pentru a vedea
        // ce butoane sunt apasate
        for (uint8_t j = PC0; j <= PC3; j++) {
            // Setam coloana pe GND
            PORTC &= ~(1 << j);
            
            for (int i = PA0; i <= PA3; i++) {
                checkPress(i, j);
            }

            // Resetam coloana la VCC
            PORTC |= (1 << j);
        }
	}
}
