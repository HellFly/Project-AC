/*
 * lichtsensor.c
 *
 * Created: 28-10-2017 1:30:05
 *  Author: Roy
 */ 


#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "AVR_TTC_scheduler.h"
#include <math.h>
#include <avr/sfr_defs.h>
#define F_CPU 16E6
#include <util/delay.h>
#define UBBRVAL 51

//Global variable for the average temperature.
uint16_t averageLight = 0;

//Will be used to indicate the state of a screen
typedef enum {DOWN=0, SCROLLING=1, UP=2} state;

//Initialize UART.
void uart_init()
{
	 // set the baud rate
	 UBRR0H = 0;
	 UBRR0L = UBBRVAL;
	 // disable U2X mode
	 UCSR0A = 0;
	 // enable transmitter
	 UCSR0B = _BV(TXEN0);
	 // set frame format : asynchronous, 8 data bits, 1 stop bit, no parity
	 UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}

//Transmit to UART
void transmit(uint16_t data)
{
	 // wait for an empty transmit buffer
	 // UDRE is set when the transmit buffer is empty
	 
	 loop_until_bit_is_set(UCSR0A, UDRE0);   
	 UDR0 = data;
}

//Set up the ADC registers: ADMUX and ADCSRA. We use ADC channel 1.
void setup()
{
	ADMUX |= (1 << MUX2); //Set ADC channel 1
	//ADMUX |= (1 << ADLAR);
	ADMUX |= (1 << REFS0); //set reference voltage 
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //set prescaler
	ADCSRA |= (1 << ADEN); //enable the ADC
}

uint16_t adc_read(uint8_t ch)
{
	// select the corresponding channel 0~7
	// ANDing with ’7? will always keep the value
	// of ‘ch’ between 0 and 7
	ch &= 0b00000111;  // AND operation with 7
	ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing
	
	// start the conversion
	ADCSRA |= (1<<ADSC);
	
	// wait for conversion to complete
	// ADSC becomes ’0? again
	// till then, run loop continuously
	while(ADCSRA & (1<<ADSC));
	
	return (ADC); 
}

//This function translates the voltage value from the ADC into a light value.
void calculateLight()
{
	uint16_t reading = adc_read(0); //get the 10 bit return value from the ADC.
	uint8_t light = (uint8_t)reading;
	
	//transmit(light);
	
	averageLight += light;
}

//This function is used to calculate the average temperature.
void calculateAverageLight()
{
	averageLight /= 3; //calculate average from 3 measured values with intervals of 10 seconds.
	transmit(averageLight); //Send average to screen.
	averageLight = 0; //reset average light.
}

int main()                     
{
	setup(); 
	uart_init();
	SCH_Init_T1();
	SCH_Add_Task(calculateLight, 0, 1000); //Read lightintensity every 10 seconds
	SCH_Add_Task(calculateAverageLight, 1000, 3000); //Calculate average every 30 seconds. Delay it by 10 seconds to prevent incomplete average measurements.
	SCH_Start();
	while(1)
	{
		SCH_Dispatch_Tasks();
	}
	               
	return 0;
}