/******************************************************
* This script is a watering garden program
* Watering can be ordered by domoticz 
*
* Using Ardruino Nano but can be used with Atmega
* Site    : http://domotique.web2diz.net/
* Detail  : http://domotique.web2diz.net/?p=234
*
* License : CC BY-SA 4.0
*
* ***************** REFERENCES ************************
* This script use the x10rf and sleep library 
* See source and reference here : 
* https://github.com/p2baron/x10rf 
* http://www.echofab.org/wiki/index.php/Capteur_d'humidit%C3%A9_DIY
*
* ConnectingStuff, Oregon Scientific v2.1 Emitter
* http://www.connectingstuff.net/blog/encodage-protocoles-oregon-scientific-sur-arduino/
* Copyright (C) 2013 olivier.lebrun@gmail.com []
*
/******************************************************/

#include <RCSwitch.h>
#include <x10rf.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LowPower.h>

/**
   PINS
**/
#define RF_IN 2                   // INPUT  RF
#define RF_OUT 11                 // OUTPUT RF 
#define TEMP_PIN 4                // TEMP SENSOR.
#define POMPE_PIN 12              // OUTPUT POMPE
#define CHARGE_BATTERIE A4        // TENTION DE CHARGE BATTERIE
#define BOUTON 5                  // BOUTON POUSSOIR VERT depart manuel
#define THN132N                   // Define only T° sensor 


/**
  VARIABLES
**/
double volt = 0;            // valeur tention batterie
double minimum_battery = 12;      // niveau mini batterie pour fonctionner (en volt)
int duree_arrosage_min = 5;       // durée d'arrosage en min 
long count = 0;             // simple compteur envois de temps
double temp = 0;            // variable pour temperature
int minute_entre_envoie_donnee = 1;   // temps entre 2 envois rf  def = 8min
int sleep_min = 5;           // temps de sleep si baterrie faible  default 20min


int ChanelTemp = 0xCA;            // Chanel pour Temp
int ChanelBat =  0xBE;          // Chanel pour Volt Bat. 
int niveau_cuve=1;            // Variable pour niveau cuve, par defaut on ne s'en occupe pas... 

/**
  ACTIVATION MYX10
**/
x10rf myx10 = x10rf(RF_OUT,0,5);    
RCSwitch mySwitch = RCSwitch();
/**
  ACTIVATION ONEWIRE 
**/
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

/** 
  OREGON SETUP START
*/
const unsigned long TIME = 512;
const unsigned long TWOTIME = TIME*2;

#define SEND_HIGH() digitalWrite(RF_OUT, HIGH)
#define SEND_LOW() digitalWrite(RF_OUT, LOW)

// Buffer for Oregon message
#ifdef THN132N
  byte OregonMessageBuffer[8];
#else
  byte OregonMessageBuffer[9];
#endif
/** 
  OREGON SETUP END
*/


/**
    MAIN SETUP
**/
void setup() {

// OREGON SETUP
SEND_LOW();       //On initialise le capteur de temperature
sensors.begin();  //On initialise le capteur de temperature

// OTHER SETUP  
Serial.begin(9600);
  Serial.println("## STARTUP... ");
myx10.begin();                 // For water level
mySwitch.enableReceive(0);    // Receiver on inerrupt 0 => that is pin #2
pinMode(POMPE_PIN, OUTPUT);   
digitalWrite(POMPE_PIN, HIGH);  


// Display config 
 //get_water_level();
   Serial.println("##");
   Serial.println("## SETUP START : ");
   Serial.print(" Sleep in case of low battery (min) : ");
   Serial.println(sleep_min);
   Serial.print(" Sent data frequency (min) :  ");
   Serial.println(minute_entre_envoie_donnee);
   Serial.print(" Watering max duration (min) :   ");
   Serial.println(duree_arrosage_min);
   Serial.print(" Batterie low level warning (V) :   ");
   Serial.println(minimum_battery);


// Fisrt sent data at startup
   Serial.println("## First sent data at startup ");
   get_and_send_temp();
   get_and_send_battery_level();

   Serial.println("## SETUP ENDS... ");
   Serial.println("##");
   Serial.println("Listening for incoming RF now...");
}

/**
    MAIN LOOP
*/
void loop(){

   if (digitalRead(BOUTON) == HIGH ){       // demarrage manuel 
    Serial.println("## Arrosage manuel");  
    start_arrosage();
    }
    if (mySwitch.available()) {           // detection d'un message RF 
    int value = mySwitch.getReceivedValue();
    if (value == 0) {           // message RF incorrect
      Serial.print("Unknown encoding");   
    } 
   else {                 // message RF correct
    Serial.print("received_id: ");
    Serial.println( mySwitch.getReceivedValue() );

    if ( mySwitch.getReceivedValue()==279573 or mySwitch.getReceivedValue()==331797){ // reception message RF pour demarrage Arrosage
      Serial.println("## Arrosage Commande ON");  
      start_arrosage();        
    }
    else if ( mySwitch.getReceivedValue()==279572 or mySwitch.getReceivedValue()==331796){ // reception message RF pour Arret Arrosage
      Serial.println("## Arrosage Commande OFF");  
     // stop arrosage
    }
/*    else if ( mySwitch.getReceivedValue()==4211733){ // reception message RF niveau cuve
      Serial.println("## Niveau cuve 0");  
      niveau_cuve= 0;
    }
    else if ( mySwitch.getReceivedValue()==4194325){ // reception message RF niveau cuve
      Serial.println("## Niveau cuve 1");  
      niveau_cuve= 1;
    }
    else if ( mySwitch.getReceivedValue()==4210709){ // reception message RF niveau cuve
      Serial.println("## Niveau cuve 2");  
      niveau_cuve= 2;
      }
    else if ( mySwitch.getReceivedValue()==4198421){ // reception message RF niveau cuve
      Serial.println("## Niveau cuve 3");  
      niveau_cuve= 3;
    }
   else if ( mySwitch.getReceivedValue()==4214805){ // reception message RF niveau cuve
      Serial.println("## Niveau cuve 4");  
      niveau_cuve= 4;
    } */
      
    else{
      Serial.print("received_id: ");        // reception message RF autre
      Serial.print( mySwitch.getReceivedValue() );
      Serial.print(" but not action associated ");
    }
    }
      mySwitch.resetAvailable();
  }
  
              
if (count > double ((minute_entre_envoie_donnee)*60000) ){ // Toutes les X min
       Serial.println("SEND ALL DATA ");          
       get_and_send_temp();               // Envois temperature
       delay(100);
       get_and_send_battery_level();      // Envois niveau batterie sleep si trop bas... 
       delay(100);

count=0;    // on remet le compteur à 0
}
delay(1);
count++;  
}
////////////////////////////////////////////
//           LOOP ENDS
///////////////////////////////////////////


/**
 * FUNCTION
 * Sleep 
 */
int sleep( int minute){
    Serial.print("START Sleeping for (min) ");  
    Serial.println(minute);  
    delay (100);
    mySwitch.disableReceive();    // Receiver on inerrupt 0 => that is pin #2
    delay (100);
    for (int i=0; i< round(minute*60/8); i++){
      // Sleep for 8 s with ADC module and BOD module off
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
    }
    delay (100);
    mySwitch.enableReceive(0);      // Receiver on inerrupt 0 => that is pin #2
    Serial.println("END Sleeping");     // Reveil à la fin de la veille
}

/**
      FUNCTION
      get_and_send_temp
**/
double get_and_send_temp(){   
     //Lancement de la commande de récuperation de la temperature
         sensors.requestTemperatures();
         int testtime=0;
         temp = sensors.getTempCByIndex(0);
          while(temp > 100.00 || temp < -40.00 and testtime < 10){
             sensors.requestTemperatures();
             temp = sensors.getTempCByIndex(0);
             testtime ++;
             delay(1);
           }
         if(temp > 100.00 || temp < -40.00){
          Serial.print("Temp read not valid, I will not sent it  : ");
          Serial.println( temp); 
         }
         else {
            sent_oregon(ChanelTemp, temp ); 
            Serial.print("Temp Sent  : ");
            Serial.println( temp);
         }
         delay(10);
return temp;
}
/**
      FUNCTION
      get_and_send_battery_level
**/
double  get_and_send_battery_level(){
   bitClear(PRR, PRADC); ADCSRA |= bit(ADEN); // Enable the ADC
  // read the value from the sensor:
  volt = analogRead(CHARGE_BATTERIE);
  volt = volt * (5/1023.0) * 4 ;
  delay(10);
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
 Serial.print("Volt Sent  : ");
 Serial.println(volt);
 sent_oregon(ChanelBat,volt );    // Sent voltage

  if ( volt < minimum_battery) {      // Si  le niveau de la batterie est suffisant on continue  
    Serial.println("I need to sleep now baterie level is low");
    sleep( sleep_min);
    get_and_send_battery_level();
  }
  return volt;
}



/**
      FUNCTION
      start_arrosage
**/
int start_arrosage(){

// Check Batterie level
   Serial.print("Niveau de charge de la batterie : ");          
   Serial.print( get_and_send_battery_level());   
   Serial.println ("V");       
   if ( volt < minimum_battery ){   // si niveau batterie trop bas 
     Serial.println( " -> Le Niveau de la batterie est trop bas" );   
     Serial.println( " -> Pas d'arrosage aujourd'hui" );        
     Serial.println( "EXIT"); 
     return 0;              // exit, batterie trop bas
   }
   else {
      Serial.println( " -> Niveau Bat OK" );  
    }
// Check water level  
   Serial.print("Capteur de niveau d'eau : ");  
   Serial.print("Niveau cuve = " );   
   Serial.println( niveau_cuve);   
  if( niveau_cuve  == 0  ){       // si niveau cuve trop bas 
     Serial.println( " -> Le Niveau de la cuve est trop bas" );   
     Serial.println( " -> Pas d'arrosage aujourd'hui" );        
     Serial.println( "EXIT"); 
     return 0;              // exit, niveau cuve trop bas
  }
  else {
      Serial.println( " -> Niveau cuve OK" );   
  }
// BAtterie and water level OK we can start                     
  Serial.print("   Debut Arroage pour  ");    
  Serial.print(duree_arrosage_min);  
  Serial.println(" minutes");  

  delay(500);
  myx10.x10Switch('D',4, ON);       // Envois de l'information a domoticz, pompe allumée
  delay (500);
  count=0;                // compteur de temps remis a 0
  // Tant que on ne presse pas le bouton, que la durée d'arrosage n'est pas fini et que le niveau d'eau est OK . 
  // Pour arreter avant le temps prevu : stop via 279572 (domoticz) ou 331796 (télécomande)
  while(digitalRead(BOUTON) ==  LOW && count < long (duree_arrosage_min*60*10) && niveau_cuve > 0 && mySwitch.getReceivedValue()!=279572 && mySwitch.getReceivedValue()!=331796 ){ 
    digitalWrite(POMPE_PIN, LOW);   // allumage de la pompe
    Serial.println("   ARROSAGE EN COURS"); 
     Serial.println(count );   
          Serial.println( long (duree_arrosage_min*60*10)  );   
    delay(100);             // On incrémente le compteur de temps
    count ++;
  }   
  Serial.print("### FIN de l'arrosage ### ");

  digitalWrite(POMPE_PIN, HIGH);    // Arret de la pompe
  myx10.x10Switch('D',4, OFF);      // Envois de l'information a domoticz, pompe arretée
 
  delay(1000);
  count =0;               // compteur de temps remis a 0

}




/**
      FUNCTION
      get_water_level
**/
int get_water_level(){
      Serial.print("Niveau cuve: ");          
      Serial.println(niveau_cuve);  
      return niveau_cuve;
}



 /***************************************
 * OREGON FUNCTION START
 ****************************************/
 /**
      FUNCTION
      Send with Oregon process
**/
void sent_oregon(int Channel, double value){

#ifdef THN132N  
  byte ID[] = {0xEA,0x4C};  // Create the Oregon message for a temperature only sensor (TNHN132N)
#else
  byte ID[] = {0x1A,0x2D};  // Create the Oregon message for a temperature/humidity sensor (THGR2228N)
#endif  

  setType(OregonMessageBuffer, ID);                   // set ID
  setBatteryLevel(OregonMessageBuffer, 1);                // set battery level
  setChannel(OregonMessageBuffer, Channel);               // set chanel
  setTemperature(OregonMessageBuffer,value);                        // set temp
  calculateAndSetChecksum(OregonMessageBuffer);             // Calculate the checksum
  sendOregon(OregonMessageBuffer, sizeof(OregonMessageBuffer));       // Send the Message over RF
  SEND_LOW();       
  delayMicroseconds(TWOTIME*8);                     // Send a "pause"
  sendOregon(OregonMessageBuffer, sizeof(OregonMessageBuffer));     // Send two time 
  SEND_LOW();
  delay(10);
}
inline void sendZero(void) 
{
  SEND_HIGH();
  delayMicroseconds(TIME);
  SEND_LOW();
  delayMicroseconds(TWOTIME);
  SEND_HIGH();
  delayMicroseconds(TIME);
}
 

inline void sendOne(void) 
{
   SEND_LOW();
   delayMicroseconds(TIME);
   SEND_HIGH();
   delayMicroseconds(TWOTIME);
   SEND_LOW();
   delayMicroseconds(TIME);
}
 
inline void sendQuarterMSB(const byte data) 
{
  (bitRead(data, 4)) ? sendOne() : sendZero();
  (bitRead(data, 5)) ? sendOne() : sendZero();
  (bitRead(data, 6)) ? sendOne() : sendZero();
  (bitRead(data, 7)) ? sendOne() : sendZero();
}

inline void sendQuarterLSB(const byte data) 
{
  (bitRead(data, 0)) ? sendOne() : sendZero();
  (bitRead(data, 1)) ? sendOne() : sendZero();
  (bitRead(data, 2)) ? sendOne() : sendZero();
  (bitRead(data, 3)) ? sendOne() : sendZero();
}
 
void sendData(byte *data, byte size)
{
  for(byte i = 0; i < size; ++i)
  {
    sendQuarterLSB(data[i]);
    sendQuarterMSB(data[i]);
  }
}

void sendOregon(byte *data, byte size)
{
    sendPreamble();
    //sendSync();
    sendData(data, size);
    sendPostamble();
}

inline void sendPreamble(void)
{
  byte PREAMBLE[]={0xFF,0xFF};
  sendData(PREAMBLE, 2);
}
 
inline void sendPostamble(void)
{
#ifdef THN132N
  sendQuarterLSB(0x00);
#else
  byte POSTAMBLE[]={0x00};
  sendData(POSTAMBLE, 1);  
#endif
}

inline void sendSync(void)
{
  sendQuarterLSB(0xA);
}
 
inline void setType(byte *data, byte* type) 
{
  data[0] = type[0];
  data[1] = type[1];
}
 
inline void setChannel(byte *data, byte channel) 
{
    data[2] = channel;
}
 
inline void setId(byte *data, byte ID) 
{
  data[3] = ID;
}

void setBatteryLevel(byte *data, byte level)
{
  if(!level) data[4] = 0x0C;
  else data[4] = 0x00;
}
 
void setTemperature(byte *data, float temp) 
{
  // Set temperature sign
  if(temp < 0)
  {
    data[6] = 0x08;
    temp *= -1;  
  }
  else
  {
    data[6] = 0x00;
  }
 
  // Determine decimal and float part
  int tempInt = (int)temp;
  int td = (int)(tempInt / 10);
  int tf = (int)round((float)((float)tempInt/10 - (float)td) * 10);
 
  int tempFloat =  (int)round((float)(temp - (float)tempInt) * 10);
 
  // Set temperature decimal part
  data[5] = (td << 4);
  data[5] |= tf;
 
  // Set temperature float part
  data[4] |= (tempFloat << 4);
}

void setHumidity(byte* data, byte hum)
{
    data[7] = (hum/10);
    data[6] |= (hum - data[7]*10) << 4;
}

int Sum(byte count, const byte* data)
{
  int s = 0;
 
  for(byte i = 0; i<count;i++)
  {
    s += (data[i]&0xF0) >> 4;
    s += (data[i]&0xF);
  }
 
  if(int(count) != count)
    s += (data[count]&0xF0) >> 4;
 
  return s;
}
 
void calculateAndSetChecksum(byte* data)
{
#ifdef THN132N
    int s = ((Sum(6, data) + (data[6]&0xF) - 0xa) & 0xff);
 
    data[6] |=  (s&0x0F) << 4;     data[7] =  (s&0xF0) >> 4;
#else
    data[8] = ((Sum(8, data) - 0xa) & 0xFF);
#endif
}
 /***************************************
 * OREGON FUNCTION END
 ****************************************/
