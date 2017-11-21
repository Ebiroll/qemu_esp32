#include "Adafruit_HT1632.h"

#define HT_DATA 22
#define HT_WR   21
#define HT_CS   23
#define HT_CS2  18

// use this line for single matrix
Adafruit_HT1632LEDMatrix matrix = Adafruit_HT1632LEDMatrix(HT_DATA, HT_WR, HT_CS);
// use this line for two matrices!
//Adafruit_HT1632LEDMatrix matrix = Adafruit_HT1632LEDMatrix(HT_DATA, HT_WR, HT_CS, HT_CS2);

void setup() {
  //Serial.begin(9600);
  Serial.begin(115200);
  matrix.begin(ADA_HT1632_COMMON_16NMOS);
  //matrix.begin(ADA_HT1632_COMMON_8NMOS);

  matrix.fillScreen();
  delay(500);
  matrix.clearScreen();
  matrix.setTextWrap(false);
}

void loop() {

  matrix.setRotation(2);

  for(uint8_t i=0; i<32; i++) {
    //matrix.setRotation(i);
    matrix.clearScreen();
    matrix.setCursor(24-i, 0);
    matrix.print("Hello");
    matrix.writeScreen();
    //matrix.dumpScreen();
    delay(300);
    int ix=0;

    for (ix=0; ix<256; ix++) {
        matrix.simplePixel(ix,1);
        matrix.writeScreen();
        delay(20);
        Serial.printf("%d\r\n",ix);
        if (ix%8==3) {
            delay(500);      
        }
    }

    for (ix=0; ix<256; ix++) {
        matrix.simplePixel(ix,0);
        matrix.writeScreen();
        
    }

    // blink!
    //matrix.blink(true);
    //delay(2000);
    //matrix.blink(false);
/*
    // Adjust the brightness down
    for (int8_t i=15; i>=0; i--) {
      matrix.setBrightness(i);
      delay(100);
    }
    // then back up
    for (uint8_t i=0; i<16; i++) {
      matrix.setBrightness(i);
      delay(100);
    }
*/
/*

  // draw an 'X' in red
  matrix.clearScreen();
  matrix.drawLine(0, 0, matrix.width()-1, matrix.height()-1, 1);
  matrix.drawLine(matrix.width()-1, 0, 0, matrix.height()-1, 1);
  matrix.writeScreen();
  delay(500);
  // fill a circle
  matrix.fillCircle(matrix.width()/2-1, matrix.height()/2-1, 7, 1);
  matrix.writeScreen();
  delay(500);

  // draw an inverted circle
  matrix.drawCircle(matrix.width()/2-1, matrix.height()/2-1, 4, 0);
  matrix.writeScreen();
  delay(500);
*/
# if 0
    for (uint8_t y=0; y<matrix.height(); y++) {
      for (uint8_t x=0; x< matrix.width(); x++) {
        matrix.setPixel(x, y);
        matrix.writeScreen();
        delay(20);
      }
    }


    for (uint8_t y=0; y<matrix.height(); y++) {
      for (uint8_t x=0; x< matrix.width(); x++) {
        matrix.clrPixel(x, y);
        matrix.writeScreen();
        delay(20);
      }
    }
  #endif
    
  }
}

#if 0
//

#include "Arduino.h"

// HT1632C PINs

#define DISPLAY_CS    23
#define DISPLAY_WR    21
#define DISPLAY_DATA  22


// HT1632C Commands

#define HT1632C_READ    B00000110
#define HT1632C_WRITE   B00000101
#define HT1632C_CMD     B00000100

#define HT1632_CMD_SYSON  0x01
#define HT1632_CMD_LEDON  0x03

void ht1632c_send_command(byte command);
void ht1632c_send_bits(byte bits, byte firstbit);

// ----------------------------------------
//  HT1632C functions
// ----------------------------------------

void ht1632c_send_command(byte command) {
  
  digitalWrite(DISPLAY_CS, LOW);  
  ht1632c_send_bits(HT1632C_CMD, 1 << 2);
  ht1632c_send_bits(command, 1 << 7);
  ht1632c_send_bits(0, 1);
  digitalWrite(DISPLAY_CS, HIGH); 
}

void ht1632c_send_bits(byte bits, byte firstbit) {
  
  while(firstbit) {
    digitalWrite(DISPLAY_WR, LOW);
    if (bits & firstbit) digitalWrite(DISPLAY_DATA, HIGH);
    else digitalWrite(DISPLAY_DATA, LOW);
    digitalWrite(DISPLAY_WR, HIGH);
    firstbit >>= 1;
  }
}


int j;

void setup() {

  Serial.begin(115200);

  // All PINs are output
  pinMode(DISPLAY_CS, OUTPUT);
  pinMode(DISPLAY_WR, OUTPUT);
  pinMode(DISPLAY_DATA, OUTPUT);
  
  // Enable System oscillator and LED duty cycle generator
  ht1632c_send_command(HT1632_CMD_SYSON);
  ht1632c_send_command(HT1632_CMD_LEDON);
  
  j = 0;
}

void loop() {

  // select display
  digitalWrite(DISPLAY_CS, LOW);  
 
  // send WRITE command
  ht1632c_send_bits(HT1632C_WRITE, 1 << 2);
  
  // send start address, 00h
  ht1632c_send_bits(0x00, 1 << 6);

  // send data
  for(int i = 0; i < 256; i++) {
    digitalWrite(DISPLAY_WR, LOW);
    if(i == j) digitalWrite(DISPLAY_DATA, HIGH);
    else digitalWrite(DISPLAY_DATA, LOW);
    digitalWrite(DISPLAY_WR, HIGH);    
  }
  
  // unselect display
  digitalWrite(DISPLAY_CS, HIGH);  

  // cycle LED position to be turned on
  if(j < 256 ) j++;
  else j = 0;
  
  delay(20);
}
#endif
