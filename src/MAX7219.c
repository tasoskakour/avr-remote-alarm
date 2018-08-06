/*
 * lib_MAX7219.c
 *
 * Created: 12/10/2016 3:49:37 μμ
 * Author : Tasos Kakouris
 */

/*
 * MAX7219 Shift Register Structure
 * --------------------------------
 * D15|D14|D13|D12|D11|D10|D9|D8|D7|D6|D5|D4|D3|D2|D1|D0
 * D15:D12: No care.
 * D11:D8: Address.
 * D7-D0: Data (MSB=D7, LSB=D0).
 */
#include <avr/io.h>
#include "MAX7219.h"
#define F_CPU 16000000UL
#include <util/delay.h>

static uint8_t globalScanLimitNum;

/*
 * Function: spiSendByte
 * ---------------------
 * Sends a byte via the SPI hardware and returns when the transfer is complete
 * 
 * byte: the byte  to be sent
 *
 */
static inline void spiSendByte(uint8_t byte)
{
	SPDR = byte;
	while (!(SPSR & (1 << SPIF)))
		;
	return;
}

/*
 * Function: MAX7219_sendCommand
 * -----------------------------
 * Sends the 16 bit data to the MAX7219 using Big-Endian Protocol:
 *	 Address 8-upper bits, Data 8-lower bits
 * 
 * address: the address to be set
 * data: the data to be sent
 */
static inline void MAX7219_sendCommand(uint8_t address, uint8_t data)
{
	MAX7219_LOAD0;
	spiSendByte(address);
	spiSendByte(data);
	MAX7219_LOAD1;
	return;
}

/*
 * Function: MAX7219_setDigitNum
 * -----------------------------
 * Higher-level function to be called from the main program
 * 
 * digit: The nth-digit to be set
 * number: The number to be set to the digit
*/
void MAX7219_setDigitNum(uint8_t digit, uint8_t number)
{
	if ((digit == 0) || (digit > 8))
		return; // 0: no-op, >8: error digit
	MAX7219_sendCommand(digit, number);
	return;
}

/*
 * Function: MAX7219_shutdownMode
 * ------------------------------
 * Sets the shutdown mode of driver
 * 
 * shutdownFlag: if set to 1, the display is off, else is in normal mode
 */
void MAX7219_shutdown(uint8_t shutdownFlag)
{
	if (shutdownFlag == 0)
	{
		MAX7219_sendCommand(SHUTDOWN, NORMAL_MODE);
	}
	else
	{
		MAX7219_sendCommand(SHUTDOWN, SHUTDOWN_MODE);
	}
	return;
}

/*
 * Function: MAX7219_decodeMode
 * ----------------------------
 * Sets decode mode for digits
 *
 * decodeMode: can be 0, 1, 2, 3
 */
void MAX7219_decodeMode(uint8_t decodeMode)
{
	switch (decodeMode)
	{
	case 0:
		MAX7219_sendCommand(DECODE_MODE, DECODE_MODE_OFF);
		break;
	case 1:
		MAX7219_sendCommand(DECODE_MODE, DECODE_MODE_DIG0);
		break;
	case 2:
		MAX7219_sendCommand(DECODE_MODE, DECODE_MODE_DIG0to3);
		break;
	case 3:
		MAX7219_sendCommand(DECODE_MODE, DECODE_MODE_DIGALL);
		break;
	}
	return;
}

/*
 * Function: MAX7219_scanLimit 
 * ---------------------------
 * Sets how many digits are displayed
 *
 * scanLimit: can be 1-8
 */
void MAX7219_scanLimit(uint8_t scanLimit)
{
	if ((scanLimit > 8) || (scanLimit == 0))
		return; // error
	MAX7219_sendCommand(SCANLIMIT, scanLimit - 1);
	globalScanLimitNum = scanLimit;
	return;
}

/*
 * Function: MAX7219_displayTest 
 * -----------------------------
 * Performs displayTest (turns on all the leds)
 *
 * displayTest: 0 or 1 
*/
void MAX7219_displayTest(uint8_t displayTest)
{
	if (displayTest == 0)
	{
		MAX7219_sendCommand(DISPLAYTEST, DISPLAYTEST_MODE_OFF);
	}
	else
	{
		MAX7219_sendCommand(DISPLAYTEST, DISPLAYTEST_MODE_ON);
	}
	return;
}

/*
 * Function: MAX7219_intensity 
 * ---------------------------
 * Sets the intensity of the leds by PWM: 
 *		User must enter a value from 0 (dimmest) to 15 (lightest)
 *
 * intensityValue: 0 to 15 
 */
void MAX7219_intensity(uint8_t intensityValue)
{
	if (intensityValue > 15)
		return; // error
	MAX7219_sendCommand(INTENSITY, intensityValue);
	return;
}

/*
 * Function: MAX7219_set4digitNum 
 * ------------------------------
 * Shows 4 digit number in bcd form
 *
 * number: The number to be shown
 */
void MAX7219_set4digitNum(uint16_t number)
{
	if (number < 10)
	{ // Special case when number < 10
		MAX7219_setDigitNum(4, number);
		if (number == 0)
		{
			MAX7219_setDigitNum(1, 0);
			MAX7219_setDigitNum(2, 0);
			MAX7219_setDigitNum(3, 0);
		}
		return;
	}
	uint16_t deca = 1000, temp = number;
	uint8_t digit, digitptr = 1;
	// Binary to BCD 4-digit
	for (int decCount = 0; decCount < 4; decCount++)
	{
		digit = 0;
		while (temp >= deca)
		{
			temp = temp - deca;
			digit++;
		}
		MAX7219_setDigitNum(digitptr, digit);
		deca = deca / 10;
		digitptr++;
	}
	return;
}

/*
 * Function: MAX7219_init
 * ----------------------
 * Sets up output ports and init's MAX7219 settings
 */
void MAX7219_init(void)
{
	// Set up SPI ports
	SPI_ddr |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS);

	// SPI Enable, Master mode, Prescaler 64
	SPCR |= (1 << SPE) | (1 << MSTR) | (1 << SPR1);

	// Sets decode mode to zero (by default)
	MAX7219_decodeMode(0);

	// Sets scanlimit = DIG0 by default
	MAX7219_scanLimit(4);

	// Deactivates display test
	MAX7219_displayTest(0);

	// Sets up an initial Intensity Level (above middle)
	MAX7219_intensity(10);

	// Sets Shutdown setting to Normal Mode
	MAX7219_shutdown(0);

	// Initialize digits
	for (int i = 0; i < globalScanLimitNum; i++)
	{
		MAX7219_setDigitNum(i, 0);
	}
	return;
}
