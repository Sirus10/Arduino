/******************************************
# This script aim at reporting tack water level
# Site    : http://domotique.web2diz.net/
# Detail  : http://domotique.web2diz.net/?p=596
#
# License : CC BY-SA 4.0
#
# This script use the x10rf library 
# See source here : 
# https://github.com/p2baron/x10rf
#
#
/*******************************************/

// including x10rf library
#include <x10rf.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// define I/O PINS
#define SENSOR_LEVEL_0_PIN 0    // SENSOR LEVEL 0
#define SENSOR_LEVEL_1_PIN 1    // SENSOR LEVEL 1
#define SENSOR_LEVEL_2_PIN 2    // SENSOR LEVEL 2
#define SENSOR_LEVEL_3_PIN 3    // SENSOR LEVEL 3
#define RF_OUT 4                // OUTPUT RF 

x10rf myx10 = x10rf(RF_OUT,0,5);
int level = 0;
volatile boolean f_wdt = 1;

// The setup 
void setup() {
ADCSRA &= ~(1<<ADEN);// disable ADC (before power-off)
  // SETING UP THE I/O PINS
  pinMode(SENSOR_LEVEL_0_PIN, INPUT);
  pinMode(SENSOR_LEVEL_1_PIN, INPUT);
  pinMode(SENSOR_LEVEL_2_PIN, INPUT);
  pinMode(SENSOR_LEVEL_3_PIN, INPUT);
  pinMode(RF_OUT, OUTPUT);

  // MYX10 INITIALIZATION
  myx10.begin();          
   setup_watchdog(9); // approximately 8 seconds sleep 
}

// The loop
void loop() {

get_and_sent_water_level();

delay(100);

// Sleep for 8 s  8 x 75 = 600 s = 10m
for (int i=0; i<75; i++){
      system_sleep();
}
delay(100);

}


/*
      FUNCTION
      get_and_sent_water_level
*/
int get_and_sent_water_level(){

// INTERNAL PULL-UP ENABLING TO BE ABLE TO READ 
digitalWrite(SENSOR_LEVEL_0_PIN, HIGH); 
digitalWrite(SENSOR_LEVEL_1_PIN, HIGH); 
digitalWrite(SENSOR_LEVEL_2_PIN, HIGH); 
digitalWrite(SENSOR_LEVEL_3_PIN, HIGH); 

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
int level = (1-digitalRead(SENSOR_LEVEL_0_PIN)) + (1-digitalRead(SENSOR_LEVEL_1_PIN)) + (1-digitalRead(SENSOR_LEVEL_2_PIN)) + (1-digitalRead(SENSOR_LEVEL_3_PIN)) ;
/*
Serial.print(" LEVEL OF WATER  : ");
Serial.println(level);  
*/

// INTERNAL PULL-UP DESABLING TO AVOID ELECTROLYSE 
digitalWrite(SENSOR_LEVEL_0_PIN, LOW); 
digitalWrite(SENSOR_LEVEL_1_PIN, LOW); 
digitalWrite(SENSOR_LEVEL_2_PIN, LOW); 
digitalWrite(SENSOR_LEVEL_3_PIN, LOW); 

// SENDING THE LEVEL OVER RF
myx10.RFXmeter(12,0,level);

}





// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
  sleep_disable();                     // System continues execution here when watchdog timed out 
 // sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {

  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}
  
// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
  f_wdt=1;  // set global flag
}

