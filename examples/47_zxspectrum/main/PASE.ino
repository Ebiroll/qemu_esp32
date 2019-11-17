
// ____________________________________________________
// Pase Spectrum Emulator (KOGEL esp32)
// Pete Todd 2017
//     You are not allowed to distribute this software commercially     
//     Please, notify me, if you make any changes to this file        
// ____________________________________________________

#include <bt.h>
#include <esp_wifi.h>
#include "PS2Kbd.h"
#include "SPI.h"
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include "FS.h"
#include "SD.h"
#include "paledefs.h"

// ________________________________________________
// SWITCHES
//
//#define WIFI_ENABLED
#define SD_ENABLED


//#define DEBUG
#define VIDEO_LCD_DUALCORE
//#define VIDEO_LCD_SINGLECORE
//#define VIDEO_SERIAL

bool run_debug = false;


// ____________________________________________________________________
// PINOUT DEFINITIONS
//
// LCD - ILI9341 based - For the Adafruit shield, these are the default.
#define TFT_DC 2
#define TFT_CS 5
#define LEDPIN 4  // LCD backlight LED

//PS2 Keyboard
//
#define KEYBOARD_DATA 32
#define KEYBOARD_CLK  33


// ____________________________________________________________________
// INSTANTS
//
// WiFi
//
#ifdef WIFI_ENABLED
//how many clients should be able to telnet to this ESP32
  #define MAX_SRV_CLIENTS 1
  const char* ssid = "*****";
  const char* password = "*****";

  WiFiMulti wifiMulti;
  WiFiServer server(23);
  WiFiClient serverClients[MAX_SRV_CLIENTS];
#endif

PS2Kbd keyboard(KEYBOARD_DATA, KEYBOARD_CLK);

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ________________________________________________________________________
// EXTERNS + GLOBALS
//
unsigned Z80_GetPC (void);         /* Get program counter                   */
void Z80_Reset (void);             /* Reset registers to the initial values */
unsigned int  Z80_Execute ();           /* Execute IPeriod T-States              */

void pump_key(char k);

byte Z80_RDMEM(uint16_t A);
void Z80_WRMEM(uint16_t A,byte V);
extern byte bank_latch;
extern int start_im1_irq;

File myFile;

// ________________________________________________________________________
// GLOBALS
//
byte *bank1;
byte *bank2;
byte *bank3;
uint16_t *tftmem;
byte z80ports_in[32];





// SETUP *************************************
// SETUP *************************************
// SETUP *************************************

void setup()
{
  // Turn off peripherals to gain memory (?do they release properly)
  esp_bt_controller_deinit();
  esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
//  esp_wifi_set_mode(WIFI_MODE_NULL);
  
  pinMode(LEDPIN,OUTPUT);
  digitalWrite(LEDPIN,HIGH);

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  keyboard.begin();

  
  Serial.begin(115200);

#ifdef WIFI_ENABLED
      // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    //test_wifi();
    setup_wifi_telnet_server();
#endif

#ifdef SD_ENABLED
    test_sd();
#endif

// ALLOCATE MEMORY
//
  bank1 = (byte *)malloc(65536);
  if(bank1 == NULL)Serial.println("Failed to allocate Bank 1 memory");

  tftmem = (uint16_t *)malloc(8192); // needs 8192 - malloc is in bytes
  if(tftmem == NULL)Serial.println("Failed to allocate tft memory");

// START Z80
// START Z80
// START Z80
  Serial.println("RESETTING Z80");
  Z80_Reset(); 

  // make sure keyboard ports are FF
  for(int t = 0;t < 32;t++)
  {
    z80ports_in[t] = 0xff;
  }

  Serial.print("Setup: MAIN Executing on core ");
  Serial.println(xPortGetCoreID());
  Serial.print("Free Heap: ");
  Serial.println(system_get_free_heap_size());

#ifdef VIDEO_LCD_DUALCORE

  xTaskCreatePinnedToCore(
                    lcdTask,   /* Function to implement the task */
                    "lcdTask", /* Name of the task */
                    16384,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    0,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    0);  /* Core where the task should run */
                          
#endif
 
}



#ifdef VIDEO_LCD_DUALCORE

// VIDEO core 0 *************************************
// VIDEO core 0 *************************************
// VIDEO core 0 *************************************

void lcdTask( void * parameter )
{
   // vTaskEndScheduler ();
   while(1)
    {
      // portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
      // taskENTER_CRITICAL(&myMutex);
      // vTaskDelay(1) ;
     
      lcd_doscreen();
    
      // taskEXIT_CRITICAL(&myMutex);
    }
}
#endif


// SPECTRUM SCREEN DISPLAY
// SPECTRUM SCREEN DISPLAY
// SPECTRUM SCREEN DISPLAY
//
/* Calculate Y coordinate (0-192) from Spectrum screen memory location */
int calcY(int offset){
  return ((offset>>11)<<6)        // sector start
       +((offset%2048)>>8)        // pixel rows
       +((((offset%2048)>>5)-((offset%2048)>>8<<3))<<3);  // character rows
}

/* Calculate X coordinate (0-255) from Spectrum screen memory location */
int calcX(int offset){
  return (offset%32)<<3;
}

void lcd_doscreen()
{
  unsigned int ff,i,byte_offset;
  unsigned char color_attrib,pixel_map,zx_fore_color,zx_back_color;
  unsigned int zx_vidcalc;

    for(unsigned int lin = 0;lin < 192;lin++)
    {
      for(ff=0;ff<32;ff++)  //foreach byte in line
      {
        byte_offset=lin*32+ff;//*2+1;
        //spectrum attributes
        //bit 7 6   5 4 3   2 1 0
        //    F B   P A P   I N K
        color_attrib=bank1[0x5800+(calcY(byte_offset)/8)*32+ff];//get 1 of 768 attrib values
        pixel_map=bank1[0x4000+byte_offset];
        zx_fore_color=color_attrib & 0x07;
        zx_back_color=(color_attrib & 0x38)>>3;

        for(i=0;i<8;i++)  //foreach pixel within a byte
        {
            zx_vidcalc=ff*8+i;
              
            tftmem[zx_vidcalc] = 0;
            byte bitpos = (0x80 >> i);
            if((pixel_map & bitpos)!=0)
            {
              if((zx_fore_color & 0x01)!= 0)
                tftmem[zx_vidcalc] |=0x1e;              
              if((zx_fore_color & 0x02)!= 0)
                tftmem[zx_vidcalc] |=0xf800;              
              if((zx_fore_color & 0x04)!= 0)
                tftmem[zx_vidcalc] |=0x7e0;              
            }
            else
            {
              if((zx_back_color & 0x01)!= 0)
                tftmem[zx_vidcalc] |=0x1e;              
              if((zx_back_color & 0x02)!= 0)
                tftmem[zx_vidcalc] |=0xf800;              
              if((zx_back_color & 0x04)!= 0)
                tftmem[zx_vidcalc] |=0x7e0;              
            }            
        }
      }
        tft.startWrite();
        tft.setAddrWindow(0, calcY(lin*32), 256, 1);   
        tft.writePixels(tftmem, 256);    // 256 pix * 16 lines
        tft.endWrite();
                  
    }

}


// LOOP core 1 *************************************
// LOOP core 1 *************************************
// LOOP core 1 *************************************

void loop() 
{
  while (1) 
  { 
        if (keyboard.available())
        {
            char t = keyboard.read();
// Do modifiers
if (keyboard.getModifiers() & SHIFT) z80ports_in[0] &= 0x7F;
if (keyboard.getModifiers() & ALT) z80ports_in[2] &= 0xbf;  // symbol shift
            if(t == '\n')t = 13;
            pump_key(t);
           // Serial.println(t,HEX);
        }    

        if(Serial.available() > 0)
        {
          char inByte = Serial.read();
//          Serial.print("Key was ");
//          Serial.println(inByte);
          pump_key(inByte);
#ifdef DEBUG          
          if(inByte == 'd')
            run_debug = true;
          if(inByte == 'e')
            run_debug = false;
#endif     
        }

#ifdef WIFI_ENABLED
        wifi_telnet_server();
#endif

        
        Z80_Execute();

        start_im1_irq=1;    // keyboard scan is run in IM1 interrupt
   
#ifdef VIDEO_SERIAL
        do_vidserial();
#endif

#ifdef VIDEO_LCD_SINGLECORE
        lcd_doscreen();
#endif
      
        vTaskDelay(20) ;  //important to avoid task watchdog timeouts - change this to slow down emu
    
  } 
}

#ifdef VIDEO_SERIAL
void do_vidserial()
{
         uint16_t vidad;
         int startline = 0;
         for(int lineno = startline; lineno < startline + 48; lineno++)
         {
         // uint16_t vstart =  0xa460 + lineno*32;
          uint16_t vstart =  0xa000 + lineno*32;
          uint16_t vend = vstart + 6;
          for(vidad = vstart; vidad < vend; vidad++)
          {
              char thebyt = bank2[vidad - 0xa000];
              for(int t = 0; t < 8; t++)
              {
                if((thebyt & (0x80 >> t)) == 0)
                  Serial.write(' ');
                else
                  Serial.write('X');
              }
          }
          Serial.println();
         }
}
#endif





#ifdef SD_ENABLED
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void test_sd()
{
      if(!SD.begin(16, SPI, 1000000)){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
 
 
  // re-open the file for reading:
  myFile = SD.open("/AirRaid.tap", FILE_READ);
  if (myFile) {
    Serial.println("COMMAND.TXT:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening COMMAND.TXT");
  }

listDir(SD, "/", 0);

}
#endif

#ifdef WIFI_ENABLED
void test_wifi()
{
    Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
        }
    }
    Serial.println("");

}

void setup_wifi_telnet_server()
{
  wifiMulti.addAP(ssid, password);
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting Wifi ");
  for (int loops = 10; loops > 0; loops--) {
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("WiFi connected ");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    }
    else {
      Serial.println(loops);
      delay(1000);
    }
  }
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi connect failed");
    delay(1000);
    ESP.restart();
  }

  //start UART and the server
  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");
}

void wifi_telnet_server()
{
    uint8_t i;
  if (wifiMulti.run() == WL_CONNECTED) {
    //check if there are any new clients
    if (server.hasClient()){
      for(i = 0; i < MAX_SRV_CLIENTS; i++){
        //find free/disconnected spot
        if (!serverClients[i] || !serverClients[i].connected()){
          if(serverClients[i]) serverClients[i].stop();
          serverClients[i] = server.available();
          if (!serverClients[i]) Serial.println("available broken");
          Serial.print("New client: ");
          Serial.print(i); Serial.print(' ');
          Serial.println(serverClients[i].remoteIP());
          break;
        }
      }
      if (i >= MAX_SRV_CLIENTS) {
        //no free/disconnected spot so reject
        server.available().stop();
      }
    }
    //check clients for data
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
        if(serverClients[i].available()){
          //get data from the telnet client and push it to the UART
          while(serverClients[i].available()) pump_key(serverClients[i].read());
        }
      }
      else {
        if (serverClients[i]) {
          serverClients[i].stop();
        }
      }
    }
    //check UART for data
//    if(Serial1.available()){
//      size_t len = Serial1.available();
//      uint8_t sbuf[len];
//      Serial1.readBytes(sbuf, len);
//      //push UART data to all connected telnet clients
//      for(i = 0; i < MAX_SRV_CLIENTS; i++){
//        if (serverClients[i] && serverClients[i].connected()){
//          serverClients[i].write(sbuf, len);
//          delay(1);
//        }
//      }
//    }
  }
  else {
    Serial.println("WiFi not connected!");
    for(i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i]) serverClients[i].stop();
    }
    delay(100);
  }
}
#endif



