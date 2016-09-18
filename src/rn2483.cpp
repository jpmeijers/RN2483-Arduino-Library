/*
 * A library for controlling a Microchip RN2483 LoRa radio.
 *
 * @Author JP Meijers
 * @Date 18/12/2015
 *
 */

#include "Arduino.h"
#include "rn2483.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
}

/*
  @param serial Needs to be an already opened stream to write to and read from.
*/
#ifdef SoftwareSerial_h
rn2483::rn2483(SoftwareSerial& serial):
_serial(serial)
{
  _serial.setTimeout(2000);
}
#endif

rn2483::rn2483(HardwareSerial& serial):
_serial(serial)
{
  _serial.setTimeout(2000);
}

void rn2483::autobaud()
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

String rn2483::hweui()
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

String rn2483::sysver()
{
  delay(100);
  while(_serial.available())
    _serial.read();
  _serial.println("sys get ver");
  String ver = _serial.readStringUntil('\n');
  ver.trim();
  return ver;
}

bool rn2483::init()
{
  if(_appeui=="0")
  {
    return false;
  }
  else if(_otaa==true)
  {
    return init(_appeui, _appskey);
  }
  else
  {
    return init(_devAddr, _appskey, _nwkskey);
  }
}


bool rn2483::initOTAA(String AppEUI, String AppKey)
{
  return init(AppEUI, AppKey);
}

bool rn2483::init(String AppEUI, String AppKey)
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
  
  _serial.println("mac reset 868");
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

  _serial.println("mac set pwridx 1");
  receivedData = _serial.readStringUntil('\n');

  _serial.println("mac set adr off");
  receivedData = _serial.readStringUntil('\n');

  _serial.println("mac set rx2 3 869525000");
  receivedData = _serial.readStringUntil('\n');

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

bool rn2483::initABP(String devAddr, String AppSKey, String NwkSKey)
{
  return init(devAddr, AppSKey, NwkSKey);
}

bool rn2483::init(String devAddr, String AppSKey, String NwkSKey)
{
  _otaa = false;
  _devAddr = devAddr;
  _appskey = AppSKey;
  _nwkskey = NwkSKey;
  String receivedData;

  //clear serial buffer
  while(_serial.available())
    _serial.read();
  
  _serial.println("mac reset 868");
  _serial.readStringUntil('\n');

  _serial.println("mac set rx2 3 869525000");
  _serial.readStringUntil('\n');

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

  _serial.println("mac set pwridx 1"); //1=max, 5=min
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

void rn2483::tx(String data)
{
  txUncnf(data); //we are unsure which mode we're in. Better not to wait for acks.
}

void rn2483::txCnf(String data)
{
  txData("mac tx cnf 1 ", data, true);
}

void rn2483::txUncnf(String data)
{
  txData("mac tx uncnf 1 ", data, true);
}

void rn2483::txData(String data, bool shouldEncode)
{
  txData("mac tx uncnf 1 ", data, shouldEncode);
}

bool rn2483::txData(String command, String data, bool shouldEncode)
{
  bool send_success = false;
  uint8_t busy_count = 0;
  uint8_t retry_count = 0;

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

void rn2483::sendEncoded(String input)
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

String rn2483::base16encode(String input)
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

String rn2483::base16decode(String input)
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

void rn2483::setDR(int dr)
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

String rn2483::sendRawCommand(String command)
{
  delay(100);
  while(_serial.available())
    _serial.read();
  _serial.println(command);
  String ret = _serial.readStringUntil('\n');
  ret.trim();
  return ret;
}
