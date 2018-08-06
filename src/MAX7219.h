#ifndef MAX7219_H
#define MAX7219_H

#ifndef F_CPU
#define F_CPU 16000000UL // 16MHz clock
#endif

#include <inttypes.h>

// Outputs, pin definitions
#define PIN_SCK PORTB5
#define PIN_MOSI PORTB3
#define PIN_SS PORTB2
#define SPI_ddr DDRB
#define MAX7219_LOAD1 PORTB |= (1 << PIN_SS)
#define MAX7219_LOAD0 PORTB &= ~(1 << PIN_SS)

/*
 * Constants below here. 
 * Better not change anything.
 */

// Register addresses (D11:D8)
#define DECODE_MODE 0x09
#define INTENSITY 0x0A
#define SCANLIMIT 0x0B
#define SHUTDOWN 0x0C
#define DISPLAYTEST 0x0F

// Data  that sets the above modes (D7:D0)
// for Shutdown:
#define SHUTDOWN_MODE 0
#define NORMAL_MODE 1
// for Decode Mode :
#define DECODE_MODE_OFF 0
#define DECODE_MODE_DIG0 1
#define DECODE_MODE_DIG0to3 0x0F
#define DECODE_MODE_DIGALL 0xFF
// for Display test:
#define DISPLAYTEST_MODE_OFF 0 // Normal mode
#define DISPLAYTEST_MODE_ON 1

// Functions that the user can call
void MAX7219_init(void);
void MAX7219_intensity(uint8_t intensityValue);
void MAX7219_displayTest(uint8_t displayTest);
void MAX7219_scanLimit(uint8_t scanLimit);
void MAX7219_decodeMode(uint8_t decodeMode);
void MAX7219_shutdown(uint8_t shutdownFlag);
void MAX7219_setDigitNum(uint8_t digit, uint8_t number);
void MAX7219_set4digitNum(uint16_t number);

#endif
