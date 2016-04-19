#include <mcp_can.h>
#include <SPI.h>
#include <iterator>
#include <string>
#include <vector>
#include <serstream>
#include <pnew.cpp>
#include <list>
#include <map>
#include <algorithm>
#include "HardwareSerial.cpp"
#include "commands.h"

MCP_CAN CAN(10); 
std::list<CAN_COMMAND> queue;
std::map<byte,CAN_COMMAND> can_commands;
//strange and important part.
//two globals for bytes read from buffer
//that go before first parsed command
//and after last parsed command in buffer
//size is really enormous 
//but as russian say "ebat, tak korolevu"
byte bfr_buffer[SERIAL_BUFFER_SIZE];
byte aft_buffer[SERIAL_BUFFER_SIZE];
//two byte flags, for number of bytes in bfr and aft buffer
byte bfr_bytes = 0;
byte aft_bytes = 0;


void fill_can_commands(){
  /*If our dispatcher receive this CAN_COMMAND, he will clear the queue*/
  can_commands[Clear]       = (CAN_COMMAND) {0,0,0,{0,0,0,0,0,0,0,0},0,0};
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


void sendCmd(CAN_COMMAND cmd){
  int count = cmd.count;
  int b_count = cmd.bytes;
  byte * buffer = new byte[b_count];
  //copy useful bytes from command to buffer to send it in CAN
  for (int i = 0; i< b_count; i++){
    buffer[i] = cmd.payload[i];
  }
  for (int i = 0; i< count; i++){
    CAN.sendMsgBuf(cmd.address, 0, b_count,buffer);
  }
  delete[] buffer;
}

/*create command from payload-typed packet if needed*/
int create_command (byte* payload, CAN_COMMAND* cmd){
  //Serial1.println("DEBUG create_command"); 
  /*payload is packet_len bytes array from android via uart */
  //check for valid command
  byte cmd_code = payload[1];
  int chk = payload[packet_len-2] << 8 | payload[packet_len-1]; //last 2 bytes 
  //Serial1.println(chk);
  if (magic_byte + cmd_code == chk){ //sanity check
    if (cmd_code!= Payload){ 
      *cmd =  can_commands[cmd_code];
      return 1;
    }
    else {
      cmd->count = 1;
      cmd->address = payload[3] << 8 | payload[4];
      cmd->bytes = payload[2];
      for (int i = 0; i < cmd->bytes; i++){
        cmd->payload[i] = payload[5+i];
      }  
      for (int i = cmd_len - cmd->bytes; i < cmd_len-2; i++){
        cmd->payload[i] = 0; // fill free space with zeros
      }
      cmd->putInTime = 0;
      cmd->delayTime = 0;
      return 1;
    }
  }
  return 0;
}



/*add can command to the queue------------------------*/
void add_can_command(CAN_COMMAND cmd){
  /*check for clear CAN_COMMAND*/
  /*fail enough, we don't have == for structs, but if count and address are equal 0
  then ok, i can hardly believe it is a valid can CAN_COMMAND, not 'Clear' CAN_COMMAND*/
  if (cmd.count == 0 && cmd.address ==  0 && cmd.bytes == 0){
    /*Serial1.println("BLAD! Struct output:");
    Serial1.print("count - > ");
    Serial1.println(cmd.count);
    Serial1.print("address - > ");
    Serial1.println(cmd.address);
    Serial1.print("bytes - > ");
    Serial1.println(cmd.bytes);
    Serial1.println("payload - > ");
    for (int i = 0; i< 8; i++){
      Serial1.print(String(cmd.payload[i]));
    }
    Serial1.print("putInTime - > ");
    Serial1.println(cmd.putInTime);
    Serial1.print("delayTime - > ");
    Serial1.println(cmd.delayTime);*/
    queue.clear();
  }
  /*put CAN_COMMAND in queue*/
    if (!(std::find(queue.begin(), queue.end(), cmd) != queue.end())){
      queue.push_back(cmd);
    }
};
/*-----------------------------------------------------*/

void dispatcher(){
  /*exec all can_commands in queue;
  if they have delayTime == not 0, 
  then check for execution needed*/
  for (std::list<CAN_COMMAND>::iterator i_cmd = queue.begin(); i_cmd != queue.end();){
    std::list<CAN_COMMAND>::iterator b_cmd; //buf iterator
    Serial1.println("DEBUG SIZE");
    Serial1.println(queue.size());
    //prepare payload buffer
    CAN_COMMAND cmd =  *i_cmd;
    byte payload[cmd.bytes];
    //fill payload for can packet
    for (int i  = 0; i < cmd.bytes; i++){
      payload[i] = cmd.payload[i];
    }
    //check for repeatness
    if (cmd.delayTime == 0){//ok, need to push it once
      //emulate can send
      //delay(20);
      //CAN.sendMsgBuf(cmd.address,0,cmd.bytes,payload);
      sendCmd(cmd);
      b_cmd = i_cmd;
      ++i_cmd;
      queue.erase(b_cmd);
    }
    else { //ok, we have periodic command
      if ( (cmd.delayTime + cmd.putInTime - millis()) > 0){
        //emulate cmd send
        //delay(20);
        //CAN.sendMsgBuf(cmd.address,0,cmd.bytes,payload);
        sendCmd(cmd);
        i_cmd->putInTime = millis();
        ++i_cmd;
      }
    }    
  }
}

/*-----------------------------------------------------*/
void read_cmd(){
  //Serial1.println("NEW READ 1");
  byte buffer[packet_len];
  CAN_COMMAND cmd;
  buffer[0] = magic_byte;

  if (Serial.read() == magic_byte){
    //Serial1.println("NEW READ 2");
    if (Serial.available() >= packet_len-1){
      //Serial1.println("NEW READ 3");
      for (int i = 1; i < packet_len; i++){
        buffer[i] = Serial.read();
      }
      for (int i = 0; i < packet_len; i++){
        //Serial1.print(buffer[i]);
      }
      Serial1.println();
      if (create_command(buffer,&cmd)!= 0){
        //Serial1.println("NEW READ 4");
        add_can_command(cmd);
      }
    }
  }
}




void read_cmd_old(){
  //reads all input buffer and finds commands
  //for each found command constructs and execs 
  //add_command function.
  //include HardwareSerial1.cpp to get defined SERIAL_BUFFER_SIZE
  //which may be 16 or 64 bytes.
  Serial1.println("DEBUG 1");
  //concat bfr and aft buffers to check for command
  if (bfr_bytes + aft_bytes == packet_len){
    Serial1.println("DEBUG 2");
    CAN_COMMAND cmd;
    byte packet[packet_len];
    for (int i = 0; i< aft_bytes; i++){
      packet[i] = aft_buffer[i];
    }
    for (int i = 0; i< bfr_bytes; i ++){
      packet[i+aft_bytes] = bfr_buffer[i];
    }
    if (create_command(packet,&cmd)!= 0){
      add_can_command(cmd);
    }
  }
  //clear aft and bfr bytes counts
  bfr_bytes = aft_bytes = 0; 
  //read serial buffer, fill bfr and aft if needed
  //parse for commands
  byte read_buffer[SERIAL_BUFFER_SIZE];
  //read buffer untill it does have bytes
  //or till it lasts 
  byte number_bytes_read = 0;
  while (Serial.available() != 0){
    if (number_bytes_read == SERIAL_BUFFER_SIZE){
      break;
    }
    
    read_buffer[number_bytes_read] = Serial.read();
    number_bytes_read++;
    
  }
  Serial1.println("DEBUG 3");
  //loop through read_buffer
  //before first magic  byte everythoing goes into 
  //bfr_buffer. Then time to look for commands.
  //when we have less bytes than valid packet len, then
  //everything goes to aft_buffer
  for (int i = 0; i < number_bytes_read;){
    Serial1.println("DEBUG 4");
    //read for the first magic byte
    if (read_buffer[i] != magic_byte && i < packet_len){
      Serial1.println("DEBUG 5");
      //if first encounter of magic byte occurs when
      //i will be less than packet_len, then ok
      //seems bfr_buffer may be legit
      //if not, it does not make sense at all
      //so we can fuck up bfr_buffer and go on
      bfr_buffer[i] = read_buffer[i];
      bfr_bytes++;
    }
    else if (number_bytes_read - i < packet_len){
      Serial1.println("DEBUG 6");
      //seems it is less bytes in buffer that can be a valid command
      //so copy them to aft_buffer and update aft_bytes count
      for (int j = 0; j < number_bytes_read - i; j++){
        aft_buffer[j] = read_buffer[j+i];
        aft_bytes++;
      }
      //terminate the loop
      break;
    }
    else{
      Serial1.println("DEBUG 7");
      //back-up i as magic byte element
      int magic_i = i;
      //fast forward i to skip possible command
      i = i+ packet_len;
      //read bytes as command
      CAN_COMMAND cmd;
      byte packet[packet_len];
      for (int j = 0; j< packet_len; j++){
        packet[j] = read_buffer[magic_i+j];
      }
      //if it is command - add it to the que
      if (create_command(packet,&cmd)!= 0){
        add_can_command(cmd);
      }
    }
  i++;
  }
}


void setup() {
  Serial1.begin(115200);
  Serial.begin(115200);
  fill_can_commands();
  
  START_INIT:

    if(CAN_OK == CAN.begin(CAN_125KBPS))                   // init can bus : baudrate = 500k
    {
        Serial1.println("CAN BUS Shield init ok!");
    }
    else
    {
        Serial1.println("CAN BUS Shield init fail");
        Serial1.println("Init CAN BUS Shield again");
        delay(100);
        goto START_INIT;
    }
    

}

void loop() {
  Serial1.println("New Loop starts here \n");
  read_cmd();
  Serial1.println(queue.size());
  Serial1.println("dispatcher starts");
  dispatcher();
  Serial1.println("dispatcher ends");
  Serial1
  .println("New Loop ends here \n");
  delay(200);
}
