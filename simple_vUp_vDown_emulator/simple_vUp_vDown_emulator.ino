#include <Button.h>

#include <SPI.h>
#include "mcp_can.h"
#define DEBUG_MODE 0

unsigned char Flag_Recv = 0;
unsigned char len = 0;
unsigned char buf[8];
char str[20];

Button upButton = Button(30);
Button downButton = Button(32);

MCP_CAN CAN(10);                                            // Set CS to pin 10

void setup()
{
    Serial.begin(115200);

START_INIT:

    if(CAN_OK == CAN.begin(CAN_125KBPS))                   // init can bus : baudrate = 500k
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
}

void loop()
{
  unsigned char up[3] = {8,0xc0,0x0};
  unsigned char down[3] = {4,0xc0,0x0};
  
  if (upButton.getState() == 1){
    CAN.sendMsgBuf(0x21F,0,3,up);
  }
  
  if (downButton.getState() == 1){
    CAN.sendMsgBuf(0x21F,0,3,down);
  }
}

