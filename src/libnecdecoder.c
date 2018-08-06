#include <avr/io.h>
#include <avr/interrupt.h>
#include "libnecdecoder.h"


volatile uint8_t ir_state;
volatile uint8_t ir_bitctr;
#ifdef PROTOCOL_NEC_EXTENDED
volatile uint8_t ir_tmp_address_l;
volatile uint8_t ir_tmp_address_h;
#else
volatile uint8_t ir_tmp_address;
#endif
volatile uint8_t ir_tmp_command;
volatile uint8_t ir_tmp_keyhold;
volatile uint8_t ir_tmp_ovf;


// ###### Initializes ir function ######
void ir_init( void )
{
	// tAGC_burst = 9ms, tBIT = 0.56ms
	#if defined (__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__)
	// Timer: 8bit, Clock: 8MHz, Prescaler 1024, Overflow = 32.768ms, Tick = 0.128ms
	TCCR0A &= ~( (1<<COM0A0) | (1<<COM0A1) | (1<<COM0B0) | (1<<COM0B1) | (1<<WGM00) | (1<<WGM01) );
	TCCR0B |= (1<<CS00) | (1<<CS02);
	TIMSK0 |= (1<<TOIE0);
	
	// Interrupt 0 (PD2): Inverted signal input, triggered by logical change
	DDRD   &= ~(1<<PD2);
	EICRA  |= (1<<ISC00);
	EIMSK  |= (1<<INT0);
	#elif defined (__AVR_ATtiny2313__) || (__AVR_ATtiny4313__)
	// Timer: 8bit, Clock: 8MHz, Prescaler 1024, Overflow = 32.768ms, Tick = 0.128ms
	TCCR0A &= ~( (1<<COM0A0) | (1<<COM0A1) | (1<<COM0B0) | (1<<COM0B1) | (1<<WGM00) | (1<<WGM01) );
	TCCR0B |= (1<<CS00) | (1<<CS02);
	TIMSK  |= (1<<TOIE0);

	// Interrupt 0 (PD2): Inverted signal input, triggered by logical change
	DDRD   &= ~(1<<PD2);
	MCUCR  |= (1<<ISC00);
	GIMSK  |= (1<<INT0);
	#elif defined (__AVR_ATmega328P__)
	// Timer: 8bit, Clock: 16MHZ, Prescaler 1024, Overflow = 16.384ms, Tick = 0.064ms, Normal Mode, Oflow Interrupt Enabled
	TCCR0A = 0;
	TCCR0B |= (1<<CS00) | (1<<CS02);
	TIMSK0 |= (1<<TOIE0);
	
	// Interrupt 0 (PD2): Inverted signal input, triggered by logical change
	DDRD   &= ~(1<<PORTD2);
	EICRA  |= (1<<ISC00);
	EIMSK  |= (1<<INT0);
	#else
	#warning "MCU not supported"
	#endif

	// Reset state
	ir_state = IR_BURST;
	
	// Reset global variables
	ir_tmp_keyhold = 0;
	ir_tmp_ovf = 0;
	
	// Global interrupt enable
	sei();
}


// ###### Stops ir function ######
void ir_stop( void )
{
	// Stop timer and disable interrupt
	#if defined (__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__)
	TCCR0B &= ~((1<<CS00) | (1<<CS02));
	TIMSK0 &= ~(1<<TOIE0);
	EIMSK  &= ~(1<<INT0);
	#elif defined (__AVR_ATtiny2313__) || (__AVR_ATtiny4313__)
	TCCR0B &= ~((1<<CS00) | (1<<CS02));
	TIMSK  &= ~(1<<TOIE0);
	GIMSK  &= ~(1<<INT0);
	#elif defined (__AVR_ATmega328P__)
	TCCR0B &= ~((1<<CS00) | (1<<CS02));
	TIMSK0 &= ~(1<<TOIE0);
	EIMSK  &= ~(1<<INT0);
	#else
	#warning "MCU not supported"
	#endif
}


// ###### INT0 for decoding ######
ISR( INT0_vect )
{
	// Get current port state to check if we triggered on rising or falling edge
	uint8_t port_state = ( PIND & (1<<PD2) );
	uint8_t cnt_state = TCNT0;

	if(ir_tmp_ovf!=0)
	{
		// Overflow, so reset and ignore.
		ir_tmp_ovf = 0;
		ir_state = IR_BURST;
		TCNT0 = 0;
		return;
	}

	switch(ir_state)
	{
		case IR_BURST:
		if(!port_state)
		{
			TCNT0 = 0; // Reset counter
			} else {
			if((cnt_state>TIME_BURST_MIN)&&(cnt_state<TIME_BURST_MAX))
			{
				ir_state = IR_GAP; // Next state
				TCNT0 = 0; // Reset counter
			}
		}
		break;
		case IR_GAP:
		if(!port_state)
		{
			if((cnt_state>TIME_GAP_MIN)&&(cnt_state<TIME_GAP_MAX))
			{
				TCNT0 = 0; // Reset counter
				ir_state = IR_ADDRESS; // Next state
				ir_bitctr = 0; // Reset bitcounter
				ir.status &= ~(1<<IR_KEYHOLD);
				break;
			} else
			if((cnt_state>TIME_HOLD_MIN)&&(cnt_state<TIME_HOLD_MAX))
			{
				if(ir.status & (1<<IR_SIGVALID))
				{
					ir.status |= (1<<IR_KEYHOLD);
					ir_tmp_keyhold = IR_HOLD_OVF;
				}
				ir_state = IR_BURST;
				break;
			}
		}
		// Should not happen, must be invalid. Reset.
		ir_state = IR_BURST;
		break;
		case IR_ADDRESS:
		if(port_state)
		{
			// Must be short pulse
			if((cnt_state>TIME_PULSE_MIN)&&(cnt_state<TIME_PULSE_MAX))
			{
				TCNT0 = 0; // Reset counter
				break;
			}
			// Should not happen, must be invalid. Reset.
			ir_state = IR_BURST;
			} else {
			if((cnt_state>TIME_ZERO_MIN)&&(cnt_state<TIME_ZERO_MAX))
			{
				// 0
				#ifdef PROTOCOL_NEC_EXTENDED
				ir_tmp_address_l &= ~(1<<ir_bitctr++);
				#else
				ir_tmp_address &= ~(1<<ir_bitctr++);
				#endif
				TCNT0 = 0; // Reset counter
				if(ir_bitctr>=8)
				{
					ir_state = IR_ADDRESS_INV; // Next state
					ir_bitctr = 0; // Reset bitcounter
				}
				break;
			} else
			if((cnt_state>TIME_ONE_MIN)&&(cnt_state<TIME_ONE_MAX))
			{
				// 1
				#ifdef PROTOCOL_NEC_EXTENDED
				ir_tmp_address_l |= (1<<ir_bitctr++);
				#else
				ir_tmp_address |= (1<<ir_bitctr++);
				#endif
				TCNT0 = 0; // Reset counter
				if(ir_bitctr>=8)
				{
					ir_state = IR_ADDRESS_INV; // Next state
					ir_bitctr = 0; // Reset bitcounter
				}
				break;
			}
			// Should not happen, must be invalid. Reset.
			ir_state = IR_BURST;
			break;
		}
		break;
		case IR_ADDRESS_INV:
		if(port_state)
		{
			// Must be short pulse
			if((cnt_state>TIME_PULSE_MIN)&&(cnt_state<TIME_PULSE_MAX))
			{
				TCNT0 = 0; // Reset counter
				break;
			}
			// Should not happen, must be invalid. Reset.
			ir_state = IR_BURST;
			} else {
			if((cnt_state>TIME_ZERO_MIN)&&(cnt_state<TIME_ZERO_MAX))
			{
				// 0 (inverted) or high address
				#ifdef PROTOCOL_NEC_EXTENDED
				ir_tmp_address_h &= ~(1<<ir_bitctr++);
				#else
				if(!(ir_tmp_address&(1<<ir_bitctr++)))
				{
					// Should not happen, must be invalid. Reset.
					ir_state = IR_BURST;
					break;
				}
				#endif
				TCNT0 = 0; // Reset counter
				if(ir_bitctr>=8)
				{
					ir_state = IR_COMMAND; // Next state
					ir_bitctr = 0; // Reset bitcounter
				}
				break;
			} else
			if((cnt_state>TIME_ONE_MIN)&&(cnt_state<TIME_ONE_MAX))
			{
				// 1 (inverted) or high address
				#ifdef PROTOCOL_NEC_EXTENDED
				ir_tmp_address_h |= (1<<ir_bitctr++);
				#else
				if(ir_tmp_address&(1<<ir_bitctr++))
				{
					// Should not happen, must be invalid. Reset.
					ir_state = IR_BURST;
					break;
				}
				#endif
				TCNT0 = 0; // Reset counter
				if(ir_bitctr>=8)
				{
					ir_state = IR_COMMAND; // Next state
					ir_bitctr = 0; // Reset bitcounter
				}
				break;
			}
			// Should not happen, must be invalid. Reset.
			ir_state = IR_BURST;
			break;
		}
		break;
		case IR_COMMAND:
		if(port_state)
		{
			// Must be short pulse
			if((cnt_state>TIME_PULSE_MIN)&&(cnt_state<TIME_PULSE_MAX))
			{
				TCNT0 = 0; // Reset counter
				break;
			}
			// Should not happen, must be invalid. Reset.
			ir_state = IR_BURST;
			} else {
			if((cnt_state>TIME_ZERO_MIN)&&(cnt_state<TIME_ZERO_MAX))
			{
				// 0
				ir_tmp_command &= ~(1<<ir_bitctr++);
				TCNT0 = 0; // Reset counter
				if(ir_bitctr>=8)
				{
					ir_state = IR_COMMAND_INV; // Next state
					ir_bitctr = 0; // Reset bitcounter
				}
				break;
			} else
			if((cnt_state>TIME_ONE_MIN)&&(cnt_state<TIME_ONE_MAX))
			{
				// 1
				ir_tmp_command |= (1<<ir_bitctr++);
				TCNT0 = 0; // Reset counter
				if(ir_bitctr>=8)
				{
					ir_state = IR_COMMAND_INV; // Next state
					ir_bitctr = 0; // Reset bitcounter
				}
				break;
			}
			// Should not happen, must be invalid. Reset.
			ir_state = IR_BURST;
			break;
		}
		break;
		case IR_COMMAND_INV:
		if(port_state)
		{
			// Must be short pulse
			if((cnt_state>TIME_PULSE_MIN)&&(cnt_state<TIME_PULSE_MAX))
			{
				TCNT0 = 0; // Reset counter
				break;
			}
			// Should not happen, must be invalid. Reset.
			ir_state = IR_BURST;
			} else {
			if((cnt_state>TIME_ZERO_MIN)&&(cnt_state<TIME_ZERO_MAX))
			{
				// 0 (inverted)
				if(!(ir_tmp_command&(1<<ir_bitctr++)))
				{
					// Should not happen, must be invalid. Reset.
					ir_state = IR_BURST;
					break;
				}
				TCNT0 = 0; // Reset counter
				if(ir_bitctr>=8)
				{
					ir_state = IR_BURST; // Decoding finished.
					// Only apply if received flag is not set, must be done
					// by the main program after reading address and command
					if(!(ir.status & (1<<IR_RECEIVED)))
					{
						#ifdef PROTOCOL_NEC_EXTENDED
						ir.address_l = ir_tmp_address_l;
						ir.address_h = ir_tmp_address_h;
						#else
						ir.address = ir_tmp_address;
						#endif
						ir.command = ir_tmp_command;
						ir.status |= (1<<IR_RECEIVED) | (1<<IR_SIGVALID);
						ir_tmp_keyhold = IR_HOLD_OVF; // To make shure that valid flag is cleared
					}
					ir_bitctr = 0; // Reset bitcounter
				}
				break;
			} else
			if((cnt_state>TIME_ONE_MIN)&&(cnt_state<TIME_ONE_MAX))
			{
				// 1 (inverted)
				if(ir_tmp_command&(1<<ir_bitctr++))
				{
					// Should not happen, must be invalid. Reset.
					ir_state = IR_BURST;
					break;
				}
				TCNT0 = 0; // Reset counter
				if(ir_bitctr>=8)
				{
					ir_state = IR_BURST; // Decoding finished.
					// Only apply if received flag is not set, must be done
					// by the main program after reading address and command
					if(!(ir.status & (1<<IR_RECEIVED)))
					{
						#ifdef PROTOCOL_NEC_EXTENDED
						ir.address_l = ir_tmp_address_l;
						ir.address_h = ir_tmp_address_h;
						#else
						ir.address = ir_tmp_address;
						#endif
						ir.command = ir_tmp_command;
						ir.status |= (1<<IR_RECEIVED) | (1<<IR_SIGVALID);
						ir_tmp_keyhold = IR_HOLD_OVF; // To make shure that valid flag is cleared
					}
					ir_bitctr = 0; // Reset bitcounter
				}
				break;
			}
			// Should not happen, must be invalid. Reset.
			ir_state = IR_BURST;
			break;
		}
		break;
	}
}


// ###### Timer 0 Overflow for hold flag clear ######
ISR (TIMER0_OVF_vect)
{
	ir_tmp_ovf = 1;
	if(ir_tmp_keyhold>0)
	{
		ir_tmp_keyhold--;
		if(ir_tmp_keyhold==0) ir.status &= ~((1<<IR_KEYHOLD) | (1<<IR_SIGVALID));
	}
}
