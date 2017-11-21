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
  matrix.fillScreen();
  delay(500);
  matrix.clearScreen();
  matrix.setTextWrap(false);
}

void loop() {


  for(uint8_t i=0; i<4; i++) {
    matrix.setRotation(i);
    matrix.clearScreen();
    matrix.setCursor(0, 0);
    matrix.print("Hello");
    matrix.writeScreen();
    delay(1000);

    for (uint8_t y=0; y<matrix.height(); y++) {
      for (uint8_t x=0; x< matrix.width(); x++) {
        matrix.setPixel(x, y);
        matrix.writeScreen();
      }
    }
    // blink!
    matrix.blink(true);
    delay(2000);
    matrix.blink(false);

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

    for (uint8_t y=0; y<matrix.height(); y++) {
      for (uint8_t x=0; x< matrix.width(); x++) {
        matrix.clrPixel(x, y);
        matrix.writeScreen();
      }
    }
  }
}
