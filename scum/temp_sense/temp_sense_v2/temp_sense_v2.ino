// SCL = pin 19 (SCL0)
// SDA = pin 18 (SDA0)
// VCC = pin 17
// GND = pin 16

#include <Wire.h> // Used to establied serial communication on the I2C bus
#include "SparkFunTMP102.h" // Used to send and recieve specific information from our sensor

TMP102 sensor0(0x48); // Initialize sensor at I2C address 0x48

// Variables for command interpreter
String inputString = "";
boolean stringComplete = false; 


// Runs once at power-on
void setup() {
  // Open USB serial port; baud doesn't matter; 12Mbps regardless of setting
  Serial.begin(9600);

  // Reserve 200 bytes for the inputString:
  inputString.reserve(200);

  // Set up to two digital outputs to work as vdd/gnd
  pinMode(16,OUTPUT); 
  pinMode(17,OUTPUT); 
  digitalWrite(16,LOW);
  digitalWrite(17,HIGH);

  sensor0.begin();  // Join I2C bus
}


// Runs repeatedly
// Monitors uart for commands and then makes function call
void loop() { 
  float temperature;
  // Do nothing until a '\n' terminated string is received
  if (stringComplete) {
     if (inputString == "temp\n"){  
      temperature = read_temp();
      Serial.println(temperature);
    }      
  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;
  }
}

float read_temp() {
  float temperature = 0;  
  
  // Turn sensor on to start temperature measurement.
  // Current consumtion typically ~10uA.
  sensor0.wakeup();

  // read temperature data
  temperature = sensor0.readTempC();

  // Place sensor in sleep mode to save power.
  // Current consumtion typically <0.5uA.
  sensor0.sleep();

  return temperature;
}
/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  if (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
