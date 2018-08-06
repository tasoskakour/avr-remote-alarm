/*
 * 24H Remote Alarm with Display.c
 * 
 * Created: 20/10/2016 7:10:35 μμ
 * Author : Tasos Kakouris
 
 * Real 24H Clock with Segment Display (+MAX7219)
 * Clock is setted by IR remote.
 * Timer1 is used for ticking
 */
#define F_CPU 16000000UL
#include <avr/io.h>
#include "main.h"
#include "MAX7219.h"
#include "libnecdecoder.h"
#include <avr/interrupt.h>
#include <util/delay.h>

/* Global variables */
volatile uint8_t digitPtr;
volatile int8_t clockDigits[4] = {0, 0, 0, 0};
volatile uint8_t tim1_cnt_compa = 0, clockDisplayFlag = 1;

volatile int8_t alarmDigits[4] = {0, 0, 0, 0};
volatile uint8_t alarmPtr, alarmSetFlag = 0, buzzerActivateFlag = 0;

/*
 * Interrupt Service Routine, TIMER1_COMPA_vect
 * --------------------------------------------
 * Called every 1 sec to update timer
 */
ISR(TIMER1_COMPA_vect)
{
	/* Update count */
	tim1_cnt_compa++;
	if (tim1_cnt_compa == 60)
	{
		tim1_cnt_compa = 0;
		clockDigits[3]++;
		if (clockDigits[3] > 9)
		{
			clockDigits[3] = 0;
			clockDigits[2]++;
			if (clockDigits[2] > 5)
			{
				clockDigits[2] = 0;
				clockDigits[1]++;
				if (clockDigits[0] < 2 && clockDigits[1] > 9)
				{
					clockDigits[1] = 0;
					clockDigits[0]++;
				}
				else if (clockDigits[0] == 2 && clockDigits[1] > 3)
				{
					clockDigits[1] = 0;
					clockDigits[0] = 0;
				}
			}
		}

		/* Update display */
		if (clockDisplayFlag)
			clockUpdateDisplay();

		/* Check for alarm */
		if (alarmSetFlag)
		{
			if ((alarmDigits[0] == clockDigits[0]) && (alarmDigits[1] == clockDigits[1]) && (alarmDigits[2] == clockDigits[2]) && (alarmDigits[3] == clockDigits[3]))
			{
				buzzerActivateFlag = 1;
				alarmSetFlag = 0;
			}
		}
	}
}

int main(void)
{
	MAX7219_init();
	MAX7219_decodeMode(2);
	ir_init();
	user_setTime(); // blocking function
	timer1_init();  // start timer...

	// Loop forever until user presses Play/Pause button
	IR_receive_mask_clear;
	while (1)
	{
		if (IR_receive_mask)
		{
			uint8_t check_val = ir.command;
			IR_receive_mask_clear;
			if (check_val == SET_ALARM_IRcommand)
			{
				clockDisplayFlag = 0; // Dont show real clock until user set alarm
				user_setAlarm();	  // mode button pressed, blocking function
				clockDisplayFlag = 1;
				alarmSetFlag = 1;
				IR_receive_mask_clear;
				clockUpdateDisplay();
				continue;
			}
		}
		if (buzzerActivateFlag)
		{
			alarmBuzzer_activate();
			alarmSetFlag = 0;
		}
	}
}

/**
 * Function: timer1_init
 * ---------------------
 * Initializes and starts Timer1 with settings:
 * 		CTC Mode, Compare Interrupt Enabled
 * 		Prescaler = 256
 * 		OCR1 = 62500
 * 
 */
void timer1_init(void)
{
	TCCR1A = 0;
	TCCR1B = 0b00001100;
	OCR1AH = HIGH(62500);
	OCR1AL = LOW(62500);
	TCNT1H = 0;
	TCNT1L = 0;
	TIMSK1 = 2;
	sei();
	clockUpdateDisplay();
	return;
}

/**
 * Function: user_setTime
 * ---------------------
 * User sets time with remote controller
 * 
 */
void user_setTime(void)
{
	MAX7219_setDigitNum(1, 0);
	MAX7219_setDigitNum(2, 0);
	MAX7219_setDigitNum(3, 0);
	MAX7219_setDigitNum(4, 0);
	uint8_t IRcommand = 0;
	while (1)
	{
		if ((IR_receive_mask == 0) && (IR_hold_mask == 0))
			continue;
		if ((IR_receive_mask == 1) && (IR_hold_mask == 0))
		{
			IRcommand = ir.command;
			IR_receive_mask_clear;
		}
		switch (IRcommand)
		{
		case INC_DIGIT_IRcommand:
			clockControl_incDigit();
			break;
		case DEC_DIGIT_IRcommand:
			clockControl_decDigit();
			break;
		case INC_DIGIT_NUM_IRcommand:
			clockControl_incDigitNum();
			break;
		case DEC_DIGIT_NUM_IRcommand:
			clockControl_decDigitNum();
			break;
		case CLOCK_DONE_IRcommmand:
			return;
		}
		_delay_ms(200); // safety blocking till IR correct receive
	}
	return;
}

/**
 * Function: clockControl_incDigit
 * ---------------------
 * Increases clock digit to the right
 * 
 */
void clockControl_incDigit(void)
{
	MAX7219_setDigitNum(digitPtr + 1, clockDigits[digitPtr]); //remove dot (.)
	digitPtr++;
	if (digitPtr > 3)
		digitPtr = 0;
	MAX7219_setDigitNum(digitPtr + 1, clockDigits[digitPtr] | 0b10000000); //add dot (.)
	return;
}

/**
 * Function: clockControl_decDigit
 * ---------------------
 * Decrease clock digit to the right
 * 
 */
void clockControl_decDigit(void)
{
	MAX7219_setDigitNum(digitPtr + 1, clockDigits[digitPtr]); //remove dot (.)
	digitPtr--;
	if (digitPtr > 3)
		digitPtr = 3;
	MAX7219_setDigitNum(digitPtr + 1, clockDigits[digitPtr] | 0b10000000); //add dot (.)
	return;
}

/**
 * Function: clockControl_incDigitNum
 * ---------------------
 * Increase clock digit num
 * 
 */
void clockControl_incDigitNum(void)
{
	clockDigits[digitPtr]++;
	switch (digitPtr)
	{
	case 0:
		if (clockDigits[0] > 2)
			clockDigits[0] = 0;
		if (clockDigits[0] == 2)
		{
			if (clockDigits[1] > 3)
				clockDigits[1] = 0;
			MAX7219_setDigitNum(2, 0);
		}
		break;
	case 1:
		if (clockDigits[0] < 2)
		{
			if (clockDigits[1] > 9)
				clockDigits[1] = 0;
		}
		else if (clockDigits[0] == 2)
		{
			if ((clockDigits[1]) > 3)
				clockDigits[1] = 0;
		}
		break;
	case 2:
		if (clockDigits[2] > 5)
			clockDigits[2] = 0;
		break;
	case 3:
		if (clockDigits[3] > 9)
			clockDigits[3] = 0;
		break;
	}
	/* Update display */
	MAX7219_setDigitNum(digitPtr + 1, clockDigits[digitPtr] | 0b10000000); //add dot (.)
	return;
}

/**
 * Function: clockControl_decDigitNum
 * ---------------------
 * Decrease clock digit num
 * 
 */
void clockControl_decDigitNum(void)
{
	clockDigits[digitPtr]--;
	switch (digitPtr)
	{
	case 0:
		if (clockDigits[0] < 0)
			clockDigits[0] = 2;
		if (clockDigits[0] == 2)
		{
			if (clockDigits[1] > 3)
				clockDigits[1] = 0;
			MAX7219_setDigitNum(2, 0);
		}
		break;
	case 1:
		if (clockDigits[0] < 2)
		{
			if (clockDigits[1] < 0)
				clockDigits[1] = 9;
		}
		else if (clockDigits[0] == 2)
		{
			if ((clockDigits[1]) < 0)
				clockDigits[1] = 3;
		}
		break;
	case 2:
		if (clockDigits[2] < 0)
			clockDigits[2] = 5;
		break;
	case 3:
		if (clockDigits[3] < 0)
			clockDigits[3] = 9;
		break;
	}
	/* Update display */
	MAX7219_setDigitNum(digitPtr + 1, clockDigits[digitPtr] | 0b10000000); //add dot (.)
	return;
}

/**
 * Function: clockUpdateDisplay
 * ---------------------
 * Updates the display with the correct format: XX.XX
 * 
 */
void clockUpdateDisplay(void)
{
	MAX7219_setDigitNum(4, clockDigits[3]);
	MAX7219_setDigitNum(3, clockDigits[2]);
	MAX7219_setDigitNum(2, clockDigits[1] | 0b10000000); //dot in middle
	MAX7219_setDigitNum(1, clockDigits[0]);
	return;
}

/**
 * Function: user_setAlarm
 * ---------------------
 * User sets alarm time
 * 
 */
void user_setAlarm(void)
{
	/*Initialize Alarm digits*/
	alarmDigits[0] = 0;
	alarmDigits[1] = 0;
	alarmDigits[2] = 0;
	alarmDigits[3] = 0;

	/*Initialize display with zeros so User will set alarm */
	MAX7219_set4digitNum(0);

	/*User alarm button interface same as clock*/
	uint8_t IRcommand = 0;

	while (1)
	{
		if ((IR_receive_mask == 0) && (IR_hold_mask == 0))
			continue;
		if ((IR_receive_mask == 1) && (IR_hold_mask == 0))
		{
			IRcommand = ir.command;
			IR_receive_mask_clear;
		}
		switch (IRcommand)
		{
		case INC_DIGIT_IRcommand:
			alarmControl_incDigit();
			break;
		case DEC_DIGIT_IRcommand:
			alarmControl_decDigit();
			break;
		case INC_DIGIT_NUM_IRcommand:
			alarmControl_incDigitNum();
			break;
		case DEC_DIGIT_NUM_IRcommand:
			alarmControl_decDigitNum();
			break;
		case CLOCK_DONE_IRcommmand:
			return;
		}
		_delay_ms(200); //safety blocking till IR correct receive
	}
	return;
}

/**
 * Function: alarmControl_incDigit
 * ---------------------
 * Increases alarm clock digit to the right
 * 
 */
void alarmControl_incDigit(void)
{
	MAX7219_setDigitNum(alarmPtr + 1, alarmDigits[alarmPtr]); //remove dot (.)
	alarmPtr++;
	if (alarmPtr > 3)
		alarmPtr = 0;
	MAX7219_setDigitNum(alarmPtr + 1, alarmDigits[alarmPtr] | 0b10000000); //add dot (.)
	return;
}

/**
 * Function: alarmControl_decDigit
 * ---------------------
 * Decreases alarm clock digit to the right
 * 
 */
void alarmControl_decDigit(void)
{
	MAX7219_setDigitNum(alarmPtr + 1, alarmDigits[alarmPtr]); //remove dot (.)
	alarmPtr--;
	if (alarmPtr > 3)
		alarmPtr = 3;
	MAX7219_setDigitNum(alarmPtr + 1, alarmDigits[alarmPtr] | 0b10000000); //add dot (.)
	return;
}

/**
 * Function: alarmControl_incDigitNum
 * ---------------------
 * Increases alarm clock digit
 * 
 */
void alarmControl_incDigitNum(void)
{
	alarmDigits[alarmPtr]++;
	switch (alarmPtr)
	{
	case 0:
		if (alarmDigits[0] > 2)
			alarmDigits[0] = 0;
		if (alarmDigits[0] == 2)
		{
			if (alarmDigits[1] > 3)
				alarmDigits[1] = 0;
			MAX7219_setDigitNum(2, 0);
		}
		break;
	case 1:
		if (alarmDigits[0] < 2)
		{
			if (alarmDigits[1] > 9)
				alarmDigits[1] = 0;
		}
		else if (alarmDigits[0] == 2)
		{
			if ((alarmDigits[1]) > 3)
				alarmDigits[1] = 0;
		}
		break;
	case 2:
		if (alarmDigits[2] > 5)
			alarmDigits[2] = 0;
		break;
	case 3:
		if (alarmDigits[3] > 9)
			alarmDigits[3] = 0;
		break;
	}

	/* Update display */
	MAX7219_setDigitNum(alarmPtr + 1, alarmDigits[alarmPtr] | 0b10000000); //add dot (.)
	return;
}

/**
 * Function: alarmControl_decDigitNum
 * ---------------------
 * Decreases alarm clock digit
 * 
 */
void alarmControl_decDigitNum(void)
{
	alarmDigits[alarmPtr]--;
	switch (alarmPtr)
	{
	case 0:
		if (alarmDigits[0] < 0)
			alarmDigits[0] = 2;
		if (alarmDigits[0] == 2)
		{
			if (alarmDigits[1] > 3)
				alarmDigits[1] = 0;
			MAX7219_setDigitNum(2, 0);
		}
		break;
	case 1:
		if (alarmDigits[0] < 2)
		{
			if (alarmDigits[1] < 0)
				alarmDigits[1] = 9;
		}
		else if (alarmDigits[0] == 2)
		{
			if ((alarmDigits[1]) < 0)
				alarmDigits[1] = 3;
		}
		break;
	case 2:
		if (alarmDigits[2] < 0)
			alarmDigits[2] = 5;
		break;
	case 3:
		if (alarmDigits[3] < 0)
			alarmDigits[3] = 9;
		break;
	}

	/* Update display */
	MAX7219_setDigitNum(alarmPtr + 1, alarmDigits[alarmPtr] | 0b10000000); //add dot (.)
	return;
}

/**
 * Function: alarmControl_decDigitNum
 * ---------------------
 * Activates buzzer for 10 seconds until user presses off
 * 
 */
void alarmBuzzer_activate(void)
{
	clockDisplayFlag = 0;
	BUZZER_ddr |= (1 << BUZZER_bit);
	BUZZER_port |= (1 << BUZZER_bit);
	IR_receive_mask_clear;
	for (int i = 0; i < 100; i++)
	{
		if (IR_receive_mask)
		{
			uint8_t check_val = ir.command;
			IR_receive_mask_clear;
			/* Check to turn off buzzer */
			if (check_val == ALARM_OFF_IRcommand)
				break;
		}
		/* 10 seconds delay total, unless user presses OFF to turn off buzzer */
		_delay_ms(100);
	}

	/*Turn off buzzer */
	BUZZER_port &= ~(1 << BUZZER_bit);
	clockDisplayFlag = 1;
	buzzerActivateFlag = 0;
	_delay_ms(200); //safety

	return;
}
