/*
 * A library for controlling a Microchip rn2xx3 LoRa radio.
 *
 * @Author JP Meijers
 * @Author Nicolas Schteinschraber
 * @Date 18/12/2015
 *
 */

#include "Arduino.h"
#include "rn2xx3.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
}

/*
  @param serial Needs to be an already opened Stream ({Software/Hardware}Serial) to write to and read from.
*/
rn2xx3::rn2xx3(Stream& serial):
_serial(serial)
{
  _serial.setTimeout(2000);
}

void rn2xx3::autobaud()
{
  String response = "";
  while (response=="")
  {
    delay(1000);
    _serial.write((byte)0x00);
    _serial.write(0x55);
    _serial.println();
    _serial.println("sys get ver");
    response = _serial.readStringUntil('\n');
  }
}


RN2xx3_t rn2xx3::configureModuleType()
{
  String version = sysver();
  String model = version.substring(2,6);
  switch (model.toInt()) {
    case 2903:
      _moduleType = RN2903;
      break;
    case 2483:
      _moduleType = RN2483;
      break;
    default:
      _moduleType = RN_NA;
      break;
  }
  return _moduleType;
}
String rn2xx3::hweui()
{
  return (sendRawCommand(F("sys get hweui")));
}

String rn2xx3::appeui()
{
  return ( sendRawCommand(F("mac get appeui") ));
}

String rn2xx3::appkey()
{
  // We can't read back from module, we send the one
  // we have memorized if it has been set
  return _appskey;
}

String rn2xx3::deveui()
{
  return (sendRawCommand(F("mac get deveui")));
}



String rn2xx3::sysver()
{
  String ver = sendRawCommand(F("sys get ver"));
  ver.trim();
  return ver;
}

bool rn2xx3::init()
{
  if(_appskey=="0") //appskey variable is set by both OTAA and ABP
  {
    return false;
  }
  else if(_otaa==true)
  {
    return initOTAA(_appeui, _appskey);
  }
  else
  {
    return initABP(_devAddr, _appskey, _nwkskey);
  }
}


bool rn2xx3::initOTAA(String AppEUI, String AppKey, String DevEUI)
{
  _otaa = true;
  _nwkskey = "0";
  String receivedData;

  //clear serial buffer
  while(_serial.available())
    _serial.read();

  configureModuleType();

  switch (_moduleType) {
    case RN2903:
      sendRawCommand(F("mac reset"));
    break;
    case RN2483:
      sendRawCommand(F("mac reset 868"));
    break;
    default:
      // we shouldn't go forward with the init
      return false;
  }

  // Key not empty, use it
  if (DevEUI.length() == 16) {
      _deveui = DevEUI;
  } else {
    // If not all keys are empty 
    // In this case use on stored in RN2xx3 module
    if (AppEUI!="" && AppKey!="")
    {
      String addr = sendRawCommand(F("sys get hweui"));
      if( addr.length() == 16 )
      {
        _deveui = addr;
      }
    }
  }


  // Key not empty, use it, else use on stored in RN2xx3 module
  if ( DevEUI.length() == 16 )
  {
    sendRawCommand("mac set deveui "+_deveui);
  }

  // Key not empty, use it, else use on stored in RN2xx3 module
  if ( AppEUI.length() == 16 )
  {
      _appeui = AppEUI;
      sendRawCommand("mac set appeui "+_appeui);
  }

  // Key not empty, use it, else use on stored in RN2xx3 module
  if ( AppKey.length() == 32 )
  {
    _appskey = AppKey; //reuse the variable
    sendRawCommand("mac set appkey "+_appskey);
  }

  if (_moduleType == RN2903)
  {
    sendRawCommand(F("mac set pwridx 5"));
  }
  else
  {
    sendRawCommand(F("mac set pwridx 1"));
  }

  sendRawCommand(F("mac set adr off"));

  // Switch off automatic replies, because this library can not
  // handle more than one mac_rx per tx. See RN2483 datasheet,
  // 2.4.8.14, page 27 and the scenario on page 19.
  sendRawCommand(F("mac set ar off"));

  if (_moduleType == RN2483)
  {
    sendRawCommand(F("mac set rx2 3 869525000"));
  }

  _serial.setTimeout(30000);
  sendRawCommand(F("mac save"));

  bool joined = false;

  for(int i=0; i<2 && !joined; i++)
  {
    sendRawCommand(F("mac join otaa"));
    // Parse 2nd response
    receivedData = _serial.readStringUntil('\n');

    if(receivedData.startsWith("accepted"))
    {
      joined=true;
      delay(1000);
    }
    else
    {
      delay(1000);
    }
  }
  _serial.setTimeout(2000);
  return joined;
}


bool rn2xx3::initOTAA(uint8_t * AppEUI, uint8_t * AppKey, uint8_t * DevEUI)
{
  String app_eui;
  String dev_eui;
  String app_key;
  char buff[3];

  app_eui="";
  for (uint8_t i=0; i<8; i++)  {
    sprintf(buff, "%02X", AppEUI[i]);
    app_eui += String (buff);
  }

  dev_eui = "0";
  if (DevEUI) {
    dev_eui = "";
    for (uint8_t i=0; i<8; i++)  {
      sprintf(buff, "%02X", DevEUI[i]);
      dev_eui += String (buff);
    }
  }

  app_key="";
  for (uint8_t i=0; i<16; i++)  {
    sprintf(buff, "%02X", AppKey[i]);
    app_key += String (buff);
  }

  return initOTAA(app_eui, app_key, dev_eui);
}

bool rn2xx3::initABP(String devAddr, String AppSKey, String NwkSKey)
{
  _otaa = false;
  _devAddr = devAddr;
  _appskey = AppSKey;
  _nwkskey = NwkSKey;
  String receivedData;

  //clear serial buffer
  while(_serial.available())
    _serial.read();

  configureModuleType();

  switch (_moduleType) {
    case RN2903:
      sendRawCommand(F("mac reset"));
      break;
    case RN2483:
      sendRawCommand(F("mac reset 868"));
      sendRawCommand(F("mac set rx2 3 869525000"));
      break;
    default:
      // we shouldn't go forward with the init
      return false;
  }

  sendRawCommand("mac set nwkskey "+_nwkskey);
  sendRawCommand("mac set appskey "+_appskey);
  sendRawCommand("mac set devaddr "+_devAddr);
  sendRawCommand(F("mac set adr off"));

  // Switch off automatic replies, because this library can not
  // handle more than one mac_rx per tx. See RN2483 datasheet,
  // 2.4.8.14, page 27 and the scenario on page 19.
  sendRawCommand(F("mac set ar off"));

  if (_moduleType == RN2903)
  {
    sendRawCommand("mac set pwridx 5");
  }
  else
  {
    sendRawCommand(F("mac set pwridx 1"));
  }
  sendRawCommand(F("mac set dr 5")); //0= min, 7=max

  _serial.setTimeout(60000);
  sendRawCommand(F("mac save"));
  sendRawCommand(F("mac join abp"));
  receivedData = _serial.readStringUntil('\n');

  _serial.setTimeout(2000);
  delay(1000);

  if(receivedData.startsWith("accepted"))
  {
    return true;
    //with abp we can always join successfully as long as the keys are valid
  }
  else
  {
    return false;
  }
}

TX_RETURN_TYPE rn2xx3::tx(String data)
{
  return txUncnf(data); //we are unsure which mode we're in. Better not to wait for acks.
}

TX_RETURN_TYPE rn2xx3::txBytes(const byte* data, uint8_t size)
{
  char msgBuffer[size*2 + 1];

  char buffer[3];
  for (unsigned i=0; i<size; i++)
  {
    sprintf(buffer, "%02X", data[i]);
    memcpy(&msgBuffer[i*2], &buffer, sizeof(buffer));
  }
  String dataToTx(msgBuffer);
  return txCommand("mac tx uncnf 1 ", dataToTx, false);
}

TX_RETURN_TYPE rn2xx3::txCnf(String data)
{
  return txCommand("mac tx cnf 1 ", data, true);
}

TX_RETURN_TYPE rn2xx3::txUncnf(String data)
{
  return txCommand("mac tx uncnf 1 ", data, true);
}

TX_RETURN_TYPE rn2xx3::txCommand(String command, String data, bool shouldEncode)
{
  bool send_success = false;
  uint8_t busy_count = 0;
  uint8_t retry_count = 0;

  //clear serial buffer
  while(_serial.available())
    _serial.read();

  while(!send_success)
  {
    //retransmit a maximum of 10 times
    retry_count++;
    if(retry_count>10)
    {
      return TX_FAIL;
    }

    _serial.print(command);
    if(shouldEncode)
    {
      sendEncoded(data);
    }
    else
    {
      _serial.print(data);
    }
    _serial.println();

    String receivedData = _serial.readStringUntil('\n');

#ifdef DEBUG_RN2483
    DEBUG_RN2483.println(receivedData) ;
#endif

    if(receivedData.startsWith("ok"))
    {
      _serial.setTimeout(30000);
      receivedData = _serial.readStringUntil('\n');
#ifdef DEBUG_RN2483
      DEBUG_RN2483.println(receivedData) ;
#endif
      _serial.setTimeout(2000);

      if(receivedData.startsWith("mac_tx_ok"))
      {
        //SUCCESS!!
        send_success = true;
        return TX_SUCCESS;
      }

      else if(receivedData.startsWith("mac_rx"))
      {
        //example: mac_rx 1 54657374696E6720313233
        _rxMessenge = receivedData.substring(receivedData.indexOf(' ', 7)+1);
        send_success = true;
        return TX_WITH_RX;
      }

      else if(receivedData.startsWith("mac_err"))
      {
        init();
      }

      else if(receivedData.startsWith("invalid_data_len"))
      {
        //this should never happen if the prototype worked
        send_success = true;
        return TX_FAIL;
      }

      else if(receivedData.startsWith("radio_tx_ok"))
      {
        //SUCCESS!!
        send_success = true;
        return TX_SUCCESS;
      }

      else if(receivedData.startsWith("radio_err"))
      {
        //This should never happen. If it does, something major is wrong.
        init();
      }

      else
      {
        //unknown response
        //init();
      }
    }

    else if(receivedData.startsWith("invalid_param"))
    {
      //should not happen if we typed the commands correctly
      send_success = true;
      return TX_FAIL;
    }

    else if(receivedData.startsWith("not_joined"))
    {
      init();
    }

    else if(receivedData.startsWith("no_free_ch"))
    {
      //retry
      delay(1000);
    }

    else if(receivedData.startsWith("silent"))
    {
      init();
    }

    else if(receivedData.startsWith("frame_counter_err_rejoin_needed"))
    {
      init();
    }

    else if(receivedData.startsWith("busy"))
    {
      busy_count++;

      if(busy_count>=10)
      {
        init();
      }
      else
      {
        delay(1000);
      }
    }

    else if(receivedData.startsWith("mac_paused"))
    {
      init();
    }

    else if(receivedData.startsWith("invalid_data_len"))
    {
      //should not happen if the prototype worked
      send_success = true;
      return TX_FAIL;
    }

    else
    {
      //unknown response after mac tx command
      init();
    }
  }

  return TX_FAIL; //should never reach this
}

void rn2xx3::sendEncoded(String input)
{
  char working;
  char buffer[3];
  for (unsigned i=0; i<input.length(); i++)
  {
    working = input.charAt(i);
    sprintf(buffer, "%02x", int(working));
    _serial.print(buffer);
  }
}

String rn2xx3::base16encode(String input)
{
  char charsOut[input.length()*2+1];
  char charsIn[input.length()+1];
  input.trim();
  input.toCharArray(charsIn, input.length()+1);

  unsigned i = 0;
  for(i = 0; i<input.length()+1; i++)
  {
    if(charsIn[i] == '\0') break;

    int value = int(charsIn[i]);

    char buffer[3];
    sprintf(buffer, "%02x", value);
    charsOut[2*i] = buffer[0];
    charsOut[2*i+1] = buffer[1];
  }
  charsOut[2*i] = '\0';
  String toReturn = String(charsOut);
  return toReturn;
}

String rn2xx3::getRx() {
  return _rxMessenge;
}

String rn2xx3::base16decode(String input)
{
  char charsIn[input.length()+1];
  char charsOut[input.length()/2+1];
  input.trim();
  input.toCharArray(charsIn, input.length()+1);

  unsigned i = 0;
  for(i = 0; i<input.length()/2+1; i++)
  {
    if(charsIn[i*2] == '\0') break;
    if(charsIn[i*2+1] == '\0') break;

    char toDo[2];
    toDo[0] = charsIn[i*2];
    toDo[1] = charsIn[i*2+1];
    int out = strtoul(toDo, 0, 16);

    if(out<128)
    {
      charsOut[i] = char(out);
    }
  }
  charsOut[i] = '\0';
  return charsOut;
}

void rn2xx3::setDR(int dr)
{
  if(dr>=0 && dr<=5)
  {
    delay(100);
    while(_serial.available())
      _serial.read();
    _serial.print("mac set dr ");
    _serial.println(dr);
    _serial.readStringUntil('\n');
  }
}

void rn2xx3::sleep(long msec)
{
  _serial.print("sys sleep ");
  _serial.println(msec);
}


String rn2xx3::sendRawCommand(String command)
{
  delay(100);
  while(_serial.available())
    _serial.read();
  _serial.println(command);
  String ret = _serial.readStringUntil('\n');
  ret.trim();

#ifdef DEBUG_RN2483
  DEBUG_RN2483.println(command);
  DEBUG_RN2483.println(ret);
#endif

  return ret;
}

RN2xx3_t rn2xx3::moduleType()
{
  return _moduleType;
}

void rn2xx3::setFrequencyPlan(FREQ_PLAN fp)
{
  switch (fp)
  {
    case SINGLE_CHANNEL_EU:
      //mac set rx2 <dataRate> <frequency>
      //sendRawCommand(F("mac set rx2 5 868100000")); //use this for "strict" one channel gateways
      sendRawCommand(F("mac set rx2 3 869525000")); //use for "non-strict" one channel gateways
      sendRawCommand(F("mac set ch dcycle 0 50")); //1% duty cycle for this channel
      sendRawCommand(F("mac set ch dcycle 1 65535")); //almost never use this channel
      sendRawCommand(F("mac set ch dcycle 2 65535")); //almost never use this channel
      break;

    case TTN_EU:
      break;

    case DEFAULT_EU:
    default:
      //set 868.1, 868.3 and 868.5
      break;
  }
}
