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

#include "SparkFun_Qwiic_Twist_Arduino_Library.h"
#include "Arduino.h"

//Constructor
TWIST::TWIST()
{
}

//Initializes the sensor with basic settings
//Returns false if sensor is not detected
boolean TWIST::begin(TwoWire &wirePort, uint8_t deviceAddress)
{
  _i2cPort = &wirePort;
  _i2cPort->begin(); //This resets any setClock() the user may have done

  _deviceAddress = deviceAddress;

  if (isConnected() == false)
    return (false); //Check for sensor presence

  return (true); //We're all setup!
}

//Returns true if I2C device ack's
boolean TWIST::isConnected()
{
  _i2cPort->beginTransmission((uint8_t)_deviceAddress);
  if (_i2cPort->endTransmission() != 0)
    return (false); //Sensor did not ACK
  return (true);
}

//Change the I2C address of this address to newAddress
void TWIST::changeAddress(uint8_t newAddress)
{
  writeRegister(TWIST_CHANGE_ADDRESS, newAddress);

  //Once the address is changed, we need to change it in the library
  _deviceAddress = newAddress;
}

//Clears the moved, clicked, and pressed bits
void TWIST::clearInterrupts()
{
  writeRegister(TWIST_STATUS, 0); //Clear the moved, clicked, and pressed bits
}

//Returns the number of indents the user has twisted the knob
int16_t TWIST::getCount()
{
  return (readRegister16(TWIST_COUNT));
}

//Set the encoder count to a specific amount
boolean TWIST::setCount(int16_t amount)
{
  return (writeRegister16(TWIST_COUNT, amount));
}

//Returns the limit of allowed counts before wrapping
//0 is disabled
uint16_t TWIST::getLimit()
{
  return (readRegister16(TWIST_LIMIT));
}

//Set the encoder count limit to a specific amount
boolean TWIST::setLimit(uint16_t amount)
{
  return (writeRegister16(TWIST_LIMIT, amount));
}

//Returns the number of ticks since last check
int16_t TWIST::getDiff(boolean clearValue)
{
  int16_t difference = readRegister16(TWIST_DIFFERENCE);

  //Clear the current value if requested
  if (clearValue == true)
    writeRegister16(TWIST_DIFFERENCE, 0);

  return (difference);
}

//Returns true if button is currently being pressed
boolean TWIST::isPressed()
{
  byte status = readRegister(TWIST_STATUS);
  boolean pressed = status & (1 << statusButtonPressedBit);

  writeRegister(TWIST_STATUS, status & ~(1 << statusButtonPressedBit)); //We've read this status bit, now clear it

  return (pressed);
}

//Returns true if a click event has occurred
boolean TWIST::isClicked()
{
  byte status = readRegister(TWIST_STATUS);
  boolean clicked = status & (1 << statusButtonClickedBit);

  writeRegister(TWIST_STATUS, status & ~(1 << statusButtonClickedBit)); //We've read this status bit, now clear it

  return (clicked);
}

//Returns true if knob has been twisted
boolean TWIST::isMoved()
{
  byte status = readRegister(TWIST_STATUS);
  boolean moved = status & (1 << statusEncoderMovedBit);

  writeRegister(TWIST_STATUS, status & ~(1 << statusEncoderMovedBit)); //We've read this status bit, now clear it

  return (moved);
}

//Returns the number of milliseconds since the last encoder movement
//By default, clear the current value
uint16_t TWIST::timeSinceLastMovement(boolean clearValue)
{
  uint16_t timeElapsed = readRegister16(TWIST_LAST_ENCODER_EVENT);

  //Clear the current value if requested
  if (clearValue == true)
    writeRegister16(TWIST_LAST_ENCODER_EVENT, 0);

  return (timeElapsed);
}

//Returns the number of milliseconds since the last button event (press and release)
uint16_t TWIST::timeSinceLastPress(boolean clearValue)
{
  uint16_t timeElapsed = readRegister16(TWIST_LAST_BUTTON_EVENT);

  //Clear the current value if requested
  if (clearValue == true)
    writeRegister16(TWIST_LAST_BUTTON_EVENT, 0);

  return (timeElapsed);
}

//Sets the color of the encoder LEDs
boolean TWIST::setColor(uint8_t red, uint8_t green, uint8_t blue)
{
  return (writeRegister24(TWIST_RED, (uint32_t)red << 16 | (uint32_t)green << 8 | blue));
}

//Sets the color of a specific color
boolean TWIST::setRed(uint8_t red)
{
  return (writeRegister(TWIST_RED, red));
}
boolean TWIST::setGreen(uint8_t green)
{
  return (writeRegister(TWIST_GREEN, green));
}
boolean TWIST::setBlue(uint8_t blue)
{
  return (writeRegister(TWIST_BLUE, blue));
}

//Returns the current value of a color
uint8_t TWIST::getRed()
{
  return (readRegister(TWIST_RED));
}
uint8_t TWIST::getGreen()
{
  return (readRegister(TWIST_GREEN));
}
uint8_t TWIST::getBlue()
{
  return (readRegister(TWIST_BLUE));
}

//Returns a two byte Major/Minor version number
uint16_t TWIST::getVersion()
{
  return (readRegister16(TWIST_VERSION));
}

//Sets the relation between each color and the twisting of the knob
//Connect the LED so it changes [amount] with each encoder tick
//Negative numbers are allowed (so LED gets brighter the more you turn the encoder down)
boolean TWIST::connectColor(int16_t red, int16_t green, int16_t blue)
{
  _i2cPort->beginTransmission((uint8_t)_deviceAddress);
  _i2cPort->write(TWIST_CONNECT_RED); //Command
  _i2cPort->write(red >> 8);          //MSB
  _i2cPort->write(red & 0xFF);        //LSB
  _i2cPort->write(green >> 8);        //MSB
  _i2cPort->write(green & 0xFF);      //LSB
  _i2cPort->write(blue >> 8);         //MSB
  _i2cPort->write(blue & 0xFF);       //LSB
  if (_i2cPort->endTransmission() != 0)
    return (false); //Sensor did not ACK
  return (true);
}

boolean TWIST::connectRed(int16_t red)
{
  return (writeRegister16(TWIST_CONNECT_RED, red));
}
boolean TWIST::connectGreen(int16_t green)
{
  return (writeRegister16(TWIST_CONNECT_GREEN, green));
}
boolean TWIST::connectBlue(int16_t blue)
{
  return (writeRegister16(TWIST_CONNECT_BLUE, blue));
}

//Get the connect value for each color
int16_t TWIST::getRedConnect()
{
  return (readRegister16(TWIST_CONNECT_RED));
}
int16_t TWIST::getGreenConnect()
{
  return (readRegister16(TWIST_CONNECT_GREEN));
}
int16_t TWIST::getBlueConnect()
{
  return (readRegister16(TWIST_CONNECT_BLUE));
}

//Get number of milliseconds that elapse between end of knob turning and interrupt firing
uint16_t TWIST::getIntTimeout()
{
  return (readRegister16(TWIST_TURN_INT_TIMEOUT));
}

//Set number of milliseconds that elapse between end of knob turning and interrupt firing
boolean TWIST::setIntTimeout(uint16_t timeout)
{
  return (writeRegister16(TWIST_TURN_INT_TIMEOUT, timeout));
}

//Reads from a given location from the Twist
uint8_t TWIST::readRegister(uint8_t addr)
{
  _i2cPort->beginTransmission((uint8_t)_deviceAddress);
  _i2cPort->write(addr);
  if (_i2cPort->endTransmission() != 0)
  {
    //Serial.println("No ack!");
    return (0); //Device failed to ack
  }

  _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)1);
  if (_i2cPort->available())
  {
    return (_i2cPort->read());
  }

  //Serial.println("No ack!");
  return (0); //Device failed to respond
}

//Reads an int from a given location from the Twist
int16_t TWIST::readRegister16(uint8_t addr)
{
  _i2cPort->beginTransmission((uint8_t)_deviceAddress);
  _i2cPort->write(addr);
  if (_i2cPort->endTransmission() != 0)
  {
    //Serial.println("No ack!");
    return (0); //Device failed to ack
  }

  _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)2);
  if (_i2cPort->available())
  {
    uint8_t lsb = _i2cPort->read();
    uint8_t msb = _i2cPort->read();
    return ((int16_t)msb << 8 | lsb);
  }

  //Serial.println("No ack!");
  return (0); //Device failed to respond
}

//Write a byte value to a spot in the Twist
boolean TWIST::writeRegister(uint8_t addr, uint8_t val)
{
  _i2cPort->beginTransmission((uint8_t)_deviceAddress);
  _i2cPort->write(addr);
  _i2cPort->write(val);
  if (_i2cPort->endTransmission() != 0)
  {
    //Serial.println("No ack!");
    return (false); //Device failed to ack
  }

  return (true);
}

//Write a 2 byte value to a spot in the Twist
boolean TWIST::writeRegister16(uint8_t addr, uint16_t val)
{
  _i2cPort->beginTransmission((uint8_t)_deviceAddress);
  _i2cPort->write(addr);
  _i2cPort->write(val & 0xFF); //LSB
  _i2cPort->write(val >> 8);   //MSB
  if (_i2cPort->endTransmission() != 0)
  {
    //Serial.println("No ack!");
    return (false); //Device failed to ack
  }

  return (true);
}

//Write a 3 byte value to a spot in the Twist
boolean TWIST::writeRegister24(uint8_t addr, uint32_t val)
{
  _i2cPort->beginTransmission((uint8_t)_deviceAddress);
  _i2cPort->write(addr);
  _i2cPort->write(val >> 16);  //MSB
  _i2cPort->write(val >> 8);   //MidMSB
  _i2cPort->write(val & 0xFF); //LSB
  if (_i2cPort->endTransmission() != 0)
  {
    //Serial.println("No ack!");
    return (false); //Device failed to ack
  }

  return (true);
}
