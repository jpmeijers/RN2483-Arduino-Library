/*
 *    Basic sketch for connecting
 *    a Sodaq Autonomo equiped with a
 *    LoRaBee to The Things Network
 *
 *    Author: Richard Verbruggen - http://vannut.nl
 */

#include <rn2xx3.h>

// Autonomo Serial port definitions.
#define debugSerial SerialUSB
#define loraSerial Serial1


// create an instance of the Library.
rn2xx3 myLora(loraSerial);


void setup()
{
  // Put power on the BeeSocket.
  digitalWrite(BEE_VCC, HIGH);

  // built_in led
  pinMode(LED_BUILTIN, OUTPUT);
  led_on();

  // make sure usb serial connection is available,
  // or after 10s go on anyway for 'headless' use of the
  // node.
  while ((!debugSerial) && (millis() < 10000));

  // beginning serial connections.
  debugSerial.begin(57600);
  loraSerial.begin(9600);

  //
  debugSerial.println(F("--------------------------------"));
  debugSerial.println(F("Basic sketch for connecting "));
  debugSerial.println(F("to The ThingsNetwork"));
  debugSerial.println(F("--------------------------------"));
  led_off();

  initialize_radio();

}

void initialize_radio()
{

  myLora.autobaud();

  // Setting the frequency plan to either TTN_US or TTN_AU. Not needed for OTAA or TTN_EU.
  //myLora.setFrequencyPlan(TTN_AU);
  
  debugSerial.println("DevEUI? ");debugSerial.print(F("> "));
  debugSerial.println(myLora.hweui());
  debugSerial.println("Version?");debugSerial.print(F("> "));
  debugSerial.println(myLora.sysver());
  debugSerial.println(F("--------------------------------"));

  debugSerial.println(F("Trying to join TTN"));
  bool join_result = false;

  /*
   * ABP: initABP(String addr, String AppSKey, String NwkSKey);
   * Paste the example code from the TTN console here:
   */
  const char *devAddr = "02017201";
  const char *nwkSKey = "AE17E567AECC8787F749A62F5541D522";
  const char *appSKey = "8D7FFEF938589D95AAD928C2E2E7E48F";

  join_result = myLora.initABP(devAddr, appSKey, nwkSKey);

  /*
   * OTAA: initOTAA(String AppEUI, String AppKey);
   * If you are using OTAA, paste the example code from the TTN console here:
   */
  //const char *appEui = "70B3D57ED00001A6";
  //const char *appKey = "A23C96EE13804963F8C2BD6285448198";

  //join_result = myLora.initOTAA(appEui, appKey);

  while(!join_result)
  {
    debugSerial.println("\u2A2F Unable to join. Are your keys correct, and do you have TTN coverage?");
    delay(30000); //delay 30s before retry
    join_result = myLora.init();
  }
  debugSerial.println("\u2713 Successfully joined TTN");


}




void loop()
{
    led_on();
    debugSerial.println(F("> TXing"));
    myLora.tx("!"); //one byte, blocking function
    led_off();

    // delay it a little bit
    // but the library manages the real dutycycle check.
    delay(200);
}


void led_on()
{
  digitalWrite(LED_BUILTIN, 1);
}

void led_off()
{
  digitalWrite(LED_BUILTIN, 0);
}
