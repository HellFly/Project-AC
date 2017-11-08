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

uint8_t averageTemperature = 0;
uint8_t averageLight = 0;

//Will be used to indicate the state of a screen
typedef enum {UP=0, SCROLLING=1, DOWN=2} state;
	
//Used for sending commands to screen
typedef enum {SCROLLDOWN=0, NEUTRAL=1, SCROLLUP=2} command;
	
static uint8_t MAX_TEMP = 22; // in degrees celcius
static uint8_t MIN_TEMP = 20;
static uint8_t MAX_LIGHT = 250;
static uint8_t MIN_LIGHT = 100;
static uint8_t MAX_DISTANCE = 160; //in centimeters. Max value = 255. This is value of distance when screen is UP
static uint8_t MIN_DISTANCE = 10; //Min possible = 5. This is value of distance when screen is DOWN
static uint8_t SCROLLSPEED = 1; //(MAX_DISTANCE - MIN_DISTANCE)%SCROLLSPEED HAS TO BE 0 !!!
static const uint8_t DEFAULT_MAX_DISTANCE = 160;
static const uint8_t DEFAULT_MIN_DISTANCE = 10;
static const uint8_t DEFAULT_SCROLLSPEED = 5;

state screen = DOWN; //screen is up by default 
command instruction = NEUTRAL;
uint8_t distance;

//Tasks to be added and deleted regularly by scheduler
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

//Actually lowers the screen
void lowerScreen(){
	distance -= SCROLLSPEED;
}

//Actually rises the screen
void upScreen(){
	distance += SCROLLSPEED;
}

//Scroll screen down if it is UP. Set instruction to SCROLLDOWN to tell CheckCommand() and CheckDistance() what they should do.
void scrollDown()
{
	if(screen == UP){
		instruction = SCROLLDOWN;
	}		
}

//Scroll screen up if it is DOWN. Set instruction to SCROLLUP to tell CheckCommand() and CheckDistance() what they should do.
void scrollUp()
{
	if(screen == DOWN){
		instruction = SCROLLUP;
	}
}

//Used for debugging. Sends value of distance to UART.
void transmitDistance(){
	transmit(distance);
}

//**********FUNCTIONS UNIQUE TO THE TEMPSENSOR****************

//This function translates the voltage value from the ADC into a temperature.
void calculateTemperature()
{
	uint16_t reading = adc_read(0); //get the 10 bit return value from the ADC. (0 - 1023)
	//uint8_t temp = (uint8_t)reading; //force cast it to an 8 bit integer. 16 bit doesnt work with uart for some reason.
	
	uint8_t high_byte = (reading >> 8);
	uint8_t low_byte = reading & 0x00FF;
	uint16_t number = (high_byte << 8) + low_byte;
	
	//Formula to calculate the temperature 
	float voltage = (float)number/(float)1024; //ADC return a value between 0 and 1023 which is a ratio to the 5V. 
	voltage *= 5; //Multiply by 5V
	voltage -= 0.5; //Deduct the offset ( Offset is 0.5 )
	float temperature = (float)100*voltage;
	
	//transmit(number); //enable to transmit to screen
	averageTemperature += (uint8_t)temperature;
}

//This function is used to calculate the average temperature.
void calculateAverageTemperature()
{
	averageTemperature /= 10; //calculate average from 6 measured values with intervals of 10 seconds.
	transmit(averageTemperature); //Send average temperature to screen.
}

//reset average temperature back to 0 so next measurement can begin
void resetAverageTemperature(){
	averageTemperature = 0; //reset average temperature.
}


//**********FUNCTIONS FOR LIGHTSENSOR**************
void calculateLight(){
	uint16_t reading = adc_read(1); //get the 10 bit return value from the ADC. (0 - 1023)
	uint8_t high_byte = (reading >> 8);
	uint8_t low_byte = reading & 0x00FF;
	uint16_t number = (high_byte << 8) + low_byte;
	
		
	//transmit(light);
		
	averageLight += (uint8_t)number;
} 

void resetAverageLight(){
	averageLight = 0; //reset average temperature.
}

//This function is used to calculate the average temperature.
void calculateAverageLight()
{
	averageLight /= 10; //calculate average from 3 measured values with intervals of 10 seconds.
	transmit(averageLight); //Send average to screen.
	averageLight = 0; //reset average light.
}


//***********FUNCTIONS TO CHECK VALUES**************
//Adjusts the screen based on the measured temperature value. Either scroll up or down
void temperatureCheck(){
	if(averageTemperature >= MAX_TEMP){
		scrollDown();
	} else if (averageTemperature <= MIN_TEMP){
		scrollUp();
	}
}

//This function uses the instruction from the ScrollDown/Up functions to flash the leds and scroll the screen.
void checkCommand(){
	if(instruction == SCROLLDOWN && screen != SCROLLING){
		screen = SCROLLING;
		turnOffAll();
		lowerscreen = SCH_Add_Task(lowerScreen, 0, 50);
		yellowon = SCH_Add_Task(turnOnYELLOW, 0, 100);
		yellowoff = SCH_Add_Task(turnOffYELLOW, 50, 100);
		turnOnRED();
	} else if(instruction == SCROLLUP && screen != SCROLLING){
		screen = SCROLLING;
		turnOffAll();
		upscreen = SCH_Add_Task(upScreen, 0, 50);
		yellowon = SCH_Add_Task(turnOnYELLOW, 0, 100);
		yellowoff = SCH_Add_Task(turnOffYELLOW, 50, 100);
		turnOnGREEN();
	}
	
	if(distance == MIN_DISTANCE && instruction == SCROLLDOWN && screen == SCROLLING){
		screen = DOWN;
		instruction = NEUTRAL;
		turnOffAll();
		SCH_Delete_Task(lowerscreen);
		SCH_Delete_Task(yellowon);
		SCH_Delete_Task(yellowoff);
		turnOnRED();
	} else if(distance == MAX_DISTANCE && instruction == SCROLLUP && screen == SCROLLING){
		screen = UP;
		instruction = NEUTRAL;
		turnOffAll();
		SCH_Delete_Task(upscreen);
		SCH_Delete_Task(yellowon);
		SCH_Delete_Task(yellowoff);
		turnOnGREEN();
	}
}

//Sets starting position of the screen and turns on the corresponding led
void setStartingPosition(){
	if(screen == UP){
		distance = MAX_DISTANCE;
		turnOnGREEN();
	} else {
		distance = MIN_DISTANCE;
		turnOnRED();
	}
}

//Makes sure all settings are valid and will not mess with the program. If settings are invalid, use the default settings.
void scrollSpeedCheck(){
	if((MAX_DISTANCE - MIN_DISTANCE)%SCROLLSPEED != 0 || MAX_DISTANCE > 255 || MIN_DISTANCE < 5 || MIN_DISTANCE >= MAX_DISTANCE){
		MAX_DISTANCE = DEFAULT_MAX_DISTANCE;
		MIN_DISTANCE = DEFAULT_MIN_DISTANCE;
		SCROLLSPEED = DEFAULT_SCROLLSPEED;
		//TODO: TRANSMIT SOME ERROR MESSAGE 
	}
}

//******MAIN********

int main()                     
{
	setupADC(); 
	setupLeds();
	uart_init();
	SCH_Init_T1();
	unsigned char scrollspeedcheck = SCH_Add_Task(scrollSpeedCheck, 0, 1); //Make sure settings are valid and correct at all times
	unsigned char startpos = SCH_Add_Task(setStartingPosition, 5, 0); //Set starting pos of screen and light starting led
	unsigned char calctemp = SCH_Add_Task(calculateTemperature, 0, 100); //Read temperature every second
	//unsigned char calclight = SCH_Add_Task(calculateLight, 0, 100); //Read light every second
	unsigned char calcaveragetemp = SCH_Add_Task(calculateAverageTemperature, 1000, 1000); //Calculate average every 10 seconds. Delay it by 10 seconds to prevent incomplete average measurements.
	//unsigned char calcaveragelight = SCH_Add_Task(calculateAverageLight, 1000, 1000); //Calculate average light every 10 seconds.
	unsigned char tempcheck = SCH_Add_Task(temperatureCheck, 1000, 1000); //What instruction should we send to screen?
	unsigned char resetaveragetemp = SCH_Add_Task(resetAverageTemperature, 1000, 1000); //reset average temperature
	//unsigned char resetaveragelight = SCH_Add_Task(resetAverageLight, 1000, 1000);
	//unsigned char transmitdistance = SCH_Add_Task(transmitDistance, 1000, 50); //Used for debugging
	unsigned char checkcomm = SCH_Add_Task(checkCommand, 1000, 10); //What leds should be flashing and what should the screen do?

	
	SCH_Start();
	while(1)
	{
		SCH_Dispatch_Tasks();
	}
	               
	return 0;
}