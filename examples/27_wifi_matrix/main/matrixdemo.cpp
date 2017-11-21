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

  #if 0
    for (ix=0; ix<8*8*4*2; ix++) {
        matrix.simplePixel(ix,1);
        matrix.writeScreen();
        delay(20);
        Serial.printf("%d\r\n",ix);
        if (ix%8==3) {
            delay(500);      
        }
    }

    for (ix=0; ix<8*8*4*2; ix++) {
        matrix.simplePixel(ix,0);
        matrix.writeScreen();
        
    }
  #endif

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
