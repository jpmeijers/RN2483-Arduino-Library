/*
 * A library for controlling a Microchip RN2483 LoRa radio.
 *
 * @Author JP Meijers
 * @Date 18/12/2015
 *
 */

#ifndef rn2483_h
#define rn2483_h

#include "Arduino.h"
#if defined(ARDUINO_ARCH_AVR)
 #include <SoftwareSerial.h>
#endif

class rn2483
{
  public:
    /*
     * A simplified constructor taking only a SoftwareSerial object.
     * It is assumed that LoRa WAN will be used.
     * The serial port should already be initialised when initialising this library.
     */
    #ifdef SoftwareSerial_h
      rn2483(SoftwareSerial& serial);
    #endif

    /*
     * A simplified constructor taking only a HardwareSerial object.
     * It is assumed that LoRa WAN will be used.
     * The serial port should already be initialised when initialising this library.
     */
    rn2483(HardwareSerial& serial);

    /*
     * Transmit the correct sequence to the rn2483 to trigger its autobauding feature.
     * After this operation the rn2483 should communicate at the same baud rate than us.
     */
    void autobaud();
    
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
     * Send a raw command to the RN2483 module.
     * Returns the raw string as received back from the RN2483.
     */
    String sendRawCommand(String command);


  private:
    Stream& _serial;

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
