/*
 *    Basic sketch for connecting
 *    a Sodaq Explorer to another Sodaq Explorer
 *
 *    Author: Dennis Ruigrok
 */

#include <rn2xx3.h>

// Explorer Serial port definitions.
#define debugSerial SerialUSB
#define loraSerial Serial2


// create an instance of the Library.
rn2xx3 myLora(loraSerial);


void setup()
{  
  // built_in led
  pinMode(LED_BUILTIN, OUTPUT);
  led_on();

  // make sure usb serial connection is available,
  // or after 10s go on anyway for 'headless' use of the
  // node.
  while ((!debugSerial) && (millis() < 10000));

  // beginning serial connections.
  debugSerial.begin(57600);
  loraSerial.begin(57600);

  //
  debugSerial.println(F("--------------------------------"));
  debugSerial.println(F("Basic sketch for communicating "));
  debugSerial.println(F("with another Sodaq Explorer"));
  debugSerial.println(F("--------------------------------"));
  led_off();

  initialize_radio();

}

void initialize_radio()
{

  myLora.autobaud();

  debugSerial.println("DevEUI? ");debugSerial.print(F("> "));
  debugSerial.println(myLora.hweui());
  debugSerial.println("Version?");debugSerial.print(F("> "));
  debugSerial.println(myLora.sysver());
  debugSerial.println(F("--------------------------------"));

  debugSerial.println(F("Setting up for listening for another explorer"));
  bool join_result = false;


  // point to point
  join_result = myLora.initP2P();
  
  
  debugSerial.println("\u2713 Successfully Activated radio 2 radio");


}




void loop()
{
    debugSerial.print("TXing");
    myLora.txCnf("Can you hear me???"); //one byte, blocking function

    switch(myLora.txCnf("!")) //one byte, blocking function
    {
      case TX_FAIL:
      {
        debugSerial.println("TX unsuccessful or not acknowledged");
        break;
      }
      case TX_SUCCESS:
      {
        debugSerial.println("TX successful and acknowledged");
        break;
      }
      case TX_WITH_RX:
      {
        String received = myLora.getRx();
        received = myLora.base16decode(received);
        debugSerial.print("Received downlink immediately: " + received);
        break;
      }
      default:
      {
        debugSerial.println("Unknown response from TX function");
      }
    }

    led_off();

    for(int i = 0; i < 3; i++)
    switch(myLora.listenP2P()) {
      case TX_WITH_RX:
      {
        String received = myLora.getRx();
        received = myLora.base16decode(received);
        debugSerial.print("Received downlink: " + received);
        break;
      }
      case RADIO_LISTEN_WITHOUT_RX:
      { 
        debugSerial.println("Listened timeout but no downlink");
        break;
      }
      
   
    }
    
    
}


void led_on()
{
  digitalWrite(LED_BUILTIN, 1);
}

void led_off()
{
  digitalWrite(LED_BUILTIN, 0);
}
