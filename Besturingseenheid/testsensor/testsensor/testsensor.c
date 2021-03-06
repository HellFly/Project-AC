/*
 * testsensor.c
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
uint16_t averageLight = 0;

//Will be used to indicate the state of a screen
typedef enum {UP=0, SCROLLING=1, DOWN=2} state;

//Used for sending commands to screen
typedef enum {SCROLLDOWN=0, NEUTRAL=1, SCROLLUP=2} command;

static uint8_t MAX_TEMP = 25; // in degrees celcius
static uint8_t MIN_TEMP = 20;
static uint8_t MAX_LIGHT = 95;
static uint8_t MIN_LIGHT = 30;
static uint8_t OPEN_DISTANCE = 10; //in centimeters. Max value = 255. This is value of distance when screen is OPEN
static uint8_t CLOSED_DISTANCE = 110; //Min possible = 5. This is value of distance when screen is CLOSED
static uint8_t SCROLLSPEED = 10; //(MAX_DISTANCE - MIN_DISTANCE)%SCROLLSPEED HAS TO BE 0 !!!

state screen = UP; //screen is up by default
command instruction = NEUTRAL;
uint16_t distance = 0;

//Tasks to be added and deleted regularly by scheduler
unsigned int redon = -1;
unsigned int yellowon = -1;
unsigned int redoff = -1;
unsigned int yellowoff = -1;
unsigned int greenon = -1;
unsigned int greenoff = -1;
unsigned int lowerscreen = -1;
unsigned int upscreen = -1;
unsigned int checkdistance = -1;


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
	 // enable transmitter and receiver
	 UCSR0B = _BV(TXEN0) | _BV(RXEN0);
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

// Returns what's received, if nothing is received, return -1
// This is non-blocking
int receive_non_blocking() {
	if (UCSR0A & (1<<RXC0)) { // is the received data bit set in the UCSR0A register?
	return (int) UDR0;
}
return -1;
}

// Sends the light value via UART
void send_light(int light) {
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
		val1 = (uint8_t)floor(light / 256);
		val2 = (uint8_t)(light % 256);
	}

	int buffer[4];
	buffer[0] = 1;
	buffer[1] = val1;
	buffer[2] = val2;
	buffer[3] = -1;
	transmit_string(buffer);
}

// Sends the temperature via UART
void send_temperature(int temp) {
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

	int buffer[3];
	buffer[0] = 2;
	buffer[1] = val;
	buffer[2] = -1;
	transmit_string(buffer);
}

// Sends whether the blinds are open or closed
// 0 = closed, 1 = moving, 2 = open
void send_blinds_status(uint8_t status) {
	if (status > 2) {
		status = 2;
	}
	int buffer[4];
	buffer[0] = 3;
	buffer[1] = 0;
	buffer[2] = status;
	buffer[3] = -1;
	transmit_string(buffer);
}

// Reset the buffer of incoming messages
int receive_buffer[20];
uint8_t buffer_reset = 0;
void reset_buffer() {

	for(uint8_t i = 0; i < sizeof(receive_buffer); i++) {
		receive_buffer[i] = -1;
	}
}

// Add a byte to the buffer of incoming messages
void add_to_buffer(uint8_t c) {
	uint8_t i = 0;
	while (receive_buffer[i] != -1) {
		i++;
	}
	receive_buffer[i] = c;
}

//Receive messages
// This should be in the scheduler
// TODO edit this to do the stuff it has to do
void receiveMessages() {
	if (buffer_reset != 123) {
		reset_buffer();
		buffer_reset = 123;
	}
	int b = receive_non_blocking();
	if (b != -1) { // If no messages were received at all there's no need to check everything again
		while (b != -1) {
			add_to_buffer((uint8_t) b);
			b = receive_non_blocking();
		}

		int c = receive_buffer[0];
		int p1 = receive_buffer[1];
		int p2 = receive_buffer[2];
		int p3 = receive_buffer[3];

		if (c == 10) { // Open blinds
			if (p1 == 1) {
				// OPEN THE BLINDS
				// Do stuff here
				ScrollUp();
				// End do stuff
				reset_buffer();
			}
			else if (p1 != -1) {
				reset_buffer();
			}
		}
		else if (c == 11) { // Close blinds
			if (p1 == 1) {
				// CLOSE THE BLINDS
				// Do stuff here
				ScrollDown();
				// End do stuff
				reset_buffer();
			}
			else if (p1 != -1) {
				reset_buffer();
			}
		}
		else if (c == 20) { // Set blinds open distance
			if (p1 != -1 && p2 != -1) {
				int blinds_open_distance = p1 * 256 + p2; // The new blinds open distance
				// Do stuff here
				OPEN_DISTANCE = blinds_open_distance;
				if(screen == UP){
					distance = OPEN_DISTANCE;
				}
				// End do stuff
				reset_buffer();
			}
		}
		else if (c == 21) { // Set blinds closed distance
			if (p1 != -1 && p2 != -1) {
				int blinds_closed_distance = p1 * 256 + p2; // The new blinds closed distance
				// Do stuff here
				CLOSED_DISTANCE = blinds_closed_distance;
				if(screen == DOWN){
					distance = CLOSED_DISTANCE;
				}
				// End do stuff
				reset_buffer();
			}
		}
		else if (c == 30) { // Set temperature to close
			if (p1 != -1) {
				int temperature_to_close = p1 - 128; // The new temperature threshold to close the blinds at
				// Do stuff here
				MAX_TEMP = temperature_to_close;
				// End do stuff
				reset_buffer();
			}
		}
		else if (c == 31) { // Set temperature to open
			if (p1 != -1) {
				int temperature_to_open = p1 - 128; // The new temperature threshold to open the blinds at
				// Do stuff here
				MIN_TEMP = temperature_to_open;
				// End do stuff
				reset_buffer();
			}
		}
		else if (c == 32) { // Set light to close
			if (p1 != -1 && p2 != -1) {
				int light_to_close = p1 * 256 + p2; // The new light threshold to close the blinds at
				// Do stuff here
				MAX_LIGHT = light_to_close;
				// End do stuff
				reset_buffer();
			}
		}
		else if (c == 33) { // Set light to open
			if (p1 != -1 && p2 != -1) {
				int light_to_open = p1 * 256 + p2; // The new light threshold to open the blinds at
				// Do stuff here
				MIN_LIGHT = light_to_open;
				// End do stuff
				reset_buffer();
			}
		}
		else if (c != -1) { // Command is not empty and not recognized, so something went wrong, reset buffer
			reset_buffer();
		}
	}
}


//***********FUNCTIONS FOR THE ADC****************

void setChannelZero(){
	ADMUX &= ~(1 << MUX0); //Set channel to 0
}

void setChannelOne(){
	ADMUX |= (1 << MUX0); // set channel to 1
}


//Set up the ADC registers: ADMUX and ADCSRA. We use ADC channel 0.
void setupADC()
{
	//Channel = 0 as of now
	ADMUX |= (1 << REFS0); //set reference voltage
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //set prescaler
	ADCSRA |= (1 << ADEN); //enable the ADC
}

uint16_t adc_read(uint8_t ch)
{
	ch &= 0b00000111;  // AND operation with 7 to keep channel < 7 always
	ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing

	// start the conversion
	ADCSRA |= (1<<ADSC);

	// wait for conversion to complete
	while(ADCSRA & (1<<ADSC));

	return (ADC);
}


//********FUNCTIONS TO CONTROL THE SCREEN*************

//Actually physically lowers the screen
void lowerScreen(){
	distance += SCROLLSPEED;
}

//Actually physically rises the screen
void upScreen(){
	distance -= SCROLLSPEED;
}

//Check to see if we are finished scrolling
void checkDistance(){
	if(distance <= OPEN_DISTANCE && instruction == SCROLLUP && screen == SCROLLING){
		screen = UP;
		instruction = NEUTRAL;
		SCH_Delete_Task(upscreen);
		SCH_Delete_Task(checkdistance);
		SCH_Delete_Task(yellowon);
		SCH_Delete_Task(yellowoff);
		upscreen = -1;
		checkdistance = -1;
		yellowon = -1;
		yellowoff = -1;
		distance = OPEN_DISTANCE;
		screen = UP;
		instruction = NEUTRAL;
		turnOffAll();
		turnOnGREEN();
		send_blinds_status(1);
	} else if(distance >= CLOSED_DISTANCE && instruction == SCROLLDOWN && screen == SCROLLING){
		screen = DOWN;
		instruction = NEUTRAL;
		SCH_Delete_Task(lowerscreen);
		SCH_Delete_Task(checkdistance);
		SCH_Delete_Task(yellowon);
		SCH_Delete_Task(yellowoff);
		lowerscreen = -1;
		checkdistance = -1;
		yellowon = -1;
		yellowoff = -1;
		distance = CLOSED_DISTANCE;
		screen = DOWN;
		instruction = NEUTRAL;
		turnOffAll();
		turnOnRED();
		send_blinds_status(0);
	}
}

//Set instruction to SCROLLDOWN, scroll the screen and light correct leds
void ScrollDown()
{
	if(screen == UP && instruction == NEUTRAL && screen != SCROLLING){ // Only scroll down if it is UP and hasnt received other instruction before
		instruction = SCROLLDOWN;
		screen = SCROLLING;
		turnOffAll();
		if(lowerscreen == -1){
			lowerscreen = SCH_Add_Task(lowerScreen, 5, 100);
		}
		if(checkdistance == -1){
			checkdistance = SCH_Add_Task(checkDistance, 7, 100);
		}
		if(yellowon == -1){
			yellowon = SCH_Add_Task(turnOnYELLOW, 6, 100);
		}
		if(yellowoff == -1){
			yellowoff = SCH_Add_Task(turnOffYELLOW, 57, 100);
		}
		turnOnRED();
		send_blinds_status(2);
	}
}

//Set instruction to SCROLLUP, scroll the screen, and light correct leds
void ScrollUp()
{
	if(screen == DOWN && instruction == NEUTRAL && screen != SCROLLING){ // Only scroll up if it is DOWN and hasnt received other instruction before
		instruction = SCROLLUP;
		screen = SCROLLING;
		turnOffAll();
		if(upscreen == -1){
		upscreen = SCH_Add_Task(upScreen, 5, 100);
		}
		if(checkdistance == -1){	
		checkdistance = SCH_Add_Task(checkDistance, 7, 100);
		}
		if(yellowon == -1){		
		yellowon = SCH_Add_Task(turnOnYELLOW, 6, 100);
		}
		if(yellowoff == -1){		
		yellowoff = SCH_Add_Task(turnOffYELLOW, 57, 100);
		}		
		turnOnGREEN();
		send_blinds_status(2);
	}
}

//Used for debugging. Sends value of distance to UART.
void transmitDistance(){
	send_temperature(distance);
}

//**********FUNCTIONS FOR TEMPSENSOR****************

//This function translates the voltage value from the ADC into a temperature.
void calculateTemperature()
{
	setChannelZero(); //Channel 0 is used to measure temperature
	uint16_t reading = adc_read(0); //get the 10 bit return value from the ADC. (0 - 1023)

	//Formula to calculate the temperature
	float voltage = (float)reading/(float)1024; //ADC return a value between 0 and 1023 which is a ratio to the 5V.
	voltage *= 5; //Multiply by 5V
	voltage -= 0.5; //Deduct the offset ( Offset is 0.5 )
	float temperature = (float)100*voltage;

	averageTemperature += (uint8_t)temperature;
}

//This function is used to calculate the average temperature.
void calculateAverageTemperature()
{
	averageTemperature /= 5; //calculate average from 5 values
	if(averageTemperature >= MAX_TEMP){
		ScrollDown();
	} else if (averageTemperature <= MIN_TEMP){
		ScrollUp();
	}
	send_temperature(averageTemperature);
	averageTemperature = 0;
}

//**********FUNCTIONS FOR LIGHTSENSOR**************
void calculateLight(){
	setChannelOne();
	uint16_t reading = adc_read(1);
	float temp = (reading/4);
	float light = 100 - ((temp/(float)255)*100); //Light is a percentage. 0 = dark. 100 = bright

	averageLight += (uint8_t)light;
}


//This function is used to calculate the average temperature.
void calculateAverageLight()
{
	averageLight /= 5; //calculate average from 5 measured values
	if(averageLight >= MAX_LIGHT){
		ScrollDown();
	} else if (averageLight <= MIN_LIGHT){
		ScrollUp();
	}
	send_light((uint8_t)averageLight);
	averageLight = 0;
}

//Sets starting position of the screen and turns on the corresponding led
void setStartingPosition(){
	if(screen == UP){
		distance = OPEN_DISTANCE;
		turnOnGREEN();
		send_blinds_status(1);
	} else {
		distance = CLOSED_DISTANCE;
		turnOnRED();
		send_blinds_status(0);
	}
}

//******MAIN********

int main()
{
	setupADC();
	setupLeds();
	uart_init();
	SCH_Init_T1();
	setStartingPosition();
	
	/*
	First argument of Add_task is the task you want to execute regularly.
	Second argument is how many ticks it should wait before starting doing it for the first time
	Third argument is how often it should be executed in ticks
	One tick = 10ms - So 200 = 2 secs and 1000 = 10 sec etc...
	*/
	SCH_Add_Task(calculateTemperature, 0, 200); //Read temperature every 2 seconds
	SCH_Add_Task(calculateLight, 100, 200); //Read light every 2 seconds

	SCH_Add_Task(calculateAverageTemperature, 1010, 1000); //Calculate average every 10 seconds. Delay it by 10.01 seconds to prevent incomplete average measurements.
	SCH_Add_Task(calculateAverageLight, 1110, 1000); //Calculate average light every 10 seconds.

	SCH_Add_Task(receiveMessages, 1003, 50); // Receive commands/settings from GUI
	//SCH_Add_Task(transmitDistance, 1004, 100); //enable to transmit height of screen to cmd

	SCH_Start();

	while(1)
	{
		SCH_Dispatch_Tasks();
	}

	return 0;
}
