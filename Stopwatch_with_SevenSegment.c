/*
 * proj2.c
 *
 *  Created on: Sep 16, 2024
 *      Author: Sameh Fawzy
 */


#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
unsigned char paused=0;
unsigned char mode=1; //1 for count up 0 for count down
unsigned char sec_units=0;
unsigned char sec_tens=0;
unsigned char min_units=0;
unsigned char min_tens=0;
unsigned char hr_units=0;
unsigned char hr_tens=0;

void countup(void) {
	// Increment seconds
	sec_units++;
	if (sec_units > 9) {
		sec_units = 0;
		sec_tens++;
	}
	if (sec_tens > 5) {
		sec_tens = 0;
		min_units++;
	}
	// Increment minutes
	if (min_units > 9) {
		min_units = 0;
		min_tens++;
	}
	if (min_tens > 5) {
		min_tens = 0;
		hr_units++;
	}
	// Increment hours
	if (hr_units > 9) {
		hr_units = 0;
		hr_tens++;
	}
	if (hr_tens >= 2 && hr_units >= 4) { // 24-hour format
		hr_units = 0;
		hr_tens = 0;
	}
}

void countdown(void) {
	// Check if the time has already reached 00:00:00
	if (hr_tens == 0 && hr_units == 0 && min_tens == 0 && min_units == 0 && sec_tens == 0 && sec_units == 0) {
		// Stop when time reaches 00:00:00 and activate buzzer
		PORTD |= (1 << 0);  // ACTIVATE BUZZER
		paused = 1;         // Pause the countdown
		return;             // Stop decrementing
	}

	// Decrement seconds
	if (sec_units == 0) {
		if (sec_tens == 0) {

			if (min_units == 0) {
				if (min_tens == 0) {

					if (hr_units == 0) {
						if (hr_tens == 0) {
							// When the time reaches 00:00:00, buzzer is activated (handled above)
							// This part should no longer be reached after the check above.
							return;
						} else if (hr_tens == 2 && hr_units == 0) {

							hr_tens = 1;
							hr_units = 9;
						} else if (hr_tens == 1 && hr_units == 0) {

							hr_tens = 0;
							hr_units = 9;
						} else if (hr_tens == 2 && hr_units > 0) {
							hr_units--;
						} else if (hr_tens < 2) {
							hr_units--;
						}
					} else {
						hr_units--;
					}
					min_tens = 5;
					min_units = 9;
				} else {
					min_tens--;
					min_units = 9;
				}
			} else {
				min_units--;
			}
			sec_tens = 5;
			sec_units = 9;
		} else {
			sec_tens--;
			sec_units = 9;
		}
	} else {
		sec_units--;
	}
}



void interupts_init(){
	SREG|=(1<<7); //I-BIT
	MCUCR |=  (1<<ISC01);   // Trigger INT0 with the falling edge
	MCUCR |= (1<<ISC10) | (1<<ISC11); // Trigger INT1 with the rising edge
	MCUCSR &= ~(1<<ISC2);     // Trigger INT2 with the falling edge
	GICR  |= (1<<INT0) | (1<<INT1) | (1<<INT2);     // Enable external interrupt pin INT0 ,INT1 and INT2
}


void timer1_init(void){
	// Set CTC mode
	TCCR1B |= (1 << WGM12);
	// Set prescaler to 256
	TCCR1B |= (1 << CS12);
	// Set OCR1A for 1 second interrupt
	OCR1A = 62499;
	// Enable compare match interrupt
	TIMSK |= (1 << OCIE1A);
	// Initialize timer counter to 0
	TCNT1 = 0;
	// Enable global interrupts
	sei();
}
ISR(TIMER1_COMPA_vect){
	if(!paused){
		if(mode){
			countup();
			display();
		}
		else if(!mode){
			countdown();
			display();
		}
	}
}
ISR(INT0_vect) {

	_delay_ms(50);        // Debounce
	reset();              // Reset the stopwatch

}
ISR(INT1_vect) {
	_delay_ms(50);
	pause();
}
ISR(INT2_vect) {
	_delay_ms(50);
	resume();
}



void display(void) {
	PORTA = 0x20;
	PORTC = sec_units;
	_delay_ms(2);
	PORTA = 0x00;
	PORTA = 0x10;
	PORTC = sec_tens;
	_delay_ms(2);
	PORTA = 0x00;
	PORTA = 0x08;
	PORTC = min_units;
	_delay_ms(2);
	PORTA = 0x00;
	PORTA = 0x04;
	PORTC = min_tens;
	_delay_ms(2);
	PORTA = 0x00;
	PORTA = 0x02;
	PORTC = hr_units;
	_delay_ms(2);
	PORTA = 0x00;
	PORTA = 0x01;
	PORTC = hr_tens;
	_delay_ms(2);
	PORTA = 0x00;
}

void reset(void){
	sec_units=0;
	sec_tens=0;
	min_units=0;
	min_tens=0;
	hr_units=0;
	hr_tens=0;
	PORTD&=~(1<<0);
	display();
}
void pause(void){
	paused=1;
}
void resume(void){
	paused=0;
}
// Function to decrement hours (24-hour format)
void hrs_decrement(void) {
	if (hr_units == 0) {
		if (hr_tens == 0) {
			// If hours are at 00:00, loop back to 23:00
			hr_tens = 2;
			hr_units = 3;
		} else if (hr_tens == 2 && hr_units == 0) {
			// Special case: From 20:00, decrement to 19:59
			hr_tens = 1;
			hr_units = 9;
		} else {
			hr_tens--;
			hr_units = 9;
		}
	} else {
		hr_units--;
	}
}

void hrs_increment(void) {
    int hours = hr_tens * 10 + hr_units;

    hours = (hours + 1) % 24;

    hr_tens = hours / 10;
    hr_units = hours % 10;
}


// Function to decrement minutes
void min_decrement(void) {
	if (min_units == 0) {
		if (min_tens == 0) {
			// No decrement if minutes are at 00, hours need to decrement
			hrs_decrement();
			min_tens = 5;
			min_units = 9;
		} else {
			min_tens--;
			min_units = 9;
		}
	} else {
		min_units--;
	}
}

// Function to increment minutes
void min_increment(void) {
	if (min_units == 9) {
		if (min_tens == 5) {
			// If minutes are at 59, reset to 00 and increment hours
			min_tens = 0;
			min_units = 0;
			hrs_increment();
		} else {
			min_tens++;
			min_units = 0;
		}
	} else {
		min_units++;
	}
}

// Function to decrement seconds
void sec_decrement(void) {
	if (sec_units == 0) {
		if (sec_tens == 0) {
			// No decrement if seconds are at 00, decrement minutes
			min_decrement();
			sec_tens = 5;
			sec_units = 9;
		} else {
			sec_tens--;
			sec_units = 9;
		}
	} else {
		sec_units--;
	}
}

// Function to increment seconds
void sec_increment(void) {
	if (sec_units == 9) {
		if (sec_tens == 5) {
			// If seconds are at 59, reset to 00 and increment minutes
			sec_tens = 0;
			sec_units = 0;
			min_increment();
		} else {
			sec_tens++;
			sec_units = 0;
		}
	} else {
		sec_units++;
	}
}




int main(void) {

	DDRC = 0x0F; // bcd decoder at portc init
	DDRA = 0x3F; //enable all 6digits as output
	PORTA = 0xFF;  // enable all 6 digits
	DDRD &= ~(1<<PD2) & ~(1<<PD3); /*Configure pin 2 and pin 3 at port D as input pins */
	DDRD |= (1<<PD0) | (1<<PD4) | (1<<PD5);
	PORTD |= (1<<PD2) | (1<<PD4) ; /*Enable internal resistance at pin 2 and set pin 4 at port D by value 1 led ON*/
	PORTD &= ~(1<<PD0) & ~(1<<PD5)  ;
	DDRB=0;
	PORTB=0XFF;
paused=0;
mode=1;
	timer1_init();
	interupts_init();

	while (1) {
		display();
		if (!(PINB & (1 << 7))) {
			_delay_ms(50); //debouncing
			if (!(PINB & (1 << 7))) {
				mode = mode ^ 1; // Toggle mode when the push button is pressed
				_delay_ms(300);
				if(mode){
					PORTD&=~(1<<0); //close the buzzer if its active and its count up mode
				}
				if(!mode){
					pause();
					reset();
				}
			}
		}
		if(mode){ //red light for count up mode
			PORTD&=~(1<<5);
			PORTD|=(1<<4);
		}
		else if(!mode){ //yellow light for count down mode
			PORTD&=~(1<<4);
			PORTD|=(1<<5);
		}
		if(!(PINB&(1<<0))){
			_delay_ms(150);
			if(!(PINB&(1<<0))){
				hrs_decrement();
			}
		}
		if(!(PINB&(1<<1))){
			_delay_ms(150);
			if(!(PINB&(1<<1))){
				hrs_increment();
			}
		}
		if(!(PINB&(1<<3))){
			_delay_ms(150);
			if(!(PINB&(1<<3))){
				min_decrement();
			}
		}
		if(!(PINB&(1<<4))){
			_delay_ms(150);
			if(!(PINB&(1<<4))){
				min_increment();
			}
		}
		if(!(PINB&(1<<5))){
			_delay_ms(150);
			if(!(PINB&(1<<5))){
				sec_decrement();
			}
		}
		if(!(PINB&(1<<6))){
			_delay_ms(150);
			if(!(PINB&(1<<6))){
				sec_increment();
			}
		}


	}







}







