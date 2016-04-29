#include <DueFlashStorage.h>
#include <flash_efc.h>
#include <efc.h>

#include <list.h>
#include <algorithm.h>
#include <map.h>
#include <vector.h>
#include <mcp_can.h>
#include <SPI.h>
#include "commands.h"
#include "declarations.h"
int deltaT;
const byte d1 = 20;
const byte d2 = 15;

DueFlashStorage dueFlashStorage;
MCP_CAN CAN(10); 
etl::list<CAN_COMMAND, MAX_SIZE> queue;
etl::map<byte,CAN_COMMAND,MAX_SIZE> can_commands;
etl::map<etl::vector<int,MAX_SIZE_VECTOR>,etl::vector<int,MAX_SIZE_VECTOR>, MAX_SIZE > android_commands;

//some sutable functions to work with AINET
//CRC for AINET packet
void crc(uint8_t *packet) {
  uint8_t crc_reg=0xff,poly,i,j;
  uint8_t bit_point, *byte_point;
  for (i=0, byte_point=packet; i<10; ++i, ++byte_point) {
    for (j=0, bit_point=0x80 ; j<8; ++j, bit_point>>=1) {
      if (bit_point & *byte_point) { // case for new bit =1
        if (crc_reg & 0x80) poly=1; // define the polynomial
        else poly=0x1c;
        crc_reg= ( (crc_reg << 1) | 1) ^ poly;
      } else { // case for new bit =0
        poly=0;
        if (crc_reg & 0x80) poly=0x1d;
        crc_reg= (crc_reg << 1) ^ poly;
      }
    }
  }
  packet[10]= ~crc_reg; // write use CRC
}

//Send AINET packet
void sendAiPacket(byte * packet){
   noInterrupts();
  //SOF
  digitalWrite(AINET, HIGH);
  delayMicroseconds(27);
  digitalWrite(AINET, LOW);
  delayMicroseconds(16);
  for (i=0;i<11;i++) {
    for (j=0;j<8;j++) {
      type=(packet[i] & (1 << (7-j))) >> (7-j);
      if (type==0) {
        digitalWrite(AINET, HIGH);
        delayMicroseconds(11);
        digitalWrite(AINET, LOW);
        delayMicroseconds(7);
      } else {
        digitalWrite(AINET, HIGH);
        delayMicroseconds(3);
        digitalWrite(AINET, LOW);
        delayMicroseconds(15);
      }
    }
  }
  interrupts();
}

//ISR for AINET with auto ack reply
void ISR_read(){
  t0 = micros();
  //detect rising or falling
  if (digitalRead(3)){
    t1 = micros();
  }
  else {
    //falling
    deltaT = t0-t1;
    if (deltaT < 8 ){// logical 1
      ainetbuffer[byteindex] |= 1 << (7-bitindex);
      if (++bitindex > 7) {
        bitindex=0;
        byteindex++;
      }
    }
    else if (deltaT < 16){// logical 0
      ainetbuffer[byteindex]&=~(1 << (7-bitindex));
      if (++bitindex > 7) {
        bitindex=0;
        byteindex++;
      }
    }
    else { 
      byteindex = 0;
      bitindex = 0;
    }
    //we recieve packet without an ack

    if ((byteindex==11) && (bitindex==0) && ainetbuffer[0] == 0x02){
        if ((ainetbuffer[10]&B00000001) == 0){
          delayMicroseconds(d2);
        }
        else {
          delayMicroseconds(d1);
        }
        for (j=0;j<8;j++) {
          type=(ainetbuffer[0] & (1 << (7-j))) >> (7-j);
          if (type==0) {
            digitalWrite(AINET, HIGH);
            delayMicroseconds(11);
            digitalWrite(AINET, LOW);
            delayMicroseconds(7);
          } else {
            digitalWrite(AINET, HIGH);
            delayMicroseconds(2);
            digitalWrite(AINET, LOW);
            delayMicroseconds(15);
          }
      }
      byteindex=0;
      bitindex=0;
    }
    
    if ((byteindex==12) && (bitindex==0)) {
      byteindex=0;
      bitindex=0;
    }
  }
}

//All new added ainet stuff and functions must be upper this line.
//End of simple AINET Stuff


//Volume control part
//couple of functions:
//to store and restore volume level
//to set volume level
void store_vol(uint8_t vol_value){
  dueFlashStorage.write(0,vol_value);
}

void set_vol(uint8_t vol_value){
  ainet_commands[7][3] = vol[vol_value];
  crc(ainet_commands[7]);
  sendAiPacket(ainet_commands[7]);
}

void restore_vol(){
    vol_value = dueFlashStorage.read(0);
    set_vol(vol_value);
}

void dec_vol(){
  if (vol_value <36){
    vol_value++;
  }
  store_vol(vol_value);
  set_vol(vol_value);
  Serial1.println("dec vol");
}

void inc_vol(){
  if (vol_value > 0){
    vol_value--;
  }
  store_vol(vol_value);
  set_vol(vol_value);
  Serial1.println("inc vol");
}
//END OF Volume control part



//CAN and ANDROID stuff
etl::vector<int,MAX_SIZE_VECTOR> getV(int *array, int len){
  return etl::vector<int ,MAX_SIZE_VECTOR>(array, array+len);
}


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

void fill_android_commands(){
  //fill android_commands
  //format is easy and not easy at the same time
  //cmd is an array with cmd[0] - CanID
  //and cmd[1] - cmd[8] - decoded can data
  //all this is converted to vector type to enable fast == operation

  //android_cmd is [4] array with [0] as magic_byte
  //and [1] as code and [2] and [3] as crc 
  //hope we did not need more than 255 codes for android cmd
  //also as i recall android_cmd must be convertable to byte
   
  int cmd[9] = {0};
  int android_cmd[4] = {magic_byte,0,0,0};
  short crc;
  //Forward
  cmd[0] = 0x21F;
  cmd[1] = 128;
  crc = magic_byte + Forward;
  android_cmd[1] = Forward;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,9)] = getV(android_cmd,4);
  
  //Backward
  cmd[0] = 0x21F;
  cmd[1] = 64;
  crc = magic_byte + Backward;
  android_cmd[1] = Backward;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,9)] = getV(android_cmd,4);
  
  //VolumeUp
  cmd[0] = 0x21F;
  cmd[1] = 8;
  crc = magic_byte + VolumeUp;
  android_cmd[1] = VolumeUp;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,9)] = getV(android_cmd,4);
  
  //VolumeDown
  cmd[0] = 0x21F;
  cmd[1] = 4;
  crc = magic_byte + VolumeDown;
  android_cmd[1] = VolumeDown;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,9)] = getV(android_cmd,4);
  
  //Source
  cmd[0] = 0x21F;
  cmd[1] = 2;
  crc = magic_byte + Source;
  android_cmd[1] = Source;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,9)] = getV(android_cmd,4);
}


void readCanCmd(){
  unsigned char len = 0;
  unsigned char buf[8]; 
  int canId;
  int cmd[9];
  if(CAN_MSGAVAIL == CAN.checkReceive()){
    canId = (int) CAN.getCanId();
    CAN.readMsgBuf(&len, buf); 
    for (int i = len; i< 8; i++){
      buf[i] = 0;
    }
    
    Serial1.println(canId, HEX);
    
    cmd[0] = (int) canId;
    for (int i=0; i<8; i++){
      cmd[i+1] =  buf[i];
    }
    
    for (int i = 0; i < 9; i++){
      Serial1.print(cmd[i]);
    }
    
    etl::vector<int,MAX_SIZE_VECTOR> key = getV(cmd, 9);
    
    if (android_commands.find(key) != android_commands.end()){
      etl::vector<int,MAX_SIZE_VECTOR> android_cmd = android_commands[key];
      
      byte cmd_buf[4];
      for (int i = 0; i < 4; i++){
        cmd_buf[i] = (byte) android_cmd[i];
      }
      
      //move volume control from android to arduino
      //probably this should be moved before loading command from map
      //but why not ?
      if (cmd_buf[1] == VolumeUp){
        inc_vol();
      }
      else if (cmd_buf[1] == VolumeDown){
        dec_vol();
      }
      else{
        Serial.write(cmd_buf,4);
      }
    }
  }
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
    queue.clear();
  }
  /*put CAN_COMMAND in queue*/
  if (!(std::find(queue.begin(), queue.end(), cmd) != queue.end())){
    queue.push_back(cmd);
  }
};

void dispatcher(){
  /*exec all can_commands in queue;
  if they have delayTime == not 0, 
  then check for execution needed*/
  for (etl::list<CAN_COMMAND,MAX_SIZE>::iterator i_cmd = queue.begin(); i_cmd != queue.end();){
    etl::list<CAN_COMMAND,MAX_SIZE>::iterator b_cmd; //buf iterator
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
      //for (int i = 0; i < packet_len; i++){
        //Serial1.print(buffer[i]);
      //}
      //Serial1.println();
      if (create_command(buffer,&cmd)!= 0){
        //Serial1.println("NEW READ 4");
        add_can_command(cmd);
      }
    }
  }
}


//END OF CAN and ANDROID stuf






void setup() {
  // put your setup code here, to run once:
  Serial1.begin(115200);
  Serial.begin(115200);
  fill_can_commands();
  fill_android_commands();
  pinMode(AINET, OUTPUT); 
  pinMode(3, INPUT);
  pinMode(7, INPUT);
  attachInterrupt(digitalPinToInterrupt(3), ISR_read, CHANGE);
  vol_value = 0;
  START_INIT:

    if(CAN_OK == CAN.begin(CAN_125KBPS))                   
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
  // put your main code here, to run repeatedly:
  read_cmd();
  readCanCmd();
  Serial1.println(queue.size());
  Serial1.println("dispatcher starts");
  dispatcher();
  Serial1.println("dispatcher ends");
  for (int i = 0; i< 13; i++){
    Serial.print(ainetbuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  /*for (int i = 0; i< 36; i++){
    set_vol(i);  
      for (int i = 0; i< 13; i++){
    Serial.print(ainetbuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
    delay(1000);
    
  }*/

  //Serial.println(REG_PIOD_PDSR&B00010000);
  //set_vol(7);
  //delay(1000);
}
