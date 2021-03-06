#include <mcp_can.h>
#include <SPI.h>
#include <iterator>
#include <string>
#include <vector>
#include <serstream>
#include <pnew.cpp>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include "HardwareSerial.cpp"
#include "commands.h"
#include "arduino2.h" 

MCP_CAN CAN(10); 
std::list<CAN_COMMAND> queue;
std::map<byte,CAN_COMMAND> can_commands;
std::map<std::vector<byte>,std::vector<byte> > android_commands;
const int AINET = 13;
uint8_t i,j,type;

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
uint8_t vol[VOL_LEN]={0x99,0x78,0x68,0x60,0x55,0x50,0x48,0x46,0x44,0x42,
                      0x40,0x38,0x36,0x34,0x32,0x30,0x28,0x26,0x24,0x22,
                      0x20,0x18,0x16,0x14,0x12,0x10,0x09,0x08,0x07,0x06,
                      0x05,0x04,0x03,0x02,0x01,0x00};



std::vector<byte> getV(byte *array, int len){
  return std::vector<byte>(array, array+len);
}


void sendAiPacket(byte * packet){
  //SOF
  digitalWrite2(AINET, HIGH);
  delayMicroseconds(31.5);
  digitalWrite2(AINET, LOW);
  delayMicroseconds(4);
    for (i=0;i<11;i++) {
    for (j=0;j<8;j++) {
      type=(packet[i] & (1 << (7-j))) >> (7-j);
      if (type==0) {
        digitalWrite2(AINET, HIGH);
        delayMicroseconds(16);
        digitalWrite2(AINET, LOW);
        delayMicroseconds(4);
      } else {
        digitalWrite2(AINET, HIGH);
        delayMicroseconds(8);
        digitalWrite2(AINET, LOW);
        delayMicroseconds(12);
      }
    }
   }
}
  
void volUp(){
  byte packet[11] = {0x40,0x02,0xD2,0x99,0x00,0x00,0x00,0x00,0x00,0x00,0xD7};
  packet[3] = vol[1];
  crc(packet);
  sendAiPacket(packet);
}

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
  
  //fill android_commands
  byte cmd[8] = {0,0,0,0,0,0,0,0};
  byte android_cmd[4] = {magic_byte,0,0,0};
  short crc;
  //Forward
  cmd[0] = 128;
  crc = magic_byte + Forward;
  android_cmd[1] = Forward;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,8)] = getV(android_cmd,4);
  
  //Backward
  cmd[0] = 64;
  crc = magic_byte + Backward;
  android_cmd[1] = Backward;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,8)] = getV(android_cmd,4);
  
  //VolumeUp
  cmd[0] = 8;
  crc = magic_byte + VolumeUp;
  android_cmd[1] = VolumeUp;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,8)] = getV(android_cmd,4);
  
  //VolumeDown
  cmd[0] = 4;
  crc = magic_byte + VolumeDown;
  android_cmd[1] = VolumeDown;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,8)] = getV(android_cmd,4);
  
  //Source
  cmd[0] = 2;
  crc = magic_byte + Source;
  android_cmd[1] = Source;
  android_cmd[2] = (crc>> 8) & 0xff;
  android_cmd[3] = crc& 0xff;
  android_commands[getV(cmd,8)] = getV(android_cmd,4);
  
  
}

void readCanCmd(){
  unsigned char len = 0;
  unsigned char buf[8]; 
  //Serial1.println("in read can cmd");
  if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&len, buf); 
        for (int i = len; i< 8; i++){
          buf[i] = 0;
        }
        Serial1.println("in can receive");
        std::vector<byte> key = getV(buf, 8);
        if (android_commands.find(key) != android_commands.end()){
          
          std::vector<byte> android_cmd = android_commands[key];
          byte*  cmd_buf = &android_cmd[0];
          for (int i = 0; i < 4; i ++){
          Serial1.print(cmd_buf[i]);
          }
          Serial.write(cmd_buf,4);
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
  pinMode2(AINET, OUTPUT); 
  pinMode (14, OUTPUT);
  
  /*START_INIT:

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
    }*/
    

}

void loop() {
  Serial1.println("New Loop starts here \n");
  //read_cmd();
  //readCanCmd();
  Serial1.println(queue.size());
  Serial1.println("dispatcher starts");
  //dispatcher();
  Serial1.println("dispatcher ends");
  Serial1.println("New Loop ends here \n");
  delay(100);
  digitalWrite(14, HIGH);
  noInterrupts();
  volUp();
  interrupts();
  delay(100);
  digitalWrite(14, LOW);
  
}
