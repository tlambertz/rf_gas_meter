// Sample RFM69 sender/node sketch, with ACK and optional encryption, and Automatic Transmission Control
// Sends periodic messages of increasing length to gateway (id=1)
// It also looks for an onboard FLASH chip, if present
// **********************************************************************************
// Copyright Felix Rusu 2018, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it
// and/or modify it under the terms of the GNU General
// Public License as published by the Free Software
// Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public
// License for more details.
//
// Licence can be viewed at
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69.h>          //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69registers.h> //get it here: https://www.github.com/lowpowerlab/rfm69

#include "Adafruit_MCP9808.h"
#include <DFRobot_QMC5883.h>

// Create Sensors Objects
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
DFRobot_QMC5883 compass(&Wire, /*I2C addr*/ HMC5883L_ADDRESS);

#define NODEID 2      // keep UNIQUE for each node on same network
#define NETWORKID 100 // keep IDENTICAL on all nodes that talk to each other
#define GATEWAYID 1   // "central" node

#define FREQUENCY RF69_868MHZ
#define SERIAL_BAUD 115200

int TRANSMITPERIOD = 500; // transmit a packet to gateway so often (in ms)
char payload[] = "123 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char buff[20];

RFM69 radio;

void setup()
{
  Serial.begin(SERIAL_BAUD);
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  radio.setHighPower(); // must include this only for RFM69HW/HCW!

  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868
                                                                                                         : 915);
  Serial.println(buff);

  radio.writeReg(REG_BITRATEMSB, RF_BITRATEMSB_1200);       // RegBitrateMsb 1200 bitrate
  radio.writeReg(REG_BITRATELSB, RF_BITRATELSB_1200);       // RegBitrateMsb
  radio.writeReg(REG_TESTLNA, RF_TESTLNA_HIGH_SENSITIVITY); // Set LNA to high-sensitivity mode (2dB boost)

  // Setup Sensors

  if (!tempsensor.begin(0x18))
  {
    Serial.println("Couldn't find MCP9808! Check your connections and verify the address is correct.");
    while (1)
      ;
  }
  Serial.println("Found MCP9808!");

  tempsensor.setResolution(2); // sets the resolution mode of reading, the modes are defined in the table bellow:
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

  if (compass.isHMC())
  {
    Serial.println("Initialize HMC5883");

    // Set/get the compass signal gain range, default to be 1.3 Ga
    //  compass.setRange(HMC5883L_RANGE_1_3GA);
    Serial.print("compass range is:");
    Serial.println(compass.getRange());

    // Set/get measurement mode
    //  compass.setMeasurementMode(HMC5883L_CONTINOUS);
    Serial.print("compass measurement mode is:");
    Serial.println(compass.getMeasurementMode());

    // Set/get the data collection frequency of the sensor
    //  compass.setDataRate(HMC5883L_DATARATE_15HZ);
    Serial.print("compass data rate is:");
    Serial.println(compass.getDataRate());

    // Get/get sensor status
    //  compass.setSamples(HMC5883L_SAMPLES_8);
    Serial.print("compass samples is:");
    Serial.println(compass.getSamples());
  }
  else
  {
    Serial.println("Wrong magnetometer? (not HMC5883)");
    while (1)
      ;
  }
}

long lastPeriod = 0;
void loop()
{
  // process any serial input
  if (Serial.available() > 0)
  {
    char input = Serial.read();
    if (input >= 48 && input <= 57) //[0,9]
    {
      TRANSMITPERIOD = 100 * (input - 48);
      if (TRANSMITPERIOD == 0)
        TRANSMITPERIOD = 1000;
      Serial.print("\nChanging delay to ");
      Serial.print(TRANSMITPERIOD);
      Serial.println("ms\n");
    }

    if (input == 'r') // d=dump register values
      radio.readAllRegs();
  }

  // check for any received packets
  if (radio.receiveDone())
  {
    Serial.print('[');
    Serial.print(radio.SENDERID, DEC);
    Serial.print("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);
    Serial.print("   [RX_RSSI:");
    Serial.print(radio.RSSI);
    Serial.print("]");

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent");
    }
    Serial.println();
  }

  int currPeriod = millis() / TRANSMITPERIOD;
  if (currPeriod != lastPeriod)
  {
    lastPeriod = currPeriod;

    // wake up sensors.
    tempsensor.wake();
    compass.setMeasurementMode(HMC5883_SINGLE);

    // TODO: check if delay necessary here?
    // 250ms conversion time at 0.06deg temp accuracy!
    // HMC5883L measurement takes ~6ms (after which device idle again, and also pulses DRDY interrupt. But its not connected -> just wait a few ms in prod device?)

    sVector_t mag = compass.readRaw();
    float c = tempsensor.readTempC();

    // Print all values to serial for now
    Serial.print("X:");
    Serial.print(mag.XAxis);
    Serial.print(" Y:");
    Serial.print(mag.YAxis);
    Serial.print(" Z:");
    Serial.print(mag.ZAxis);
    Serial.print(" C:");
    Serial.println(c);

    // ADC6 has 2:1 resistor divider for battery voltage.
    float bat_voltage = 0.0064453125 * (float)analogRead(A6);


    static char c_str[15];
    dtostrf(c,4,2,c_str);
    static char battery_str[15];
    dtostrf(bat_voltage,4,2,battery_str);
    int payloadSize = sprintf(payload, "%d,%d,%d,%s,%s", mag.XAxis, mag.YAxis, mag.ZAxis, c_str, battery_str);

    Serial.print("Sending ");
    Serial.println(payload);
    
    if (radio.sendWithRetry(GATEWAYID, payload, payloadSize, 3, 100))
      Serial.print(" ok!");
    else
      Serial.print(" nothing...");
 
    Serial.println();
    
    tempsensor.shutdown(); // shutdown MSP9808 - power consumption ~0.1 mikro Ampere, stops temperature sampling
    // compass was single read -> auto shutdown? TODO: verify in datasheet

  }
}
