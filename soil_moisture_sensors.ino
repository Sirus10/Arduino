/*****************************************************
# This script aim at reporting Soil moisture
# Using Attiny85 devices with low power consuption
# Site    : http://domotique.web2diz.net/
# Detail  : http://domotique.web2diz.net/?p=629
#
# License : CC BY-SA 4.0
#
# This script use the x10rf and sleep library 
# See source and reference here : 
# https://github.com/p2baron/x10rf 
# http://www.echofab.org/wiki/index.php/Capteur_d'humidit%C3%A9_DIY
#
#
/******************************************************/
// including x10rf library
#include <x10rf.h>                          // you have to add it to your Arduino library folder
// including low power consuption libraries // by default in Arduino library
#include <avr/sleep.h>
#include <avr/wdt.h>

// consuption libraries function define
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


// define I/O PINS
#define capteurHumidite A1
#define alimCaptHum 3
#define RF_OUT 4                // OUTPUT RF 

int humidite_val = 12 ;         // Initial value, why not 12 ? 
volatile boolean f_wdt = 1;
x10rf myx10 = x10rf(RF_OUT,0,5);



void setup() {
  // put your setup code here, to run once:
   setup_watchdog(9);         // Approximately 8 seconds sleep 
   pinMode(capteurHumidite, INPUT);
   pinMode(alimCaptHum, OUTPUT);
   pinMode(RF_OUT, OUTPUT);
}

void loop() {
//Switch on sensor alim
digitalWrite(alimCaptHum, HIGH);
delay(1000);
humidite_val = analogRead(capteurHumidite); 
myx10.RFXmeter(3,0,humidite_val);
//Switch off sensor alim to avoid electrolysis
digitalWrite(alimCaptHum, LOW);
delay(500);

// Sleep for 8 s  8 x 75 = 600 s = 10m
for (int i=0; i<75; i++){
      system_sleep();
}
delay(200);
}


////  bellow only for tiny //////
// set system into the sleep state 
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
