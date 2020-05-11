/*
  This is a library written for the SparkFun Qwiic Twist Encoder
  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/15083

  Written by Nathan Seidle @ SparkFun Electronics, November 25th, 2018

  The Qwiic Twist is a I2C controlled encoder

  https://github.com/sparkfun/SparkFun_Qwiic_Twist_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.7

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SPARKFUN_QWIIC_TWIST_ARDUINO_LIBRARY_H
#define _SPARKFUN_QWIIC_TWIST_ARDUINO_LIBRARY_H
#include "Arduino.h"
#include "Wire.h"

#define QWIIC_TWIST_ADDR 0x3F //7-bit unshifted default I2C Address

const byte statusButtonClickedBit = 2;
const byte statusButtonPressedBit = 1;
const byte statusEncoderMovedBit = 0;

const byte enableInterruptButtonBit = 1;
const byte enableInterruptEncoderBit = 0;

//Map to the various registers on the Twist
enum encoderRegisters
{
  TWIST_ID = 0x00,
  TWIST_STATUS = 0x01, //2 - button clicked, 1 - button pressed, 0 - encoder moved
  TWIST_VERSION = 0x02,
  TWIST_ENABLE_INTS = 0x04, //1 - button interrupt, 0 - encoder interrupt
  TWIST_COUNT = 0x05,
  TWIST_DIFFERENCE = 0x07,
  TWIST_LAST_ENCODER_EVENT = 0x09, //Millis since last movement of knob
  TWIST_LAST_BUTTON_EVENT = 0x0B,  //Millis since last press/release

  TWIST_RED = 0x0D,
  TWIST_GREEN = 0x0E,
  TWIST_BLUE = 0x0F,

  TWIST_CONNECT_RED = 0x10, //Amount to change red LED for each encoder tick
  TWIST_CONNECT_GREEN = 0x12,
  TWIST_CONNECT_BLUE = 0x14,

  TWIST_TURN_INT_TIMEOUT = 0x16,
  TWIST_CHANGE_ADDRESS = 0x18,
  TWIST_LIMIT = 0x19,
};

class TWIST
{
public:
  TWIST();

  boolean begin(TwoWire &wirePort = Wire, uint8_t deviceAddress = QWIIC_TWIST_ADDR);
  boolean isConnected(); //Checks if sensor ack's the I2C request

  int16_t getCount();                                        //Returns the number of indents the user has turned the knob
  boolean setCount(int16_t amount);                          //Set the number of indents to a given amount
  int16_t getDiff(boolean clearValue = true);                //Returns the number of ticks since last check
  boolean isMoved();                                         //Returns true if knob has been twisted
  boolean isPressed();                                       //Return true if button is currently pressed.
  boolean isClicked();                                       //Returns true if a click event has occurred. Event flag is then reset.
  uint16_t timeSinceLastMovement(boolean clearValue = true); //Returns the number of milliseconds since the last encoder movement
  uint16_t timeSinceLastPress(boolean clearValue = true);    //Returns the number of milliseconds since the last button event (press and release)
  void clearInterrupts();                                    //Clears the moved and clicked bits

  boolean setColor(uint8_t red, uint8_t green, uint8_t blue); //Sets the color of the encoder LEDs, 0-255
  boolean setRed(uint8_t);                                    //Set the red LED, 0-255
  boolean setGreen(uint8_t);                                  //Set the green LED, 0-255
  boolean setBlue(uint8_t);                                   //Set the blue LED, 0-255
  uint8_t getRed();                                           //Get current value
  uint8_t getGreen();                                         //Get current value
  uint8_t getBlue();                                          //Get current value

  //'Connect' commands Set the relation between each color and the twisting of the knob
  //This will connect the LED so it changes [amount] with each encoder tick
  //Negative numbers are allowed (so LED gets brighter the more you turn the encoder down)
  boolean connectColor(int16_t red, int16_t green, int16_t blue); //Connect all colors in one command
  boolean connectRed(int16_t);                                    //Connect individual colors
  boolean connectGreen(int16_t);                                  //Connect individual colors
  boolean connectBlue(int16_t);                                   //Connect individual colors
  int16_t getRedConnect();                                        //Get the connect value for each color
  int16_t getGreenConnect();
  int16_t getBlueConnect();

  boolean setLimit(uint16_t); //Set the limit of what the encoder will go to, then wrap to 0. Set to 0 to disable.
  uint16_t getLimit();

  uint16_t getIntTimeout();                //Get number of milliseconds that must elapse between end of knob turning and interrupt firing
  boolean setIntTimeout(uint16_t timeout); //Set number of milliseconds that elapse between end of knob turning and interrupt firing

  uint16_t getVersion(); //Returns a two byte Major/Minor version number

  void changeAddress(uint8_t newAddress); //Change the I2C address to newAddress

private:
  TwoWire *_i2cPort;
  uint8_t _deviceAddress;

  uint8_t readRegister(uint8_t addr);
  int16_t readRegister16(uint8_t addr);

  boolean writeRegister(uint8_t addr, uint8_t val);
  boolean writeRegister16(uint8_t addr, uint16_t val);
  boolean writeRegister24(uint8_t addr, uint32_t val);
};

#endif