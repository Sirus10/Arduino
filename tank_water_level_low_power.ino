/*****************************************************
# This script aim at reporting tank water level
# Using Attiny85 devices 
# Site    : http://domotique.web2diz.net/
# Detail  : http://domotique.web2diz.net/?p=596
#
# License : CC BY-SA 4.0
#
# This script use the x10rf and sleep library 
# See source and reference here : 
# https://github.com/p2baron/x10rf
# https://gist.github.com/JChristensen/5616922
#
# To troubleshoot on Arduino you can 
# uncommend the Serial lines 
#
/******************************************************/

// including x10rf library
#include <x10rf.h>
#include <avr/sleep.h>
#define BODS 7                     //BOD Sleep bit in MCUCR
#define BODSE 2                    //BOD Sleep enable bit in MCUCR


// define I/O PINS
#define SENSOR_LEVEL_0_PIN 0    // SENSOR LEVEL 0
#define SENSOR_LEVEL_1_PIN 1    // SENSOR LEVEL 1
#define SENSOR_LEVEL_2_PIN 2    // SENSOR LEVEL 2
#define SENSOR_LEVEL_3_PIN 3    // SENSOR LEVEL 3
#define RF_OUT 4                // OUTPUT RF 

x10rf myx10 = x10rf(RF_OUT,0,5);
int level = 0;


void setup() {
// To display log on Arduino only
// Serial.begin(9600);

// Disable ADC 
ADCSRA &= ~(1<<ADEN);

// SETING UP THE I/O PINS
pinMode(SENSOR_LEVEL_0_PIN, INPUT);
pinMode(SENSOR_LEVEL_1_PIN, INPUT);
pinMode(SENSOR_LEVEL_2_PIN, INPUT);
pinMode(SENSOR_LEVEL_3_PIN, INPUT);
pinMode(RF_OUT, OUTPUT);

// MYX10 INITIALIZATION
myx10.begin();          

}

// THE MAIN LOOP
void loop() {

get_and_sent_water_level();

// Go to sleep in low power mode with x * 8s (here 80s)
	for (int i=0; i<10; i++){
		goToSleep(); // sleep for 8s
	}
}


/*
      FUNCTION
      get_and_sent_water_level
*/
int get_and_sent_water_level(){

// INTERNAL PULL-UP DESABLING TO AVOID ELECTROLYSE 
digitalWrite(SENSOR_LEVEL_0_PIN, LOW); 
digitalWrite(SENSOR_LEVEL_1_PIN, LOW); 
digitalWrite(SENSOR_LEVEL_2_PIN, LOW); 
digitalWrite(SENSOR_LEVEL_3_PIN, LOW); 

delay(100);
/*
Serial.print(" LEVELLEVELLEVEL  : ");
Serial.println(digitalRead(SENSOR_LEVEL_0_PIN));
Serial.print(" LEVELLEVELLEVEL  : ");
Serial.println(digitalRead(SENSOR_LEVEL_1_PIN));
Serial.print(" LEVELLEVEL  : ");
Serial.println(digitalRead(SENSOR_LEVEL_2_PIN));
Serial.print(" LEVEL  : ");
Serial.println(digitalRead(SENSOR_LEVEL_3_PIN));  
*/

// calculate the water level value: 
int level = (1-digitalRead(SENSOR_LEVEL_0_PIN)) + (1-digitalRead(SENSOR_LEVEL_1_PIN)) + (1-digitalRead(SENSOR_LEVEL_2_PIN)) + (1-digitalRead(SENSOR_LEVEL_3_PIN)) ;

/*
Serial.print(" LEVEL OF WATER  : ");
Serial.println(level);  
*/

// INTERNAL PULL-UP DESABLING TO AVOID ELECTROLYSE 
digitalWrite(SENSOR_LEVEL_0_PIN, HIGH); 
digitalWrite(SENSOR_LEVEL_1_PIN, HIGH); 
digitalWrite(SENSOR_LEVEL_2_PIN, HIGH); 
digitalWrite(SENSOR_LEVEL_3_PIN, HIGH); 

// SENDING THE LEVEL OVER RF
myx10.RFXmeter(12,0,level);
delay(100);
}

/*
      FUNCTION
      goToSleep
*/
void goToSleep(void)
{
    byte adcsra, mcucr1, mcucr2;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    MCUCR &= ~(_BV(ISC01) | _BV(ISC00));      //INT0 on low level
    GIMSK |= _BV(INT0);                       //enable INT0
    adcsra = ADCSRA;                          //save ADCSRA
    ADCSRA &= ~_BV(ADEN);                     //disable ADC
    cli();                                    //stop interrupts to ensure the BOD timed sequence executes as required
    mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  //turn off the brown-out detector
    mcucr2 = mcucr1 & ~_BV(BODSE);            //if the MCU does not have BOD disable capability,
    MCUCR = mcucr1;                           //  this code has no effect
    MCUCR = mcucr2;
    sei();                                    //ensure interrupts enabled so we can wake up again
    sleep_cpu();                              //go to sleep
    sleep_disable();                          //wake up here
    ADCSRA = adcsra;                          //restore ADCSRA as it was before
}

/*
      FUNCTION
	   external interrupt 0 wakes the MCU
*/
ISR(INT0_vect)
{
    GIMSK = 0;                     //disable external interrupts (only need one to wake up)
}

