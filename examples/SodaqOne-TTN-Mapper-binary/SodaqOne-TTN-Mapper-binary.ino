/*
 * Author: JP Meijers
 * Date: 2016-09-02
 *
 * Transmit GPS coordinates via TTN. This happens as fast as possible, while still keeping to
 * the 1% duty cycle rules enforced by the RN2483's built in LoRaWAN stack. Even though this is
 * allowed by the radio regulations of the 868MHz band, the fair use policy of TTN may prohibit this.
 *
 * CHECK THE RULES BEFORE USING THIS PROGRAM!
 *
 * CHANGE ADDRESS!
 * Change the device address, network (session) key, and app (session) key to the values
 * that are registered via the TTN dashboard.
 * The appropriate line is "myLora.initABP(XXX);" or "myLora.initOTAA(XXX);"
 * When using ABP, it is advised to enable "relax frame count".
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
 * To decode the binary payload, you can use the following
 * javascript decoder function. It should work with the TTN console.
 *
function Decoder(bytes, port) {
  // Decode an uplink message from a buffer
  // (array) of bytes to an object of fields.
  var decoded = {};

  // if (port === 1) decoded.led = bytes[0];
  decoded.lat = ((bytes[0]<<16)>>>0) + ((bytes[1]<<8)>>>0) + bytes[2];
  decoded.lat = (decoded.lat / 16777215.0 * 180) - 90;

  decoded.lon = ((bytes[3]<<16)>>>0) + ((bytes[4]<<8)>>>0) + bytes[5];
  decoded.lon = (decoded.lon / 16777215.0 * 360) - 180;

  var altValue = ((bytes[6]<<8)>>>0) + bytes[7];
  var sign = bytes[6] & (1 << 7);
  if(sign)
  {
    decoded.alt = 0xFFFF0000 | altValue;
  }
  else
  {
    decoded.alt = altValue;
  }

  decoded.hdop = bytes[8] / 10.0;

  return decoded;
}
 *
 */
#include "Sodaq_UBlox_GPS.h"
#include <rn2xx3.h>

// Create an instance of the rn2xx3 library,
// Giving Serial1 as stream to use for communication with the radio
rn2xx3 myLora(Serial1);

String toLog;
uint8_t txBuffer[9];
uint32_t LatitudeBinary, LongitudeBinary;
uint16_t altitudeGps;
uint8_t hdopGps;
int dr = 0;

void setup()
{
    delay(3000);

    SerialUSB.begin(57600);
    Serial1.begin(57600);

    // Make sure usb serial connection is available,
    // or after 10s go on anyway for 'headless' use of the node.
    while ((!SerialUSB) && (millis() < 10000));

    SerialUSB.println("SODAQ LoRaONE TTN Mapper starting");

    initialize_radio();

    // Transmit a startup message
    myLora.tx("TTN Mapper on Sodaq One");

    // Enable next line to enable debug information of the sodaq_gps
    //sodaq_gps.setDiag(SerialUSB);

    // Initialize GPS
    sodaq_gps.init(GPS_ENABLE);

    // Set the datarate/spreading factor at which we communicate.
    // DR5 is the fastest and best to use. DR0 is the slowest.
    myLora.setDR(dr);

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

  // Print out the HWEUI so that we can register it via ttnctl
  String hweui = myLora.hweui();
  while(hweui.length() != 16)
  {
    SerialUSB.println("Communication with RN2xx3 unsuccessful. Power cycle the Sodaq One board.");
    delay(10000);
    hweui = myLora.hweui();
  }
  SerialUSB.println("When using OTAA, register this DevEUI: ");
  SerialUSB.println(hweui);
  SerialUSB.println("RN2xx3 firmware version:");
  SerialUSB.println(myLora.sysver());

  // Configure your keys and join the network
  SerialUSB.println("Trying to join TTN");
  bool join_result = false;

  const char *devAddr = "02017201";
  const char *nwkSKey = "AE17E567AECC8787F749A62F5541D522";
  const char *appSKey = "8D7FFEF938589D95AAD928C2E2E7E48F";

  join_result = myLora.initABP(devAddr, appSKey, nwkSKey);

  //OTAA: initOTAA(String AppEUI, String AppKey);
  //join_result = myLora.initOTAA("70B3D57ED00001A6", "A23C96EE13804963F8C2BD6285448198");

  while(!join_result)
  {
    SerialUSB.println("Unable to join. Are your keys correct, and do you have TTN coverage?");
    delay(60000); // Delay a minute before retry
    digitalWrite(LED_BLUE, LOW);
    join_result = myLora.init();
    digitalWrite(LED_BLUE, HIGH);
  }

  SerialUSB.println("Successfully joined TTN");

}

void loop()
{
  SerialUSB.println("Waiting for GPS fix");

  // Turn off GREEN and turn on RED led to indicate GPS usage
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);

  // Scan until we have a succesful GPS fix
  while(!sodaq_gps.scan(true))
  {
    SerialUSB.println("No GPS fix, doing another scan");
  }

  digitalWrite(LED_RED, HIGH);

  LatitudeBinary = ((sodaq_gps.getLat() + 90) / 180) * 16777215;
  LongitudeBinary = ((sodaq_gps.getLon() + 180) / 360) * 16777215;

  txBuffer[0] = ( LatitudeBinary >> 16 ) & 0xFF;
  txBuffer[1] = ( LatitudeBinary >> 8 ) & 0xFF;
  txBuffer[2] = LatitudeBinary & 0xFF;

  txBuffer[3] = ( LongitudeBinary >> 16 ) & 0xFF;
  txBuffer[4] = ( LongitudeBinary >> 8 ) & 0xFF;
  txBuffer[5] = LongitudeBinary & 0xFF;

  altitudeGps = sodaq_gps.getAlt();
  txBuffer[6] = ( altitudeGps >> 8 ) & 0xFF;
  txBuffer[7] = altitudeGps & 0xFF;

  hdopGps = sodaq_gps.getHDOP()*10;
  txBuffer[8] = hdopGps & 0xFF;

  toLog = "";
  for(size_t i = 0; i<sizeof(txBuffer); i++)
  {
    char buffer[3];
    sprintf(buffer, "%02x", txBuffer[i]);
    toLog = toLog + String(buffer);
  }

  SerialUSB.print("Transmit on DR");
  SerialUSB.print(dr);
  SerialUSB.print(" coordinates ");
  SerialUSB.print(sodaq_gps.getLat(), 13);
  SerialUSB.print(" ");
  SerialUSB.print(sodaq_gps.getLon(), 13);
  SerialUSB.print(" altitude ");
  SerialUSB.print(sodaq_gps.getAlt(), 1);
  SerialUSB.print(" and HDOP ");
  SerialUSB.print(sodaq_gps.getHDOP(), 2);
  SerialUSB.print(" hex ");
  SerialUSB.println(toLog);

  // Turn on blue to indicate Lora usage
  digitalWrite(LED_BLUE, LOW);
  myLora.txBytes(txBuffer, sizeof(txBuffer));
  digitalWrite(LED_BLUE, HIGH);

  // Cycle between datarate 0 and 5
  //dr = (dr + 1) % 6;
  //myLora.setDR(dr);

  SerialUSB.println("TX done");
}
