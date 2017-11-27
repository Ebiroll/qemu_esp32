To build

cd components && \
git clone https://github.com/espressif/arduino-esp32.git arduino && \
cd .. && \
make menuconfig

By using a heltec lora module, similar to this
https://hackaday.io/project/27791-esp32-lora-oled-module


I was able to send a join requests to the things network.
https://console.thethingsnetwork.org/
This is from the gateway console log.

This is the gateway software used
https://github.com/bokse001/dual_chan_pkt_fwd



Join Request

Dev EUI
1234567812345678
App EUI
70B3D57ED00085BE

Physical Payload
00BE8500D07ED5B37078563412785634125F121D23766D
Event Data


{
  "gw_id": "eui-b827ebffff79bb75",
  "payload": "AL6FANB+1bNweFY0EnhWNBJfEh0jdm0=",
  "dev_eui": "1234567812345678",
  "lora": {
    "spreading_factor": 7,
    "bandwidth": 125,
    "air_time": 61696000
  },
  "coding_rate": "4/5",
  "timestamp": "2017-11-27T22:13:38.150Z",
  "rssi": -68,
  "snr": 9,
  "app_eui": "70B3D57ED00085BE",
  "frequency": 868100000
}


The payload is base64 encoded and is,
0x00 0xBE 0x85 0x00 0xD0 0x7E 0xD5 0xB3 0x70 0x78 0x56 0x34 0x12 0x78 0x56 0x34 0x12 0x5F 0x12 0x1D 0x23 0x76 0x6D
