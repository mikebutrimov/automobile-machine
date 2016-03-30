#define packet_len 11
#define repeat_threshold 20
#define cmd_len 8
#define Clear 0
#define Payload 254  //Special type for payload command
#define HeadUnitOn 1
#define MenuEnter 10
#define Esc 11
#define RightButton 12
#define LeftButton 13
#define UpButton 14
#define DownButton 15
#define LolOk 16
#define Mode 17
#define Dark 18
#define Forward 100
#define Backward 101
#define VolumeUp 102
#define VolumeDown 103
#define Source 104

struct CAN_COMMAND {
  short count;
  short address;
  short bytes;
  short payload[8];
  unsigned long putInTime;
  int delayTime;
};





