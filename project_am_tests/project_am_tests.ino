#include <mcp_can.h>
#include <SPI.h>
#include <iterator>
#include <string>
#include <vector>
#include <serstream>
#include <pnew.cpp>
#include <list>
#include <map>can_commands
#include "commands.h"

MCP_CAN CAN(10); 
std::list<CAN_COMMAND> queue;
std::map<int,CAN_COMMAND> can_commands;

void fill_can_commands(){
  /*If our dispatcher receive this CAN_COMMAND, he will clear the queue*/
  can_commands[Clear]       = (CAN_COMMAND){0,0,0,{0,0,0,0,0,0,0,0},0,0};
  /*Turn on head unit. must send with 250 ms. interval*/
  can_commands[HeadUnitOn]  = (CAN_COMMAND){1,0x165,4,{200,192,16,0,0,0,0,0},0,250};
  /*Enter and leave menu and control cursor buttons*/
  can_commands[MenuEnter]   = (CAN_COMMAND){1,0x3e5,6,{64,0,0,0,0,0,0,0},0,0};//enter menu
  can_commands[Esc]         = (CAN_COMMAND){1,0x3e5,6,{0,0,16,0,0,0,0,0},0,0};//esc
  can_commands[RightButton] = (CAN_COMMAND){1,0x3e5,6,{0,0,0,0,0,4,0,0},0,0}; //right
  can_commands[LeftButton]  = (CAN_COMMAND){1,0x3e5,6,{0,0,0,0,0,1,0,0},0,0}; //left
  can_commands[UpButton]    = (CAN_COMMAND){1,0x3e5,6,{0,0,0,0,0,16,0,0},0,0};//down
  can_commands[DownButton]  = (CAN_COMMAND){1,0x3e5,6,{0,0,0,0,0,64,0,0},0,0};//up
  can_commands[LolOk]       = (CAN_COMMAND){1,0x3e5,6,{0,0,64,0,0,0,0,0},0,0};//ok
  can_commands[Mode]        = (CAN_COMMAND){1,0x3e5,6,{0,16,0,0,0,0,0,0},0,0};//mode
  can_commands[Dark]        = (CAN_COMMAND){1,0x3e5,6,{0,0,4,0,0,0,0,0},0,0};//dark
  /*Media controller buttons*/
  can_commands[Forward]     = (CAN_COMMAND){2,0x21f,3,{128,0,0,0,0,0,0,0},0,0};//>>
  can_commands[Backward]    = (CAN_COMMAND){2,0x21f,3,{64,0,0,0,0,0,0,0},0,0}; //<<
  can_commands[VolumeUp]    = (CAN_COMMAND){2,0x21f,3,{8,0,0,0,0,0,0,0},0,0};  //vUp
  can_commands[VolumeDown]  = (CAN_COMMAND){2,0x21f,3,{4,0,0,0,0,0,0,0},0,0};  //vDown
  can_commands[Source]      = (CAN_COMMAND){2,0x21f,3,{2,0,0,0,0,0,0,0},0,0};  //src
}

/*create command from payload-typed packet if needed*/
CAN_COMMAND create_command (short* payload){
  /*payload is 11 bytes array from android via uart */
  /*[command code, address, useful bytes count, byte0,...byte 7]*/
  CAN_COMMAND cmd;
  if (sizeof(payload) == 11) {
    
    short cmd_code = payload[0];
    if (cmd_code!= Payload){ 
      return can_commands[cmd_code];
    }
    else {
      cmd.count = 1;
      cmd.address = payload[1];
      cmd.bytes = payload[2];
      for (int i = 0; i < cmd.bytes; i++){
        cmd.payload[i] = payload[3+i];
      }  
      for (int i = cmd_len - cmd.bytes; i < cmd_len; i++){
        cmd.payload[i] = 0; // fill free space with zeros
      }
      cmd.putInTime = 0;
      cmd.delayTime = 0;
      return cmd;
    }
  }
  return cmd;
}
/*----------------------------------------------------*/

/*add can command to the queue------------------------*/
void add_can_command(CAN_COMMAND CAN_COMMAND){
  /*check for clear CAN_COMMAND*/
  /*fail enough, we don't have == for structs, but if count and address are equal 0
  then ok, i can hardly believe it is a valid can CAN_COMMAND, not 'Clear' CAN_COMMAND*/
  if (CAN_COMMAND.count == 0 && CAN_COMMAND.address == 0){
    queue.clear();
  }
  /*put CAN_COMMAND in queue*/
    queue.push_back(CAN_COMMAND);
};
/*-----------------------------------------------------*/


void dispatcher(){
  /*exec all can_commands in queue;
  if they have delayTime == not 0, 
  then check for execution needed and exec or reschedule*/
  for (std::list<CAN_COMMAND>::iterator i_cmd = queue.begin(); i_cmd != queue.end();){
    //prepare payload buffer
    CAN_COMMAND cmd =  *i_cmd;
    byte payload[cmd.bytes];
    //fill payload for can packet
    for (int i  = 0; i < cmd.bytes; i++){
      payload[i] = cmd.payload[i];
    }
    
    //check for repeatness
    if (cmd.delayTime == 0){//ok, need to push it once
      CAN.sendMsgBuf(cmd.address,0,cmd.bytes,payload);
      queue.erase(i_cmd);
    }
    else { //ok, we have periodic command
      if ( (millis() - cmd.putInTime - cmd.delayTime) < repeat_threshold){
        CAN.sendMsgBuf(cmd.address,0,cmd.bytes,payload);
        queue.erase(i_cmd);
      }
      else {
        i_cmd->putInTime = millis();
        ++i_cmd;
      }
    }    
  }
}

void readCmd(){
  int avail = Serial.available();
  if (avail > 50){
    avail = 50;
  }
  
  if (avail >0){
    Serial.println(avail);
    char *buf = new char[avail];
    Serial.println(sizeof(buf));
    Serial.readBytesUntil('x',buf,avail);
    Serial.println(buf);
    delay (100);
    delete [] buf;
  }
  Serial.flush();
}


void setup() {
  Serial.begin(115200);
  fill_can_commands();
  /*
  START_INIT:

    if(CAN_OK == CAN.begin(CAN_500KBPS))                   // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS Shield init ok!");
    }
    else
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println("Init CAN BUS Shield again");
        delay(100);
        goto START_INIT;
    }
    */

}

void loop() {
  Serial.println("New Loop starts here \n");
  readCmd();
  delay(1000);
  
}
