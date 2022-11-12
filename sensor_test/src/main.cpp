#include <Arduino.h>
#include "Adafruit_MCP9808.h"
#include <DFRobot_QMC5883.h>
//#include <Wire.h>

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
DFRobot_QMC5883 compass(&Wire, /*I2C addr*/HMC5883L_ADDRESS);

// Board has a 1:1 resistor-divider of battery voltage at ADC6.

void setup() {
  Serial.begin(115200);
  while (!Serial); //waits for serial terminal to be open, necessary in newer arduino boards.
  Serial.println("Hello world!");
  
  if (!tempsensor.begin(0x18)) {
    Serial.println("Couldn't find MCP9808! Check your connections and verify the address is correct.");
    while (1);
  }
  Serial.println("Found MCP9808!");

  tempsensor.setResolution(3); // sets the resolution mode of reading, the modes are defined in the table bellow:
  // Mode Resolution SampleTime
  //  0    0.5°C       30 ms
  //  1    0.25°C      65 ms
  //  2    0.125°C     130 ms
  //  3    0.0625°C    250 ms




 while (!compass.begin())
  {
    Serial.println("Could not find a valid 5883 sensor, check wiring!");
    delay(500);
  }
  Serial.println("Found Magnetometer!");



  if(compass.isHMC())
  {
    Serial.println("Initialize HMC5883");

    //Set/get the compass signal gain range, default to be 1.3 Ga
    // compass.setRange(HMC5883L_RANGE_1_3GA);
    Serial.print("compass range is:");
    Serial.println(compass.getRange());

    //Set/get measurement mode
    // compass.setMeasurementMode(HMC5883L_CONTINOUS);
    Serial.print("compass measurement mode is:");
    Serial.println(compass.getMeasurementMode());

    //Set/get the data collection frequency of the sensor
    // compass.setDataRate(HMC5883L_DATARATE_15HZ);
    Serial.print("compass data rate is:");
    Serial.println(compass.getDataRate());

    //Get/get sensor status
    // compass.setSamples(HMC5883L_SAMPLES_8);
    Serial.print("compass samples is:");
    Serial.println(compass.getSamples());
  } else {
    Serial.println("Wrong magnetometer? (not HMC5883)");
    while(1);
  }

}

void do_temp_measurement() {
  Serial.println("wake up MCP9808.... "); // wake up MCP9808 - power consumption ~200 mikro Ampere
  tempsensor.wake();   // wake up, ready to read!

  // Read and print out the temperature, also shows the resolution mode used for reading.
  Serial.print("Resolution in mode: ");
  Serial.println (tempsensor.getResolution());
  float c = tempsensor.readTempC();
  float f = tempsensor.readTempF();
  Serial.print("Temp: "); 
  Serial.print(c, 4); Serial.print("*C\t and "); 
  Serial.print(f, 4); Serial.println("*F.");
  
  //delay(2000);
  Serial.println("Shutdown MCP9808.... ");
  tempsensor.shutdown_wake(1); // shutdown MSP9808 - power consumption ~0.1 mikro Ampere, stops temperature sampling
  Serial.println("");
  delay(200);
}

void do_mag_measurement() {
  float declinationAngle = (4.0 + (26.0 / 60.0)) / (180 / PI);
  compass.setDeclinationAngle(declinationAngle);
  sVector_t mag = compass.readRaw();
  compass.getHeadingDegrees();
  Serial.print("X:");
  Serial.print(mag.XAxis);
  Serial.print(" Y:");
  Serial.print(mag.YAxis);
  Serial.print(" Z:");
  Serial.println(mag.ZAxis);
  Serial.print("Degress = ");
  Serial.println(mag.HeadingDegress);
}

void loop() {
  //Serial.println("\n\nStarting Measurements..!\n"); 
  // do_temp_measurement();
  //do_mag_measurement();

  // wake up sensors.
  tempsensor.wake();   
  compass.setMeasurementMode(HMC5883_SINGLE);

  // TODO: check if delay necessary here?
  // 250ms conversion time at 0.06deg temp accuracy!
  // HMC5883L measurement takes ~6ms (after which device idle again, and also pulses DRDY interrupt. But its not connected -> just wait a few ms in prod device?)

  sVector_t mag = compass.readRaw();
  float c = tempsensor.readTempC();

  tempsensor.shutdown(); // shutdown MSP9808 - power consumption ~0.1 mikro Ampere, stops temperature sampling
  // compass was single read -> auto shutdown? TODO: verify in datasheet

  // Print all values to serial for now
  Serial.print("X:");
  Serial.print(mag.XAxis);
  Serial.print(" Y:");
  Serial.print(mag.YAxis);
  Serial.print(" Z:");
  Serial.print(mag.ZAxis);
  Serial.print(" C:");
  Serial.println(c);

  delay(100);
}
