/*
 * A library for controlling a Microchip RN2xx3 LoRa radio.
 *
 * @Author JP Meijers
 * @Author Nicolas Schteinschraber
 * @Author The Things Network contributors
 * @Date 18/12/2015
 *
 */

#ifndef rn2xx3_h
#define rn2xx3_h

#include "Arduino.h"

enum RN2xx3_t {
  RN_NA = 0, // Not set
  RN2903 = 2903,
  RN2483 = 2483
};



class rn2xx3
{
  public:

    /*
     * A simplified constructor taking only a Stream ({Software/Hardware}Serial) object.
     * The serial port should already be initialised when initialising this library.
     */
    rn2xx3(Stream& serial);

    /*
     * Transmit the correct sequence to the rn2xx3 to trigger its autobauding feature.
     * After this operation the rn2xx3 should communicate at the same baud rate than us.
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
     * Get the RN2xx3's version number.
     */
    String sysver();

    /*
     * Initialise the RN2xx3 and join the LoRa network (if applicable).
     */
    bool init();

    /*
     * Initialise the RN2xx3 and join a network using personalization.
     *
     * addr: The device address as a HEX string. Example "0203FFEE"
     * AppSKey: Application Session Key as a HEX string. Example "8D7FFEF938589D95AAD928C2E2E7E48F"
     * NwkSKey: Network Session Key as a HEX string. Example "AE17E567AECC8787F749A62F5541D522"
     */
    bool initABP(String addr, String AppSKey, String NwkSKey);

    /*
     * Initialise the RN2xx3 and join a network using over the air activation.
     *
     * AppEUI: Application EUI as a HEX string. Example "70B3D57ED00001A6"
     * AppKey: Apllication key as a HEX string. Example "A23C96EE13804963F8C2BD6285448198"
     */
    bool initOTAA(String AppEUI, String AppKey);

    /*
     * Transmit the provided data. The data is hex-encoded by this library,
     * so plain text can be provided.
     * This function is an alias for txUncnf().
     *
     * Parameter is an ascii text string.
     */
    bool tx(String);

    /*
     * Transmit raw byte encoded data via LoRa WAN.
     * This method expects a raw byte array as first parameter.
     * The second parameter is the count of the bytes to send.
     */
    bool txBytes(const byte*, uint8_t);

    /*
     * Do a confirmed transmission via LoRa WAN.
     *
     * Parameter is an ascii text string.
     */
    bool txCnf(String);

    /*
     * Do an unconfirmed transmission via LoRa WAN.
     *
     * Parameter is an ascii text string.
     */
    bool txUncnf(String);

    /*
     * Transmit the provided data using the provided command.
     *
     * String - the tx command to send
                can only be one of "mac tx cnf 1 " or "mac tx uncnf 1 "
     * String - an ascii text string if bool is true. A HEX string if bool is false.
     * bool - should the data string be hex encoded or not
     */
    bool txCommand(String, String, bool);

    /*
     * Change the datarate at which the RN2xx3 transmits.
     * A value of between 0 and 5 can be specified,
     * as is defined in the LoRaWan specs.
     * This can be overwritten by the network when using OTAA.
     * So to force a datarate, call this function after initOTAA().
     */
    void setDR(int dr);

    /*
     * Put the RN2xx3 to sleep for a specified timeframe.
     * The RN2xx3 accepts values from 100 to 4294967296.
     */
    void sleep(long msec);

    /*
     * Send a raw command to the RN2xx3 module.
     * Returns the raw string as received back from the RN2xx3.
     * If the RN2xx3 replies with multiple line, only the first line will be returned.
     */
    String sendRawCommand(String command);


    /*
     * Returns the module type either RN2903 or RN2483, or NA.
     */
    RN2xx3_t moduleType();

  private:
    Stream& _serial;

    RN2xx3_t _moduleType = RN_NA;

    //Flags to switch code paths. Default is to use OTAA.
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

    /*
     * Auto configure for either RN2903 or RN2483 module
     */
    RN2xx3_t configureModuleType();

    void sendEncoded(String);
    String base16encode(String);
    String base16decode(String);

    //bandplan functions from The Things Network library
    void configureTTNEU868();
    void configureTTNUS915();
};

#endif
