// **********************************************************************************
//            !!!!     ATTENTION:    !!!!
// This is just a simple receiving sketch that will work with most examples
// in the RFM69 library.
//
// If you're looking for the Gateway sketch to use with your RaspberryPi,
// as part of the PiGateway software interface (lowpowerlab.com/gateway),
// this is the wrong sketch. Use this sketch instead: PiGateway:
// https://github.com/LowPowerLab/RFM69/blob/master/Examples/PiGateway/PiGateway.ino
// **********************************************************************************
// Sample RFM69 receiver/gateway sketch, with ACK and optional encryption, and Automatic Transmission Control
// Passes through any wireless received messages to the serial port & responds to ACKs
// It also looks for an onboard FLASH chip, if present
// **********************************************************************************
// Copyright Felix Rusu 2016, http://www.LowPowerLab.com/contact
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
#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69registers.h>     //get it here: https://www.github.com/lowpowerlab/rfm69

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NODEID        1    //should always be 1 for a Gateway
#define NETWORKID     100  //the same on all nodes that talk to each other
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY     RF69_433MHZ
#define FREQUENCY     RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ

#define SERIAL_BAUD   115200

RFM69 radio;
bool spy = false; //set to 'true' to sniff all packets on the same network

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);

  radio.setHighPower(); //must include this only for RFM69HW/HCW!

  //radio.encrypt(ENCRYPTKEY);
  radio.spyMode(spy);
  
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);

  radio.writeReg(REG_BITRATEMSB,RF_BITRATEMSB_1200);   //RegBitrateMsb 1200 bitrate
  radio.writeReg(REG_BITRATELSB,RF_BITRATELSB_1200);   //RegBitrateMsb
  radio.writeReg(REG_TESTLNA, RF_TESTLNA_HIGH_SENSITIVITY); // Set LNA to high-sensitivity mode (2dB boost)

}

byte ackCount=0;
uint32_t packetCount = 0;
void loop() {
  //process any serial input
  if (Serial.available() > 0)
  {
    char input = Serial.read();
    if (input == 'r') //d=dump all register values
      radio.readAllRegs();

    if (input == 'p')
    {
      spy = !spy;
      radio.spyMode(spy);
      Serial.print("SpyMode mode ");Serial.println(spy ? "on" : "off");
    }
    
    if (input == 't')
    {
      byte temperature =  radio.readTemperature(-1); // -1 = user cal factor, adjust for correct ambient
      byte fTemp = 1.8 * temperature + 32; // 9/5=1.8
      Serial.print( "Radio Temp is ");
      Serial.print(temperature);
      Serial.print("C, ");
      Serial.print(fTemp); //converting to F loses some resolution, obvious when C is on edge between 2 values (ie 26C=78F, 27C=80F)
      Serial.println('F');
    }
  }

  if (radio.receiveDone())
  {
    Serial.print("#[");
    Serial.print(++packetCount);
    Serial.print(']');
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    if (spy) Serial.print("to [");Serial.print(radio.TARGETID, DEC);Serial.print("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");
    
    if (radio.ACKRequested())
    {
      byte theNodeID = radio.SENDERID;
      radio.sendACK();
      Serial.print(" - ACK sent.");

      // When a node requests an ACK, respond to the ACK
      // and also send a packet requesting an ACK (every 3rd one only)
      // This way both TX/RX NODE functions are tested on 1 end at the GATEWAY
      if (ackCount++%3==0)
      {
        Serial.print(" Pinging node ");
        Serial.print(theNodeID);
        Serial.print(" - ACK...");
        delay(3); //need this when sending right after reception .. ?
        if (radio.sendWithRetry(theNodeID, "ACK TEST", 8, 0, 100))  // 0 = only 1 attempt, no retries, wait 100ms max
          Serial.print("ok!");
        else Serial.print("nothing");
      }
    }
    Serial.println();
  }
  //Serial.println(radio.readRSSI());
}

void Blink(byte PIN, int DELAY_MS)
{
  /*pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);*/
}
