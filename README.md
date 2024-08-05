# Adafruit-Datalogger

Documentation for the Simulation of Arduino Data Logger 

Summer 2024 

NOTE: Read and adjust the code during the first run. Must make sure all the libraries and their dependents are installed.  

 
Directions for Usage:  

Adjust these values: 

LOG_INTERVAL & SYNC_INTERVAL (LINE 10 & 11) 

How much time you want between each record (Make sure they are the same. This is the safest option) 

known_weight (LINE 39) 

The weight of the calibration (Be sure to change this value if the calibration weight is changed) 

Connect the Arduino Mimina and Computer via a cable (make sure it is not a charge only cable). In addition, make sure the SD card is in the datalogger.  

The Arduino should automatically boot up and start running the code. The Yellow LED should be powered on.  

Wait for about ~100 seconds and the red LED will turn on. This indicates to put a known weight on the strain gauge. REMEMBER to change the weight value (in grams) in the code on line 44) 

Once the red LED turns off, wait another ~100 seconds  

Eventually, both the yellow and red LED will blink periodically. This mean that calibration is completed, and measurements are currently taking place.  
  

Red LED to signifying syncing to SD Card  

Yellow LED to show data collection and working through voidsetup  

Note: DO NOT use pins 10 TO 13 as they are used by the data logger. Using these pins with any of the sensors simultaneously will result in erroneous readings.  


 =================================================================================

Issues, Solution and Comments with the sensors, Arduino Minima and other Components:  


Arduino Minima 

Issue(s) 

Connecting/communicating to the board  


Solution(s) 

Unplug and replug the cable that is connected to the Arduino on the computer side (works most of the time)  

Make sure the cable you use is not a charge-only cable  

Driver is updated for the Arduino Minima 

Device Manager → ports → USB Serial Device 

Right click the name and update the drivers  

In the Arduino IDE, make sure to connect to the correct port and model of board  

 
MAX31855 (Thermocouple Amplifiers) & Thermocouples 

Issue(s) 

Thermocouple is constantly reading 32 degrees Fahrenheit  

Error messages that pop up from the code  

Solution(s) 

To resolve the constant reading of 32 degrees Fahrenheit, make sure all the cables are connected properly, and there are no loose cables. Then make sure the code has the correct pin you have connected from screw block shield to sensor. For example, MAXDO is defined as pin 3. In real life, pin 3 should be connected to the DO pin of the MAX31855 for thermocouple 1 

To fix the error messages, make sure the power and ground cables are connected to the correct power and ground rails of the breadboard  

 
NAU7802 (Strain Gauge Amplifier)  

Issue(s) 

The code is “hanged” in the calibration portion in voidsetup() (line 103 to 167 in the code) or the calibration is not leveling and constantly changes by a large margin (at least ± 100)  

Solution(s) 

Make sure all cables are connected correctly. Not only check the connection from amplifier to the Arduino but check the connection from amplifier to strain gauge. 	 

MAKE sure all four cables of the strain gauge are connected properly. Triple check this step.    


INA260 (Power Monitor) 

Issue(s) 
No issues but use ONLY either the screw block or the pins on the sensor. This helps remove any potential miswiring and shorting possibility that may occur.  

Solution(s) 
N/A  


Adafruit Data Logger  

Issue(s) 
RedLED is stuck on HIGH 
The logger is not consistently reading data 

Solution(s) 

Is the SD card in the logger?  

When the logger is not consistently reading data, this is most likely due to an adjustment that is made to the code. If this occurs, please use the backup code that I have attached.  
