/*
 * Author: JP Meijers
 * Date: 2016-09-02
 * 
 * Transmit GPS coordinates via LoRaWAN. This happens as fast as possible, while still keeping to 
 * the 1% duty cycle rules enforced by the RN2483's built in LoRaWAN stack. Even though this is 
 * allowed by the radio regulations of the 868MHz band, it can cause unnecessary collisions.
 * 
 * This sketch assumes you are using the Sodaq One V4 node in its original configuration.
 * 
 * This program makes use of the Sodaq UBlox library, but with minor changes to include altitude and HDOP.
 * 
 * LED indicators:
 * Blue: Busy transmitting a packet
 * Green waiting for a new GPS fix
 * Red: GPS fix taking a long time. Try to go outdoors.
 * 
 */
#include <Arduino.h>
#include "Sodaq_UBlox_GPS.h"
#include <rn2xx3.h>

//create an instance of the rn2xx3 library, 
//giving Serial1 as stream to use for communication with the radio
rn2xx3 myLora(Serial1);

String toLog;

void setup()
{
    delay(3000);

    SerialUSB.begin(57600);
    Serial1.begin(57600);

    // make sure usb serial connection is available,
    // or after 10s go on anyway for 'headless' use of the node.
    while ((!SerialUSB) && (millis() < 10000));

    SerialUSB.println("SODAQ ONE simple bicycle tracker");

    initialize_radio();
  
    // transmit a startup message
    myLora.tx("SODAQ ONE startup");

    // initialize GPS with enable on pin 27
    sodaq_gps.init(27);

    // Set the datarate/spreading factor at which we commuincate. 
    // DR5 is the fastest and best to use. DR0 is the slowest.
    // myLora.setDR(0); 

    // LED pins as outputs. HIGH=Off, LOW=On
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, HIGH);
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, HIGH);
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_GREEN, HIGH);
}

void initialize_radio()
{
  delay(100); //wait for the RN2xx3's startup message
  while(Serial1.available()){
    Serial1.read();
  }
  
  //print out the HWEUI so that we can register it via ttnctl
  String hweui = myLora.hweui();
  while(hweui.length() != 16)
  {
    SerialUSB.println("Communication with RN2xx3 unsuccesful. Power cycle the Sodaq One board.");
    delay(10000);
    hweui = myLora.hweui();
  }
  SerialUSB.println("Your Device EUI is: ");
  SerialUSB.println(hweui);
  SerialUSB.println("RN2xx3 firmware version:");
  SerialUSB.println(myLora.sysver());

  //configure your keys and join the network
  SerialUSB.println("Trying to join LoRaWAN network");
  bool join_result = false;
  
  //ABP: initABP(String addr, String AppSKey, String NwkSKey);
  //join_result = myLora.initABP("02017201", "8D7FFEF938589D95AAD928C2E2E7E48F", "AE17E567AECC8787F749A62F5541D522");
  
  //OTAA: initOTAA(String AppEUI, String AppKey);
  join_result = myLora.initOTAA("1122334455667788", "11111111111111111111111111111111");

  while(!join_result)
  {
    SerialUSB.println("Unable to join. Are your keys correct, and do you have LoRaWAN coverage?");
    delay(60000); //delay a minute before retry
    join_result = myLora.init();
  }
  SerialUSB.println("Successfully joined network");
  
}

void loop()
{
  SerialUSB.println("Waiting for GPS fix");
  
  digitalWrite(LED_GREEN, LOW);
  sodaq_gps.scan();
  digitalWrite(LED_GREEN, HIGH);

  // if the latitude is 0, we likely do not have a GPS fix yet, so wait longer
  while(sodaq_gps.getLat()==0.0)
  {
    digitalWrite(LED_RED, LOW);
    sodaq_gps.scan();
    digitalWrite(LED_RED, HIGH);
  }

  // Create the payload string. A string is used in this example. Try to use binary data.
  toLog = String(long(sodaq_gps.getLat()*1000000));
  toLog += " ";
  toLog += String(long(sodaq_gps.getLon()*1000000));
  toLog += " ";
  toLog += String(int(sodaq_gps.getAlt()));
  toLog += " ";
  toLog += String(int(sodaq_gps.getHDOP()*100));

  SerialUSB.println(toLog);
  digitalWrite(LED_BLUE, LOW);
  // Send an unconfirmed message
  myLora.tx(toLog);
  digitalWrite(LED_BLUE, HIGH);
  SerialUSB.println("TX done");
}
