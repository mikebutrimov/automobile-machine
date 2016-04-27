


volatile long t0 = 0; // our magic timer
volatile long t1 = 0; // yet another magic timer
volatile byte bufferbyte[13],buffer[13];
volatile uint8_t byteindex=0,bitindex = 0;
volatile byte ololo[13];
const int AINET = 13;
uint8_t i,j,type;
byte b;
const byte VOL_LEN=36;
uint8_t vol[VOL_LEN]={0x99,0x78,0x68,0x60,0x55,0x50,0x48,0x46,0x44,0x42,
                      0x40,0x38,0x36,0x34,0x32,0x30,0x28,0x26,0x24,0x22,
                      0x20,0x18,0x16,0x14,0x12,0x10,0x09,0x08,0x07,0x06,
                      0x05,0x04,0x03,0x02,0x01,0x00};

                      

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



void sendAiPacket(byte * packet){
  //SOF
  digitalWrite(AINET, HIGH);
  delayMicroseconds(30);
  digitalWrite(AINET, LOW);
  delayMicroseconds(14);
    for (i=0;i<11;i++) {
    for (j=0;j<8;j++) {
      type=(packet[i] & (1 << (7-j))) >> (7-j);
      if (type==0) {
        digitalWrite(AINET, HIGH);
        delayMicroseconds(14);
        digitalWrite(AINET, LOW);
        delayMicroseconds(6);
      } else {
        digitalWrite(AINET, HIGH);
        delayMicroseconds(6);
        digitalWrite(AINET, LOW);
        delayMicroseconds(13);
      }
    }
   }
}


void volUp(){
  byte packet[11] = {0x40,0x02,0xD2,0x99,0x00,0x00,0x00,0x00,0x00,0x00,0xD7};
  //packet[3] = vol[1];
  //crc(packet);
  sendAiPacket(packet);
}


void ISR_read(){
  t0 = micros();
  //detect rising or falling
  if (digitalRead(2)){
    t1 = micros();
  }
  else {
    //falling
    if (t0 -t1 < 8 ){// logical 1
      buffer[byteindex] |= 1 << (7-bitindex);
      if (++bitindex > 7) {
       bitindex=0;
       byteindex++;
      }
    }
    else if (t0 -t1 < 16){// logical 0
      buffer[byteindex]&=~(1 << (7-bitindex));
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
  if ((byteindex==12) && (bitindex==0)) {
      byteindex=0;
      bitindex=0;
    }
  if ((byteindex==11) && (bitindex==0)){
    if (buffer[0] == 0x02){
    //maybe time to send ack
    delayMicroseconds(33); 
      for (j=0;j<8;j++) {
        type=(buffer[0] & (1 << (7-j))) >> (7-j);
          if (type==0) {
            digitalWrite(AINET, HIGH);
            delayMicroseconds(14);
            digitalWrite(AINET, LOW);
            delayMicroseconds(6);
          } else {
            digitalWrite(AINET, HIGH);
            delayMicroseconds(6);
            digitalWrite(AINET, LOW);
            delayMicroseconds(13);
          }
        }   
      }
    }
   }
  }
 




void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(AINET, OUTPUT); 
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  attachInterrupt(digitalPinToInterrupt(2), ISR_read, CHANGE);
  

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(12, LOW);
  noInterrupts();
  volUp();
  interrupts();
  for (int i = 0; i <13; i++){
    Serial.print(buffer[i],HEX);
    Serial.print(" ");
  }

  Serial.println();
  delay(1000);
}
