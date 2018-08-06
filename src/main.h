
#ifndef MAIN_H
#define MAIN_H

#ifndef F_CPU
#warning "F_CPU undefined, set to 16MHz"
#define F_CPU 16000000UL
#endif

#include <inttypes.h>

/* Buzzer hardware pins definitions */
#define BUZZER_ddr DDRC
#define BUZZER_port PORTC
#define BUZZER_bit PORTC5

/* Definitions for the Remote Controller Codes that control the alarm. */
#define INC_DIGIT_NUM_IRcommand 0x0D
#define DEC_DIGIT_NUM_IRcommand 0x19
#define INC_DIGIT_IRcommand 0x43
#define DEC_DIGIT_IRcommand 0x40
#define CLOCK_DONE_IRcommmand 0x44
#define SET_ALARM_IRcommand 0x46
#define ALARM_OFF_IRcommand 0x45

/* IR masks */
#define IR_receive_mask (ir.status & (1 << IR_RECEIVED))
#define IR_receive_mask_clear (ir.status &= ~(1 << IR_RECEIVED))
#define IR_hold_mask (ir.status & (1 << IR_KEYHOLD))

/* General definitions */
#define HIGH(x) (((x) >> 8) & 0xFF)
#define LOW(x) ((x)&0xFF)

/* Functions declarations */
void timer1_init(void);
void user_setTime(void);
void clockControl_incDigit(void);
void clockControl_decDigit(void);
void clockControl_incDigitNum(void);
void clockControl_decDigitNum(void);
void clockUpdateDisplay(void);
void user_setAlarm(void);
void alarmControl_incDigit(void);
void alarmControl_decDigit(void);
void alarmControl_incDigitNum(void);
void alarmControl_decDigitNum(void);
void alarmBuzzer_activate(void);

#endif