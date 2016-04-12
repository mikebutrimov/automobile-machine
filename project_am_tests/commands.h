struct CAN_COMMAND {
  short count;
  short address;
  short bytes;
  short payload[8];
  unsigned long putInTime;
  int delayTime;
  
  bool operator==(const CAN_COMMAND& a) const
    {
      bool payload_chk = false;
      int equal_count = 0;
      for (int i = 0; i < 8; i++){
       if (payload[i] == a.payload[i]){
         equal_count++;
       }
      }
      if (equal_count == 8){
        payload_chk = true;
      }
      return (count == a.count && 
        address == a.address &&
        payload_chk
        );
    }
    
};

const byte packet_len = 15;
const byte magic_byte = 113;
const byte cmd_len = 8; 
const byte Clear = 0;
const byte Payload = 254;  //Special type for payload command
const byte HeadUnitOn = 1;
const byte MenuEnter = 10;
const byte Esc = 11;
const byte RightButton = 12;
const byte LeftButton = 13;
const byte UpButton = 14;
const byte DownButton = 15;
const byte LolOk = 16;
const byte Mode = 17;
const byte Dark = 18;
const byte Forward = 100;
const byte Backward = 101;
const byte VolumeUp = 102;
const byte VolumeDown = 103;
const byte Source = 104;



