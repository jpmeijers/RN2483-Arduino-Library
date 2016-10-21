/*
 * A library for controlling a Microchip RN2xx3 LoRa radio.
 *
 * @Author JP Meijers
 * @Author Nicolas Schteinschraber
 * @Date 18/12/2015
 *
 */

#ifndef rn2xx3_h
#define rn2xx3_h

#include "Arduino.h"
#if defined(ARDUINO_ARCH_AVR) || defined(ESP8266)
 #include <SoftwareSerial.h>
#endif

enum RN2xx3_t {
  RN_NA = 0, // Not set
  RN2903 = 2903,
  RN2483 = 2483
};

class rn2xx3
{
  public:

    #ifdef SoftwareSerial_h
    /*
     * A simplified constructor taking only a SoftwareSerial object.
     * It is assumed that LoRa WAN will be used.
     * The serial port should already be initialised when initialising this library.
     */
      rn2xx3(SoftwareSerial& serial, Stream& debugSerial);
    #endif

    /*
     * A simplified constructor taking only a HardwareSerial object.
     * It is assumed that LoRa WAN will be used.
     * The serial port should already be initialised when initialising this library.
     */
    rn2xx3(HardwareSerial& serial, Stream& debugSerial);
    /*
     * Transmit the correct sequence to the rn2483 to trigger its autobauding feature.
     * After this operation the rn2483 should communicate at the same baud rate than us.
     */
    void autobaud();

    /*
     * Auto configure for either RN2903 or RN2483 module
     */
    RN2xx3_t configureModuleType();

    /*
     * Get the hardware EUI of the radio, so that we can register it on The Things Network
     * and obtain the correct AppKey.
     * You have to have a working serial connection to the radio before calling this function.
     * In other words you have to at least call autobaud() some time before this function.
     */
    String hweui();

    /*
     * Get the RN2483's version number.
     */
    String sysver();

    /*
     * Initialise the RN2483 and join the LoRa network (if applicable).
     */
    bool init();

    /*
     * Initialise the RN2483 and join The Things Network using personalization.
     * This ignores your previous choice to use or not use the LoRa WAN.
     */
    bool initABP(String addr, String AppSKey, String NwkSKey);
    bool init(String addr, String AppSKey, String NwkSKey);

    /*
     * Initialise the RN2483 and join The Things Network using over the air activation.
     * This ignores your previous choice to use or not use the LoRa WAN.
     */
    bool initOTAA(String AppEUI, String AppKey);
    bool init(String AppEUI, String AppKey);

    /*
     * Transmit the provided data. The data is hex-encoded by this library,
     * so plain text can be provided.
     * This function is an alias for txUncnf().
     */
    void tx(String);

    /*
     * Do a confirmed transmission via LoRa WAN.
     * Note: Only use this function if LoRa WAN is used.
     */
    void txCnf(String);

    /*
     * Do an unconfirmed transmission via LoRa WAN.
     * Note: Only use this function if either LoRa WAN or TTN is used.
     */
    void txUncnf(String);

    /*
     * String the tx command to send - "mac tx cnf 1 " or "mac tx uncnf 1 "
     * String the data string
     * bool should the data string be hex encoded or not
     */
    bool txData(String, String, bool);

    /*
     * Transmit the provided data as an uncorfirmed message.
     * bool should the data be HEX encoded or not
     */
    void txData(String, bool);

    /*
     * Change the datarate at which the RN2483 transmits.
     * A value of between 0 and 5 can be specified,
     * as is defined in the LoRaWan specs.
     * This can be overwritten by the network when using OTAA.
     */
    void setDR(int dr);

    /*
     * Put the RN2483 to sleep for a specified timeframe.
     * The RN2483 accepts values from 100 to 4294967296.
     */
    void sleep(long msec);

    /*
     * Send a raw command to the RN2483 module.
     * Returns the raw string as received back from the RN2483.
     */
    String sendRawCommand(String command);


  private:
    Stream& _serial;
    Stream& _debugSerial;
    
    RN2xx3_t _moduleType = RN_NA;

    //Flags to switch code paths. Default is to use WAN (join OTAA)
    bool _otaa = true;

    //The default address to use on TTN if no address is defined.
    //This one falls in the "testing" address space.
    String _devAddr = "03FFBEEF";

    //if the hardware id can not be obtained from the module,
    // use this deveui for LoRa WAN
    String _default_deveui = "0011223344556677";

    //the appeui to use for LoRa WAN
    String _appeui = "0";

    //the nwkskey to use for LoRa WAN
    String _nwkskey = "0";

    //the appskey to use for LoRa WAN
    String _appskey = "0";


    void sendEncoded(String);
    String base16encode(String);
    String base16decode(String);
};

#endif
