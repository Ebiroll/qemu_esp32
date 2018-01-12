#include <Streaming.h>

void setup()
{
  Serial.begin(9600);
  int lettera = 'A';
  int month = 4, day = 17, year = 2009;
  
  Serial << "This is an example of the new streaming" << endl;
  Serial << "library.  This allows you to print variables" << endl;
  Serial << "and strings without having to type line after" << endl;
  Serial << "line of Serial.print() calls.  Examples: " << endl;
  
  Serial << "A is " << lettera << "." << endl;
  Serial << "The current date is " << day << "-" << month << "-" << year << "." << endl;
  
  Serial << "You can use modifiers too, for example:" << endl;
  Serial << _BYTE(lettera) << " is " << _HEX(lettera) << " in hex. " << endl;
}

void loop()
{}
