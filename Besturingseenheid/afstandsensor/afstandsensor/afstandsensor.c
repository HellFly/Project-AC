/*
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

uint8_t test = 0;



//**********FUNCTIONS TO CONTROL LEDS*****************
void setupLeds(){
	DDRB |= _BV(DDB5); //red led pin 5
	DDRB |= _BV(DDB3); //yellow led pin 3
	DDRB |= _BV(DDB1); //green led pin 1
}

void turnOnRED(){
	 PORTB |= _BV(PORTB5);
}

void turnOffRED(){
	PORTB &= ~_BV(PORTB5);
}

void turnOnYELLOW(){
	PORTB |= _BV(PORTB3);
}

void turnOffYELLOW(){
	PORTB &= ~_BV(PORTB3);
}	

void turnOnGREEN(){
	PORTB |= _BV(PORTB1);
}

void turnOffGREEN(){
	PORTB &= ~_BV(PORTB1);
}

void turnOffAll(){
	turnOffYELLOW();
	turnOffRED();
	turnOffGREEN();
}

//*********UART FUNCTIONS*****************

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
	 //enable receiver
	// UCSR0B = _BV(RXEN0);
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

void transmit_string(int *c) {
	while (*c != -1) {
		transmit(*c);
		c++;
	}
}

// Receives a byte from UART
uint8_t receive(uint8_t response) {
	loop_until_bit_is_set(UCSR0A, RXC0);
	return response;
}



//**************FUNCTIONS FOR DISTANCE SENSOR*************

void setupUltrasonic(){
	clearEcho();
	DDRD |= _BV(DDD2); //Trig as output
	DDRD &= ~_BV(DDD3); //Echo as input
	//transmit(DDRD);
}


void clearTrigger(){
	PORTD &= ~_BV(PORTD2);
}

void setTrigger(){
	PORTD |= _BV(PORTD2);
}

void clearEcho(){
	PORTD &= ~_BV(PORTD3);
}

void setEcho(){
	PORTD |= _BV(PORTD3);
}

uint8_t calcDistance()
{
	int counter = 0;
	clearEcho();
	clearTrigger(); 
	_delay_us(2);
	setTrigger();
	_delay_us(10);
	clearTrigger();
	//transmit(pulse);
	while(PIND3 == 0);

	while(PIND3 > 0){
		counter += 1;
	}

	float temp = (float)counter/(float)58;
	test = (uint8_t)temp;

	transmit(counter);
	return test;

}

//******MAIN********

int main()                     
{
	setupUltrasonic();
	uart_init();
	while(1){
		calcDistance();
		_delay_ms(500);
	}	
	              
	return 0;
}