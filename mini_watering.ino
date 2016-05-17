/*****************************************************
# This script is a simple watering garden program
# Watering occure only during night and you can set 
# several occurence per night
# 
# Using Attiny85 devices with low power consuption
# Site    : http://domotique.web2diz.net/
# Detail  : http://domotique.web2diz.net/?p=659
#
# License : CC BY-SA 4.0
#
# This script use the x10rf and sleep library 
# See source and reference here : 
# https://github.com/p2baron/x10rf 
#
/******************************************************/
// including x10rf and sleep library 
#include <x10rf.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

// DIF PIN see schemas here : http://domotique.web2diz.net/?p=659
#define RF_OUT_PIN  1   // I use this RF transmiter to sent information to Domoticz but not necessary
#define PHOTO_RES   A1  // Photoresistor 
#define RELAY       3   // Output relay command 
#define WATER_LEV   4   // Water level sensor 

// SETUP START
int watering_duration_sec = 20;       // duration of watering active (sec)
int minimum_between_watering_min = 2; // good value is 600 = 10h to avoid multiple watering per night 
int check_night_interval_min = 1;     // good value is 30 min
// SETUP END

// VARIABLE 
float photores = 0;
float night_limit =1.0;   // may need ajustement, 0.5 semaqs to b a better value.
int i=0;
int dayORnight=3;         // INITIAL Value
int previousDayORnight=3; // INITIAL Value
int DAY = 1;
int NIGHT = 0;

// SETUT FOR SLEEP - DO NOT CHANGE
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
volatile boolean f_wdt = 1;
// SETUT FOR SLEEP - DO NOT CHANGE

// RF SETUP
x10rf myx10 = x10rf(RF_OUT_PIN,0,5);


void setup() {
pinMode(WATER_LEV, OUTPUT); // DEFINE WATER SENSOR as OUTPUT TO USE INTERNAL PULL-UP 
pinMode(RELAY, OUTPUT);     
setup_watchdog(9);
myx10.begin(); 
}

void loop() {  
  
// Read Analog value from photoresistor
photores = 5.00 * (analogRead(PHOTO_RES) / 1023.00); 
if(photores < night_limit ){ // IF NIGHT
  dayORnight = NIGHT ;
}
else { // IF DAY
  dayORnight = DAY;
}

// START WATERING only is status = night and previous check was day
if (dayORnight == 0 &&  previousDayORnight == 1 && check_water() ){
  arrosage();
}


// If not yet night, then Sleep before next check

previousDayORnight = dayORnight;
system_sleep_min(60);//sleep 1h with power saving

} 



/*
 * FUNCTIONS
 */

// Do the watering 
void arrosage(){
digitalWrite(RELAY, LOW);       // START WATERING
  myx10.RFXmeter(2,0,1);        // SEND INFORMATION TO DOMOTICZ
  delay (watering_duration_sec*1000);
  digitalWrite(RELAY, HIGH);    // END WATERING 
  myx10.RFXmeter(2,0,0);        // SEND INFORMATION TO DOMOTICZ
  delay (200);
}


// Return true if water level is OK
bool check_water(){
digitalWrite(WATER_LEV, HIGH);  // INTERNAL PULL-UP ENABLING TO BE ABLE TO READ 
delay (200);
int val = digitalRead(WATER_LEV); 
delay (200);
digitalWrite(WATER_LEV, LOW);   // INTERNAL PULL-UP DESABLING TO AVOID ELECTROLYSE 
  if (  val == LOW){
     return true;
  }
  else {
      return false;
  }

}


// set system into the sleep state in minute
// system wakes up when wtchdog is timed out
void system_sleep_min(int sleep_min){
  for (int i=0; i< round(sleep_min*60/8); i++){
      // Sleep for 8 s with ADC module and BOD module off
      system_sleep();
    }
}

// set system into the sleep state for 8s
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
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
