﻿/*
 * tempsensor.c
 *
 * Created: 24-10-2017 1:30:05
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
uint16_t averageTemperature = 0;

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
void transmit(uint8_t data)
{
	 // wait for an empty transmit buffer
	 // UDRE is set when the transmit buffer is empty
	 
	 loop_until_bit_is_set(UCSR0A, UDRE0);   
	 UDR0 = data;
}

// Sends a string of chars (bytes) over UART
void transmit_string(uint8_t *c) {
	while (*c) {
		transmit(*c);
		c++;
	}
}

// Sends the light value via UART
void send_light(uint8_t light) {
	uint8_t val1;
	uint8_t val2;
	
	if (light < 0) {
		val1 = 0;
		val2 = 0;
	}
	else if (light > 32767) {
		// if light value > max value able to send
		val1 = 127;
		val2 = 255;
	}
	else {
		val1 = (uint8_t)(light / 256);
		val2 = (uint8_t)(light % 256);
	}

	uint8_t buffer[3];
	buffer[0] = 1;
	buffer[1] = val1;
	buffer[2] = val2;
	transmit_string(buffer);
}

// Sends the temperature via UART
void send_temperature(uint8_t temp) {
	temp += 128;
	uint8_t val;
	
	if (temp < 0) {
		val = 0;
	}
	else if (temp > 255) {
		val = 255;
	}
	else {
		val = (uint8_t)temp;
	}
	
	uint8_t buffer[2];
	buffer[0] = 2;
	buffer[1] = val;
	transmit_string(buffer);
}

// Sends whether the shutter of the light unit is open or closed
// 1 = open, 0 = closed
void send_shutter_status_light(uint8_t is_open) {
	if (is_open > 1) {
		is_open = 1;
	}
	uint8_t buffer[3];
	buffer[0] = 3;
	buffer[1] = 0;
	buffer[2] = is_open;
	transmit_string(buffer);
}

// Sends whether the shutter of the temperature unit is open or closed
// 1 = open, 0 = closed
void send_shutter_status_temp(uint8_t is_open) {
	if (is_open > 1) {
		is_open = 1;
	}
	uint8_t buffer[3];
	buffer[0] = 3;
	buffer[1] = 1;
	buffer[2] = is_open;
	transmit_string(buffer);
}

//Set up the ADC registers: ADMUX and ADCSRA. We use ADC channel 0.
void setup()
{
	ADMUX |= (1 << REFS0); //set reference voltage 
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //set prescaler
	ADCSRA |= (1 << ADEN); //enable the ADC
}

uint16_t adc_read(uint8_t ch)
{
	// select the corresponding channel 0~7
	// ANDing with ’7′ will always keep the value
	// of ‘ch’ between 0 and 7
	ch &= 0b00000111;  // AND operation with 7
	ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing
	
	// start the conversion
	ADCSRA |= (1<<ADSC);
	
	// wait for conversion to complete
	// ADSC becomes ’0′ again
	// till then, run loop continuously
	while(ADCSRA & (1<<ADSC));
	
	return (ADC); 
}

//This function translates the voltage value from the ADC into a temperature.
void calculateTemperature()
{
	uint16_t reading = adc_read(0); //get the 10 bit return value from the ADC.
	uint8_t temp = (uint8_t)reading; //force cast it to an 8 bit integer
	
	//Formula to calculate the temperature 
	float voltage = (float)temp/(float)1024; //ADC return a value between 0 and 1023 which is a ratio to the 5V. 
	voltage *= 5;
	voltage -= 0.5;
	float temperature = (float)100*voltage;
	
	//transmit(temperature); //enable to transmit to screen
	//send_temperature(temperature); //enable to transmit to screen
	averageTemperature += temperature;
}

//This function is used to calculate the average temperature.
void calculateAverageTemperature()
{
	averageTemperature /= 6; //calculate average from 6 measured values with intervals of 10 seconds.
	//transmit(averageTemperature); //Send average temperature to screen.
	send_temperature(averageTemperature);
	averageTemperature = 0; //reset average temperature.
}

int main()                     
{
	setup(); 
	uart_init();
	SCH_Init_T1();
	SCH_Add_Task(calculateTemperature, 0, 1000); //Read temperature every 10 seconds
	SCH_Add_Task(calculateAverageTemperature, 1000, 6000); //Calculate average every minute. Delay it by 10 seconds to prevent incomplete average measurements.
	SCH_Start();
	while(1)
	{
		SCH_Dispatch_Tasks();
	}
	               
	return 0;
}