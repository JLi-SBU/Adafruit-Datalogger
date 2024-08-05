#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include "RTClib.h"
#include "Adafruit_MAX31855.h"
#include <Adafruit_NAU7802.h>
#include <Adafruit_INA260.h>

// how many milliseconds between grabbing data and logging it. 1000 ms is once a second. BEST TO SET LOG_INTERVAL & SYNC_INTERVAL EQUAL TO EACH OTHER 
#define LOG_INTERVAL  120000 // mills between entries (reduce to take more/faster data)
#define SYNC_INTERVAL 120000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // 1 = write to serial port ------- 0 = do not write to serial port 

//defining pins - Debugging LED 
#define redLED 2 
#define yellowLED 9 

//defining pins - Sensors Pins 
#define MAXDO   3
#define MAXCS   4
#define MAXCLK  5
#define MAXDO2  6
#define MAXCS2  7
#define MAXCLK2  8

RTC_PCF8523 rtc;
Adafruit_NAU7802 nau;
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);
Adafruit_MAX31855 thermocouple2(MAXCLK2, MAXCS2, MAXDO2);
Adafruit_INA260 ina260 = Adafruit_INA260();

//next are all the customizable variables some are used to display default settings or parameters
int nmax=100; // number of samples reading for average
int timer=10000; //time delay between steps
float offset, offset_raw_weight, slope;
int calibrating=1;
float known_weight = 453; // Change according to what known weight you have in grams 

// for the data logging shield, we use digital pin 10 for the SD cs line
//aka DO NOT CHANGE 
const int chipSelect = 10;
File logfile;


void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  digitalWrite(redLED, HIGH); //indicating error 

  while(1);
}

void setup(void) {

//Start serial monitor and plotter 
Serial.begin(9600);

//debugging pin setup 
pinMode (redLED,OUTPUT);
pinMode(yellowLED, OUTPUT);

digitalWrite(yellowLED, HIGH);

  // Starting both Thermocouple AMP. 

Serial.println("MAX31855 test");
  delay(500);
  Serial.print("Initializing sensor 1...");
  if (!thermocouple.begin()) {
    Serial.println("ERROR.");
    while (1) delay(10);
  }
  
  Serial.println("Initializing sensor 2...");
  if (!thermocouple2.begin()) {
    Serial.println("ERROR.");
    while (1) delay(10);
  }

  Serial.println("DONE.");

delay(1000);

//=========================================================================================================================================================

  //Starting Power Monitor 

Serial.println("Adafruit INA260 Test");
delay(100);
  if (!ina260.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1);
  }
  Serial.println("Found INA260 chip");

//=========================================================================================================================================================

  //Starting Strain Guage Amp. 

    Serial.println("NAU7802");
  if (! nau.begin()) {
    Serial.println("Failed to find NAU7802");
  }
  Serial.println("Found NAU7802");

  nau.setLDO(NAU7802_3V0);
  Serial.print("LDO voltage set to ");
  switch (nau.getLDO()) {
    case NAU7802_4V5:  Serial.println("4.5V"); break;
    case NAU7802_4V2:  Serial.println("4.2V"); break;
    case NAU7802_3V9:  Serial.println("3.9V"); break;
    case NAU7802_3V6:  Serial.println("3.6V"); break;
    case NAU7802_3V3:  Serial.println("3.3V"); break;
    case NAU7802_3V0:  Serial.println("3.0V"); break;
    case NAU7802_2V7:  Serial.println("2.7V"); break;
    case NAU7802_2V4:  Serial.println("2.4V"); break;
    case NAU7802_EXTERNAL:  Serial.println("External"); break;
  }

  nau.setGain(NAU7802_GAIN_128);
  Serial.print("Gain set to ");
  switch (nau.getGain()) {
    case NAU7802_GAIN_1:  Serial.println("1x"); break;
    case NAU7802_GAIN_2:  Serial.println("2x"); break;
    case NAU7802_GAIN_4:  Serial.println("4x"); break;
    case NAU7802_GAIN_8:  Serial.println("8x"); break;
    case NAU7802_GAIN_16:  Serial.println("16x"); break;
    case NAU7802_GAIN_32:  Serial.println("32x"); break;
    case NAU7802_GAIN_64:  Serial.println("64x"); break;
    case NAU7802_GAIN_128:  Serial.println("128x"); break;
  }

  nau.setRate(NAU7802_RATE_10SPS);
  Serial.print("Conversion rate set to ");
  switch (nau.getRate()) {
    case NAU7802_RATE_10SPS:  Serial.println("10 SPS"); break;
    case NAU7802_RATE_20SPS:  Serial.println("20 SPS"); break;
    case NAU7802_RATE_40SPS:  Serial.println("40 SPS"); break;
    case NAU7802_RATE_80SPS:  Serial.println("80 SPS"); break;
    case NAU7802_RATE_320SPS:  Serial.println("320 SPS"); break;
  }

  // Take 10 readings to flush out readings
  for (uint8_t i=0; i<10; i++) {
    while (! nau.available()) delay(1);
    nau.read();
  }

  while (! nau.calibrate(NAU7802_CALMOD_INTERNAL)) {
    Serial.println("Failed to calibrate internal offset, retrying!");
    delay(1000);
  }
  Serial.println("Calibrated internal offset");

  while (! nau.calibrate(NAU7802_CALMOD_OFFSET)) {
    Serial.println("Failed to calibrate system offset, retrying!");
    delay(1000);
  }
Serial.println("Calibrated system offset");

//=========================================================================================================================================================

  // START OF CALIBRATION CODE FOR LOAD CELL

  while (! nau.available()) {
    delay(1);
  }
  Serial.print("# of samples ");Serial.println(nmax);

  //Beginning of Step One 
  Serial.println("Step One: Average Offset Value | Do not put anything on the load cell"); 
  // Don't put anything on the load cell. 

offset = readaverage(nmax);

  Serial.print("Offset Value:");
  Serial.println(offset);


  //Beginning of Step Two
  Serial.println ("Step Two: Slope | There is a 15 second delay. Please put a known weight on the load cell");
  digitalWrite(redLED,HIGH);
  delay(15000); // Value can be changed if you want to have a shorter or longer delay
  digitalWrite(redLED,LOW);
  Serial.println("Step 2 begins");
  offset_raw_weight = readaverage(nmax);
  slope = (offset_raw_weight - offset)/known_weight; 

  Serial.print("Slope:");
  Serial.println (slope);
  

//=========================================================================================================================================================

// Initializing the SD card Section 

  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
  
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);

//=========================================================================================================================================================

  // connect to RTC
  Wire.begin();  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

if (! rtc.initialized() || rtc.lostPower()) {
Serial.println("rtc is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the rtc to the date & time this sketch was compiled
rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

//=========================================================================================================================================================

String Header = "millis,stamp,datetime,Thermocouple1,Thermocouple2,Weight(g),Current(A),Voltage(V),Power(W)";

logfile.println(Header);    
#if ECHO_TO_SERIAL
  Serial.println(Header);
#endif //ECHO_TO_SERIAL

//=========================================================================================================================================================

digitalWrite(yellowLED, LOW); 
}

void loop() {

delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));

digitalWrite(yellowLED, HIGH);

   double F1 = thermocouple.readFahrenheit(); 
   if (isnan(F1)) {
     Serial.println("Thermocouple 1 fault(s) detected!");
     uint8_t e = thermocouple.readError();
     if (e & MAX31855_FAULT_OPEN) Serial.println("FAULT: Thermocouple is open - no connections.");
     if (e & MAX31855_FAULT_SHORT_GND) Serial.println("FAULT: Thermocouple is short-circuited to GND.");
     if (e & MAX31855_FAULT_SHORT_VCC) Serial.println("FAULT: Thermocouple is short-circuited to VCC.");
   } 

   double F2 = thermocouple2.readFahrenheit(); 
   if (isnan(F2)) {
     Serial.println("Thermocouple 2 fault(s) detected!");
     uint8_t e = thermocouple.readError();
     if (e & MAX31855_FAULT_OPEN) Serial.println("FAULT: Thermocouple is open - no connections.");
     if (e & MAX31855_FAULT_SHORT_GND) Serial.println("FAULT: Thermocouple is short-circuited to GND.");
     if (e & MAX31855_FAULT_SHORT_VCC) Serial.println("FAULT: Thermocouple is short-circuited to VCC.");
   }

  while (! nau.available()) {
    delay(1);
  }
  int32_t val = nau.read();

  double adjusted = (val/slope); 

// Reading output ------------------------------------------

double I = ina260.readCurrent()/1000; 
double V = ina260.readBusVoltage()/1000; 
double W = ina260.readPower()/1000;

 DateTime now;

  // delay for the amount of time we want between readings
  
  // log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(", ");    
#if ECHO_TO_SERIAL
  Serial.print(m);         // milliseconds since start
  Serial.print(", ");  
#endif

  // fetch the time
  now = rtc.now();
  // log time
  logfile.print(now.unixtime()); // seconds since 1/1/1970
  logfile.print(", ");
  logfile.print('"');
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print('"');
#if ECHO_TO_SERIAL
  Serial.print(now.unixtime()); // seconds since 1/1/1970
  Serial.print(", ");
  Serial.print('"');
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print('"');
#endif //ECHO_TO_SERIAL

\

//START OF DATA LOGGING ON TO SD CARD 
  logfile.print(", ");    
  logfile.print(F1,3); 
  logfile.print(", ");    
  logfile.print(F2,3); 
  logfile.print(", "); 
  logfile.print(adjusted,3); 
  logfile.print(", ");    
  logfile.print(I,3); 
  logfile.print(", ");    
  logfile.print(V,3); 
  logfile.print(", "); 
  logfile.println(W,3);
  #if ECHO_TO_SERIAL
  Serial.print("Sensor1(F):");
  Serial.print(F1,3);
  Serial.print(",");
  Serial.print("Sensor2(F):");
  Serial.print(F2,3);
  Serial.print(",");
  Serial.print("Weight(g):");
  Serial.print(adjusted,3);
  Serial.print(",");
  Serial.print("Current(A): ");
  Serial.print(I,3);
  Serial.print(",");
  Serial.print("Bus Voltage(V): ");
  Serial.print(V,3);
  Serial.print(",");
  Serial.print("Power(W): ");
  Serial.println(W,3);
#endif //ECHO_TO_SERIAL
// END OF DATA LOGGING TO SD CARD


  digitalWrite(yellowLED, LOW);

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  
  // blink LED to show we are syncing data to the card & updating FAT!
  digitalWrite(redLED, HIGH);
  logfile.flush();
  digitalWrite(redLED, LOW);

}


// Average Function - Do not change unless needed------------------------------------------
float readaverage (int nmax){
  int n=1;
  float valtotal=0.0, val;
  while (n<=nmax) { 
    val = nau.read();
    valtotal = valtotal + val;
    n=n+1;
    
    Serial.print("Reading...");
    Serial.println(val);
    Serial.print(n-1);
    Serial.println("% Done");
    delay(500);
 
  }  
  return valtotal/nmax;

}