# Lmic under esp-idf

https://github.com/matthijskooijman/arduino-lmic


https://www.lora-alliance.org/lorawan-for-developers

With arduino, esp32
https://github.com/PiAir/esp32-sx1276


LMIC was implemented before LoRaWAN specification existed, there are some things to fix, for example:
Remove the 864.x MHz join channels deprecated in LoRaWAN 1.0.2
Remove the Re-Join FTYPE. in LoRaWAN 1.0.2 this field is RFU althoug in LoRaWAN 1.1 Re-Join will be implemented

Things to do to get started:

Fix implementation to be LoRaWAN 1.0.2 specification compatible.
Add Compliance Test code to be able to certificate under LoRaWAN 1.0.2 specification
Add Class-C support


https://github.com/matthijskooijman/arduino-lmic/commit/6e098863f4a59131ab773c6314a51cd7323a519d


https://github.com/TheThingsNetwork/arduino-device-lib


https://github.com/berkutta/esp32_lmic

https://github.com/Benjamin1231/esp-idf-lmic/tree/master/lmic

https://www.thethingsnetwork.org/wiki/LoRaWAN/Address-Space#address-space-in-lorawan_devices_prefix-assignments
