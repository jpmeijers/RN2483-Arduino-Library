/*
 * A library for controlling a Microchip rn2xx3 LoRa radio.
 *
 * @Author JP Meijers
 * @Author Nicolas Schteinschraber
 * @Author The Things Network contributors
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
  @param serial Needs to be an already opened Stream ({Software/Hardwre}Serial) to write to and read from.
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
  delay(100);
  while(_serial.available())
  {
    _serial.read();
  }
  _serial.println("sys get hweui");
  String addr = _serial.readStringUntil('\n');
  addr.trim();
  return addr;
}

String rn2xx3::sysver()
{
  delay(100);
  while(_serial.available())
    _serial.read();
  _serial.println("sys get ver");
  String ver = _serial.readStringUntil('\n');
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


bool rn2xx3::initOTAA(String AppEUI, String AppKey)
{
  _otaa = true;
  _appeui = AppEUI;
  _nwkskey = "0";
  _appskey = AppKey; //reuse the variable
  String receivedData;

  //clear serial buffer
  while(_serial.available())
    _serial.read();

  _serial.println("sys get hweui");
  String addr = _serial.readStringUntil('\n');
  addr.trim();

  configureModuleType();

  switch (_moduleType) {
    case RN2903:
      _serial.println("mac reset");
      _serial.readStringUntil('\n');
      //include the bandplan settings
      configureTTNUS915();
      break;
    case RN2483:
      _serial.println("mac reset 868");
      _serial.readStringUntil('\n');
      _serial.println("mac set rx2 3 869525000");
      _serial.readStringUntil('\n');
      //include the bandplan settings
      configureTTNEU868();
      break;
    default:
      // we shouldn't go forward with the init
      return false;
  }
  receivedData = _serial.readStringUntil('\n');

  _serial.println("mac set appeui "+_appeui);
  receivedData = _serial.readStringUntil('\n');

  _serial.println("mac set appkey "+_appskey);
  receivedData = _serial.readStringUntil('\n');

  if(addr!="" && addr.length() == 16)
  {
    _serial.println("mac set deveui "+addr);
  }
  else
  {
    _serial.println("mac set deveui "+_default_deveui);
  }
  receivedData = _serial.readStringUntil('\n');

  if (_moduleType == RN2903)
  {
    _serial.println("mac set pwridx 5");
  }
  else
  {
    _serial.println("mac set pwridx 1");
  }
  receivedData = _serial.readStringUntil('\n');

  _serial.println("mac set adr off");
  receivedData = _serial.readStringUntil('\n');

  if (_moduleType == RN2483)
  {
    _serial.println("mac set rx2 3 869525000");
    receivedData = _serial.readStringUntil('\n');
  }

  _serial.setTimeout(30000);
  _serial.println("mac save");
  receivedData = _serial.readStringUntil('\n');

  bool joined = false;

  for(int i=0; i<2 && !joined; i++)
  {
    _serial.println("mac join otaa");
    receivedData = _serial.readStringUntil('\n');
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
      _serial.println("mac reset");
      _serial.readStringUntil('\n');
      //include the bandplan settings
      configureTTNUS915();
      break;
    case RN2483:
      _serial.println("mac reset 868");
      _serial.readStringUntil('\n');
      _serial.println("mac set rx2 3 869525000");
      _serial.readStringUntil('\n');
      //include the bandplan settings
      configureTTNEU868();
      break;
    default:
      // we shouldn't go forward with the init
      return false;
  }

  _serial.println("mac set nwkskey "+_nwkskey);
  _serial.readStringUntil('\n');
  _serial.println("mac set appskey "+_appskey);
  _serial.readStringUntil('\n');

  _serial.println("mac set devaddr "+_devAddr);
  _serial.readStringUntil('\n');

  _serial.println("mac set adr off");
  _serial.readStringUntil('\n');
  _serial.println("mac set ar off");
  _serial.readStringUntil('\n');

  if (_moduleType == RN2903)
  {
    _serial.println("mac set pwridx 5");
  }
  else
  {
    _serial.println("mac set pwridx 1");
  }
  _serial.readStringUntil('\n');
  _serial.println("mac set dr 5"); //0= min, 7=max
  _serial.readStringUntil('\n');

  _serial.setTimeout(60000);
  _serial.println("mac save");
  _serial.readStringUntil('\n');
  _serial.println("mac join abp");
  receivedData = _serial.readStringUntil('\n');
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

bool rn2xx3::tx(String data)
{
  return txUncnf(data); //we are unsure which mode we're in. Better not to wait for acks.
}

bool rn2xx3::txBytes(const byte* data, uint8_t size)
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

bool rn2xx3::txCnf(String data)
{
  return txCommand("mac tx cnf 1 ", data, true);
}

bool rn2xx3::txUncnf(String data)
{
  return txCommand("mac tx uncnf 1 ", data, true);
}

bool rn2xx3::txCommand(String command, String data, bool shouldEncode)
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
      return false;
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

    if(receivedData.startsWith("ok"))
    {
      _serial.setTimeout(30000);
      receivedData = _serial.readStringUntil('\n');
      _serial.setTimeout(2000);

      if(receivedData.startsWith("mac_tx_ok"))
      {
        //SUCCESS!!
        send_success = true;
        return true;
      }

      else if(receivedData.startsWith("mac_rx"))
      {
        //we received data downstream
        //TODO: handle received data -
        // this can be done by returning a struct containing:
        // 1. a boolean for confirmed message acks'
        // 2. a boolean for received data
        // 3. a string/char array for the downlink data
        send_success = true;
        return true;
      }

      else if(receivedData.startsWith("mac_err"))
      {
        init();
      }

      else if(receivedData.startsWith("invalid_data_len"))
      {
        //this should never happen if the prototype worked
        send_success = true;
        return false;
      }

      else if(receivedData.startsWith("radio_tx_ok"))
      {
        //SUCCESS!!
        send_success = true;
        return true;
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
      return false;
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
      return false;
    }

    else
    {
      //unknown response after mac tx command
      init();
    }
  }

  return false; //should never reach this
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
  return ret;
}

RN2xx3_t rn2xx3::moduleType()
{
  return _moduleType;
}

void rn2xx3::configureTTNEU868() {

  uint8_t ch;
  int8_t dr = -1;
  uint32_t freq = 867100000;
  String str = "";

  str.concat(F("mac set rx2 3 869525000"));
  sendRawCommand(str);
  str = "";
  for (ch = 0; ch <= 7; ch++) {
    if (ch >= 3) {
      str.concat(F("mac set ch freq "));
      str.concat(ch);
      str.concat(F(" "));
      str.concat(freq);
      sendRawCommand(str);

      str = "";
      str.concat(F("mac set ch drrange "));
      str.concat(ch);
      str.concat(F(" 0 5"));
      sendRawCommand(str);

      str = "";
      str.concat(F("mac set ch status "));
      str.concat(ch);
      str.concat(F(" on"));
      sendRawCommand(str);
      str = "";
      freq = freq + 200000;
    }
    str.concat(F("mac set ch dcycle "));
    str.concat(ch);
    str.concat(F(" 799"));
    sendRawCommand(str);
    str = "";
  }
  str.concat(F("mac set ch drrange 1 0 6"));
  sendRawCommand(str);


}

void rn2xx3::configureTTNUS915() {
/*
  uint8_t ch;
  int8_t dr = -1;
  String str = "";
  uint8_t chLow = fsb > 0 ? (fsb - 1) * 8 : 0;
  uint8_t chHigh = fsb > 0 ? chLow + 7 : 71;
  uint8_t ch500 = fsb + 63;

  sendCommand(F("radio set freq 904200000"));
  str = "";
  str.concat(F("mac set pwridx "));
  str.concat(TTN_PWRIDX_915);
  sendCommand(str);
  for (ch = 0; ch < 72; ch++) {
    str = "";
    str.concat(F("mac set ch status "));
    str.concat(ch);
    if (ch == ch500 || ch <= chHigh && ch >= chLow) {
      str.concat(F(" on"));
      sendCommand(str);
      if (ch < 63) {
        str = "";
        str.concat(F("mac set ch drrange "));
        str.concat(ch);
        str.concat(F(" 0 3"));
        sendCommand(str);
        str = "";
      }
    }
    else {
      str.concat(F(" off"));
      sendCommand(str);
    }
  }
*/
}
