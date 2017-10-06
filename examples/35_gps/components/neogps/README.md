NeoGPS
======
https://github.com/SlashDevin/NeoGPS
More useful info,
http://www.maartenlamers.com/nmea/

This fully-configurable Arduino library uses _**minimal**_ RAM, PROGMEM and CPU time, 
requiring as few as _**10 bytes of RAM**_, **866 bytes of PROGMEM**, and **less than 1mS of CPU time** per sentence.  

It supports the following protocols and messages:

#### NMEA 0183
* GPGGA - System fix data
* GPGLL - Geographic Latitude and Longitude
* GPGSA - DOP and active satellites
* GPGST - Pseudo Range Error Statistics
* GPGSV - Satellites in View
* GPRMC - Recommended Minimum specific GPS/Transit data
* GPVTG - Course over ground and Ground speed
* GPZDA - UTC Time and Date

The "GP" prefix usually indicates an original [GPS](https://en.wikipedia.org/wiki/Satellite_navigation#GPS) source.  NeoGPS parses *all* Talker IDs, including
  * "GL" ([GLONASS](https://en.wikipedia.org/wiki/Satellite_navigation#GLONASS)),
  * "BD" or "GB" ([BeiDou](https://en.wikipedia.org/wiki/Satellite_navigation#BeiDou)),
  * "GA" ([Galileo](https://en.wikipedia.org/wiki/Satellite_navigation#Galileo)), and
  * "GN" (mixed)

This means that GLRMC, GBRMC or BDRMC, GARMC and GNRMC from the latest GPS devices (e.g., ublox M8N) will also be correctly parsed.  See discussion of Talker IDs in [Configurations](extras/doc/Configurations.md#enabledisable-the-talker-id-and-manufacturer-id-processing).

Most applications can be fully implemented with the standard NMEA messages above.  They are supported by almost all GPS manufacturers.  Additional messages can be added through derived classes.

Most applications will use this simple, familiar loop structure:
```
NMEAGPS gps;
gps_fix fix;

void loop()
{
  while (gps.available( gps_port )) {
    fix = gps.read();
    doSomeWork( fix );
  }
}
```
