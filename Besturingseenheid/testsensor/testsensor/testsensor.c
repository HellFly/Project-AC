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

//*************SETTING UP AND DEFINING SOME VARIABLES****************

uint16_t averageTemperature = 0;

//Will be used to indicate the state of a screen
typedef enum {UP=0, SCROLLING=1, DOWN=2} state;
	
//Used for sending commands to screen
typedef enum {SCROLLDOWN=0, NEUTRAL=1, SCROLLUP=2} command;
	
static const uint8_t MAX_TEMP = 25; // in degrees celcius
static const uint8_t MIN_TEMP = 22;
static const uint8_t MAX_DISTANCE = 160; //in centimeters
static const uint8_t MIN_DISTANCE = 10;

state screen = UP; //screen is up by default 
command instruction = NEUTRAL;
uint8_t distance = 160; 

unsigned char redon;
unsigned char yellowon;
unsigned char redoff;
unsigned char yellowoff;
unsigned char greenon;
unsigned char greenoff;
unsigned char lowerscreen;
unsigned char upscreen;


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


//***********FUNCTIONS FOR THE ADC****************

//Set up the ADC registers: ADMUX and ADCSRA. We use ADC channel 0.
void setupADC()
{
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


//********FUNCTIONS TO CONTROL THE SCREEN*************

void lowerScreen(){
	distance -= 10;
}

void upScreen(){
	distance += 10;
}

//Scrolls screen down
void scrollDown()
{
	if(screen == UP){
		instruction = SCROLLDOWN;
		lowerscreen = SCH_Add_Task(lowerScreen, 0, 50);
		screen = SCROLLING;
	}		
}

//Scroll screen up
void scrollUp()
{
	if(screen == DOWN){
		instruction = SCROLLUP;
		upscreen = SCH_Add_Task(upScreen, 0, 50);
		screen = SCROLLING;
	}
}

void transmitDistance(){
	transmit(distance);
}

//**********FUNCTIONS UNIQUE TO THE TEMPSENSOR****************

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
	averageTemperature += temperature;
}

//This function is used to calculate the average temperature.
void calculateAverageTemperature()
{
	averageTemperature /= 10; //calculate average from 6 measured values with intervals of 10 seconds.
	transmit(averageTemperature); //Send average temperature to screen.
}

void resetAverageTemperature(){
	averageTemperature = 0; //reset average temperature.
}

void temperatureCheck(){
	if(averageTemperature >= MAX_TEMP){
		scrollDown();
	} else if (averageTemperature <= MIN_TEMP){
		scrollUp();
	}
}

//Mainly used for controlling the LEDS
void checkCommand(){
	if(instruction == SCROLLDOWN){
		turnOffAll();
		redon = SCH_Add_Task(turnOnRED, 0, 100);
		yellowon = SCH_Add_Task(turnOnYELLOW, 0, 100);
		redoff = SCH_Add_Task(turnOffRED, 50, 100);
		yellowoff = SCH_Add_Task(turnOffYELLOW, 50, 100);
	} else if(instruction == SCROLLUP){
		turnOffAll();
		greenon = SCH_Add_Task(turnOnGREEN, 0, 100);
		yellowon = SCH_Add_Task(turnOnYELLOW, 0, 100);
		greenoff = SCH_Add_Task(turnOffGREEN, 50, 100);
		yellowoff = SCH_Add_Task(turnOffYELLOW, 50, 100);
	} 
}

//Stop scrolling the screen and flashing the LEDs
void checkDistance(){
	
	if(distance == MIN_DISTANCE && instruction == SCROLLDOWN && screen == SCROLLING){
		screen = DOWN;
		instruction = NEUTRAL; 
		turnOffAll();
		SCH_Delete_Task(lowerscreen);
		SCH_Delete_Task(redon);
		SCH_Delete_Task(yellowon);
		SCH_Delete_Task(redoff);
		SCH_Delete_Task(yellowoff);
	} else if(distance == MAX_DISTANCE && instruction == SCROLLUP && screen == SCROLLING){
		screen = UP;
		instruction = NEUTRAL;
		turnOffAll();
		SCH_Delete_Task(upscreen);
		SCH_Delete_Task(greenon);
		SCH_Delete_Task(yellowon);
		SCH_Delete_Task(greenoff);
		SCH_Delete_Task(yellowoff);
	}
	 else if(distance == MAX_DISTANCE){
		turnOnGREEN();
		turnOffYELLOW();
		turnOffRED();
	} else if(distance == MIN_DISTANCE){
		turnOffGREEN();
		turnOffYELLOW();
		turnOnRED();
	} else{
		turnOffAll();
	}
}

void turnOffAll(){
	turnOffYELLOW();
	turnOffRED();
	turnOffGREEN();
}


//******MAIN********

int main()                     
{
	setupADC(); 
	setupLeds();
	uart_init();
	SCH_Init_T1();
	unsigned char calctemp = SCH_Add_Task(calculateTemperature, 0, 100); //Read temperature every second
	unsigned char calcaveragetemp = SCH_Add_Task(calculateAverageTemperature, 1000, 1000); //Calculate average every 10 seconds. Delay it by 10 seconds to prevent incomplete average measurements.
	unsigned char tempcheck = SCH_Add_Task(temperatureCheck, 1000, 1000); //What should the screen do?
	SCH_Add_Task(transmitDistance, 500, 100);
	unsigned char checkcomm = SCH_Add_Task(checkCommand, 1000, 50); //Which Leds have to be flashing?
	unsigned char resetaverage = SCH_Add_Task(resetAverageTemperature, 1000, 1000); //reset average temperature
	unsigned char checkdistance = SCH_Add_Task(checkDistance, 1000, 50); //Screen is done scrolling? Turn off/on the correct leds . 

	
	SCH_Start();
	while(1)
	{
		SCH_Dispatch_Tasks();
	}
	               
	return 0;
}