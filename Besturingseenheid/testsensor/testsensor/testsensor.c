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
uint16_t averageLight = 0;

//Will be used to indicate the state of a screen
typedef enum {UP=0, SCROLLING=1, DOWN=2} state;

//Used for sending commands to screen
typedef enum {SCROLLDOWN=0, NEUTRAL=1, SCROLLUP=2} command;

static uint8_t MAX_TEMP = 22; // in degrees celcius
static uint8_t MIN_TEMP = 20;
static uint8_t MAX_LIGHT = 80;
static uint8_t MIN_LIGHT = 20;
static uint8_t MAX_DISTANCE = 160; //in centimeters. Max value = 255. This is value of distance when screen is UP
static uint8_t MIN_DISTANCE = 10; //Min possible = 5. This is value of distance when screen is DOWN
static uint8_t SCROLLSPEED = 5; //(MAX_DISTANCE - MIN_DISTANCE)%SCROLLSPEED HAS TO BE 0 !!!
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
	if (UCSR0A & 1<RXC0) { // is the received data bit set in the UCSR0A register?
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

// Sends whether the shutter is open or closed
// 1 = open, 0 = closed
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
    int b = receive_non_blocking();
    while (b != -1) {
        add_to_buffer((uint8_t) b);
        b = receive_non_blocking();
    }

    int c = receive_buffer[0];
    int p1 = receive_buffer[1];
    int p2 = receive_buffer[2];
    int p3 = receive_buffer[3];

    switch (c) {
        case -1:
        break;
        case 10:
        if (p1 != -1) {
            if (p1 == 1) {
                // OPEN THE BLINDS
                instruction = SCROLLUP;
<<<<<<< HEAD

=======
               
>>>>>>> d86e3b6e1040ac051d3d45a21dd2d00cd12c5735
                // End do stuff
                reset_buffer();
            }
            else {
                reset_buffer();
            }
        }
        break;
        case 11:
        if (p1 != -1) {
            if (p1 == 1) {
                // CLOSE THE BLINDS
                instruction = SCROLLDOWN;
<<<<<<< HEAD

=======
               
>>>>>>> d86e3b6e1040ac051d3d45a21dd2d00cd12c5735
                // End do stuff
                reset_buffer();
            }
            else {
                reset_buffer();
            }
        }
        break;
        case 20:
        if (p1 != -1 && p2 != -1 && p3 != -1) {
            if (p1 == 1) {
                int blinds_open_distance = p2 * 256 + p3; // The new blinds open distance
                // Do stuff here
                MAX_DISTANCE = blinds_open_distance;
                // End do stuff
                reset_buffer();
            }
            else {
                reset_buffer();
            }
        }
        break;
        case 21:
        if (p1 != -1 && p2 != -1 && p3 != -1) {
            if (p1 == 1) {
                int blinds_closed_distance = p2 * 256 + p3; // The new blinds closed distance
                // Do stuff here
                MIN_DISTANCE = blinds_closed_distance;
                // End do stuff
                reset_buffer();
            }
            else {
                reset_buffer();
            }
        }
        break;
        case 30:
        if (p1 != -1) {
            int temperature_to_close = p1 + 128; // The new temperature threshold to close the blinds at
            // Do stuff here
            MAX_TEMP = temperature_to_close;
            // End do stuff
            reset_buffer();
        }
        break;
        case 31:
        if (p1 != -1) {
            int temperature_to_open = p1 + 128; // The new temperature threshold to open the blinds at
            // Do stuff here
           MIN_TEMP = temperature_to_open;
            // End do stuff
            reset_buffer();
        }
        break;
        case 32:
        if (p1 != -1 && p2 != -1) {
            int light_to_close = p1 * 256 + p2; // The new light threshold to close the blinds at
            // Do stuff here
           MAX_LIGHT = light_to_close;
            // End do stuff
            reset_buffer();
        }
        break;
        case 33:
        if (p1 != -1 && p2 != -1) {
            int light_to_open = p1 * 256 + p2; // The new light threshold to open the blinds at
            // Do stuff here
           MIN_LIGHT = light_to_open;
            // End do stuff
            reset_buffer();
        }
        break;
        default:
            reset_buffer();
        break;
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
	//ADMUX |= (1 << MUX0); // set channel to 1
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

	//transmit(number); //enable to transmit to screen
	averageTemperature += (uint8_t)temperature;
}

//This function is used to calculate the average temperature.
void calculateAverageTemperature()
{
	averageTemperature /= 5; //calculate average from 6 measured values with intervals of 10 seconds.
	//transmit(averageTemperature); //Send average temperature to screen.
	send_temperature(averageTemperature);
}

//reset average temperature back to 0 so next measurement can begin
void resetAverageTemperature(){
	averageTemperature = 0; //reset average temperature.
}


//**********FUNCTIONS FOR LIGHTSENSOR**************
void calculateLight(){
	setChannelOne();
	uint16_t reading = adc_read(1);
	float temp = (reading/4);
	//uint8_t high_byte = (reading >> 8);
	//uint8_t low_byte = reading & 0x00FF;
	//uint16_t number = (high_byte << 8) + low_byte;
<<<<<<< HEAD

=======
	
>>>>>>> d86e3b6e1040ac051d3d45a21dd2d00cd12c5735
	float light = 100 - ((temp/(float)255)*100); //Light is a percentage. 0 = dark. 100 = bright
	//transmit(light);

	averageLight += (uint8_t)light;
}

void resetAverageLight(){
	averageLight = 0; //reset average temperature.
}

//This function is used to calculate the average temperature.
void calculateAverageLight()
{
	averageLight /= 5; //calculate average from 10 measured values
	//transmit(averageLight); //Send average to screen.
	send_light((uint8_t)averageLight);
}


//***********FUNCTIONS TO CHECK VALUES**************
//Adjusts the screen based on the measured temperature value. Either scroll up or down if possible
void temperatureCheck(){
	if(averageTemperature >= MAX_TEMP){
		scrollDown();
	} else if (averageTemperature <= MIN_TEMP){
		scrollUp();
	}
}

//Adjusts the screen based on the measured light value. Either scroll up or down if possible
void lightCheck(){
	if(averageLight >= MAX_LIGHT){
		scrollDown();
	} else if (averageLight <= MIN_LIGHT){
		scrollUp();
	}
}

//This function uses the instruction from the ScrollDown/Up functions to flash the leds and scroll the screen.
void checkCommand(){
	if(instruction == SCROLLDOWN && screen != SCROLLING){
		screen = SCROLLING;
		send_blinds_status(2);
		turnOffAll();
		lowerscreen = SCH_Add_Task(lowerScreen, 0, 50);
		yellowon = SCH_Add_Task(turnOnYELLOW, 0, 100);
		yellowoff = SCH_Add_Task(turnOffYELLOW, 50, 100);
		turnOnRED();
	} else if(instruction == SCROLLUP && screen != SCROLLING){
		screen = SCROLLING;
		send_blinds_status(2);
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
		send_blinds_status(0);
	} else if(distance == MAX_DISTANCE && instruction == SCROLLUP && screen == SCROLLING){
		screen = UP;
		instruction = NEUTRAL;
		turnOffAll();
		SCH_Delete_Task(upscreen);
		SCH_Delete_Task(yellowon);
		SCH_Delete_Task(yellowoff);
		turnOnGREEN();
		send_blinds_status(1);
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
	SCH_Add_Task(scrollSpeedCheck, 0, 1); //Make sure settings are valid and correct at all times
	SCH_Add_Task(setStartingPosition, 500, 0); //Set starting pos of screen and light starting led
	SCH_Add_Task(receiveMessages, 0, 1);
	SCH_Add_Task(calculateTemperature, 0, 200); //Read temperature every second
	SCH_Add_Task(calculateLight, 100, 200); //Read light every second
	SCH_Add_Task(calculateAverageTemperature, 1000, 1000); //Calculate average every 10 seconds. Delay it by 10 seconds to prevent incomplete average measurements.
	SCH_Add_Task(calculateAverageLight, 1100, 1000); //Calculate average light every 10 seconds.
	SCH_Add_Task(temperatureCheck, 1001, 1000); //What instruction should we send to screen?
	SCH_Add_Task(lightCheck, 1101, 1000);
	SCH_Add_Task(resetAverageTemperature, 1002, 1000); //reset average temperature
	SCH_Add_Task(resetAverageLight, 1102, 1000);
	//SCH_Add_Task(transmitDistance, 1000, 50); //Used for debugging
	SCH_Add_Task(checkCommand, 1000, 10); //What leds should be flashing and what should the screen do?

<<<<<<< HEAD

	SCH_Start();

=======
	
	SCH_Start();
	
>>>>>>> d86e3b6e1040ac051d3d45a21dd2d00cd12c5735
	while(1)
	{
		SCH_Dispatch_Tasks();
	}

	return 0;
}
