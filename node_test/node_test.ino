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
#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69registers.h>//get it here: https://www.github.com/lowpowerlab/rfm69


#define NODEID        2    // keep UNIQUE for each node on same network
#define NETWORKID     100  // keep IDENTICAL on all nodes that talk to each other
#define GATEWAYID     1    // "central" node

#define FREQUENCY   RF69_868MHZ
#define SERIAL_BAUD   115200

int TRANSMITPERIOD = 200; //transmit a packet to gateway so often (in ms)
char payload[] = "123 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char buff[20];
byte sendSize=0;

RFM69 radio;

void setup() {
  Serial.begin(SERIAL_BAUD);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.setHighPower(); //must include this only for RFM69HW/HCW!

  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);

  radio.writeReg(REG_BITRATEMSB,RF_BITRATEMSB_1200);   //RegBitrateMsb 1200 bitrate
  radio.writeReg(REG_BITRATELSB,RF_BITRATELSB_1200);   //RegBitrateMsb
  radio.writeReg(REG_TESTLNA, RF_TESTLNA_HIGH_SENSITIVITY); // Set LNA to high-sensitivity mode (2dB boost)
}

long lastPeriod = 0;
void loop() {
  //process any serial input
  if (Serial.available() > 0)
  {
    char input = Serial.read();
    if (input >= 48 && input <= 57) //[0,9]
    {
      TRANSMITPERIOD = 100 * (input-48);
      if (TRANSMITPERIOD == 0) TRANSMITPERIOD = 1000;
      Serial.print("\nChanging delay to ");
      Serial.print(TRANSMITPERIOD);
      Serial.println("ms\n");
    }

    if (input == 'r') //d=dump register values
      radio.readAllRegs();
  }

  //check for any received packets
  if (radio.receiveDone())
  {
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent");
    }
    Serial.println();
  }

  int currPeriod = millis()/TRANSMITPERIOD;
  if (currPeriod != lastPeriod)
  {
    lastPeriod=currPeriod;

    //send FLASH id
    if(sendSize==0)
    {
      Serial.print("SendingID");
      sprintf(buff, "FLASH_MEM_ID:0x1337");
      byte buffLen=strlen(buff);
      if (radio.sendWithRetry(GATEWAYID, buff, buffLen, 3, 100))
        Serial.print(" ok!");
      else Serial.print(" nothing...");
      //sendSize = (sendSize + 1) % 31;
    }
    else
    {
      Serial.print("Sending[");
      Serial.print(sendSize);
      Serial.print("]: ");
      for(byte i = 0; i < sendSize; i++)
        Serial.print((char)payload[i]);

      if (radio.sendWithRetry(GATEWAYID, payload, sendSize, 3, 100))
       Serial.print(" ok!");
      else Serial.print(" nothing...");
    }
    sendSize = (sendSize + 1) % 31;
    Serial.println();
  }
}
