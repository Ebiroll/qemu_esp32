/* Adapted from Sermus' project : https://github.com/Sermus/ESP8266_Adafruit_ILI9341*/

#include <Adafruit_GFX_AS.h>
#include <Adafruit_ILI9341_fast_as.h>

#include "time.h"
#include <sys/time.h>

extern "C" {
#include "mini-printf.h"
}

//#define TARGETTEMPSCREEN

#define VGA_BLACK		0x0000
#define VGA_WHITE		0xFFFF
#define VGA_RED			0xF800
#define VGA_GREEN		0x0400
#define VGA_BLUE		0x001F
#define VGA_SILVER		0xC618
#define VGA_GRAY		0x8410
#define VGA_MAROON		0x8000
#define VGA_YELLOW		0xFFE0
#define VGA_OLIVE		0x8400
#define VGA_LIME		0x07E0
#define VGA_AQUA		0x07FF
#define VGA_TEAL		0x0410
#define VGA_NAVY		0x0010
#define VGA_FUCHSIA		0xF81F
#define VGA_PURPLE		0x8010

extern Adafruit_ILI9341 tft;

extern float target_room_temperature;
extern float RW_temperature;
extern float target_RW_temperature;
extern float room1_temperature;
extern float room2_temperature;
extern float outside_temperature;
extern float min_target_temp;
extern float max_target_temp;
extern bool heater_enabled;
extern time_t room1_updated;
extern time_t room2_updated;
extern time_t outside_updated;
extern time_t heater_state_changed_time;
extern time_t total_on_time;
extern time_t total_off_time;
extern time_t last24h_on_time;
extern time_t last24h_off_time;
extern time_t cursecs;

time_t start_time;

#define HEADERTEXT 2
#define LINETEXT 2
#define BIGNUM 7

int color(uint8_t r, uint8_t g, uint8_t b)
{
	return ((r&248)|g>>5) << 8 | ((g&28)<<3|b>>3);
}

int drawPlaceholder(int x, int y, int width, int height, int bordercolor, const char* headertext)
{
	int headersize = 18;
	tft.drawRoundRect(x, y, width, height, 3, bordercolor);
	tft.drawCentreString(headertext, x + width / 2, y + 1, HEADERTEXT);
	tft.drawFastHLine(x, y + headersize, width, bordercolor);
	
	return y + headersize;
}

const char* kidroom = "Kidroom";
const char* bedroom = "Bedroom";
const char* outside = "Outside";

void drawWireFrame()
{
	tft.setTextColor(VGA_SILVER, VGA_BLACK);
	//Target placeholder
	drawPlaceholder(0, 0, 133, 78, VGA_RED, "TARGET");
	
	//Temperatures placeholder
	int placeholderbody = drawPlaceholder(135, 0, 184, 78, VGA_GREEN, "AIR");
	tft.drawString(kidroom, 140, placeholderbody + 2, LINETEXT);
	tft.drawString(bedroom, 140, placeholderbody + 22, LINETEXT);
	tft.drawString(outside, 140, placeholderbody + 42, LINETEXT);

	//Water temperatures placeholder
	placeholderbody = drawPlaceholder(135, 80, 184, 60, VGA_PURPLE, "WATER");
	tft.drawString("Current", 140, placeholderbody + 2, LINETEXT);
	tft.drawString("Target", 140, placeholderbody + 22, LINETEXT);

	//Heater icon placeholder
	drawPlaceholder(0, 80, 133, 60, VGA_RED, "HEATER");

	//Status placeholder
	placeholderbody = drawPlaceholder(0, 142, 319, 97, VGA_WHITE, "UPDATED (secs ago)");
	tft.drawString(kidroom, 6, placeholderbody + 2, LINETEXT);
	tft.drawString(bedroom, 6, placeholderbody + 22, LINETEXT);
	tft.drawString(outside, 6, placeholderbody + 42, LINETEXT);
	tft.drawString("Heater state", 160, placeholderbody + 2, LINETEXT);
	tft.drawString("Uptime", 160, placeholderbody + 22, LINETEXT);
	tft.drawString("OnPercentage", 160, placeholderbody + 42, LINETEXT);
	tft.drawString("24hOn", 160, placeholderbody + 62, LINETEXT);
}

void drawTemperatures()
{
	tft.setTextColor(VGA_AQUA, VGA_BLACK);
//	tft.fillRect(255, 20, 48, 16, VGA_BLACK);
	tft.drawFloat(room1_temperature, 1, 255, 20, LINETEXT);

//	tft.fillRect(255, 40, 48, 16, VGA_BLACK);
	tft.drawFloat(room2_temperature, 1, 255, 40, LINETEXT);

//	tft.fillRect(255, 60, 48, 16, VGA_BLACK);
	tft.drawFloat(outside_temperature, 1, 255, 60, LINETEXT);
}

void drawWaterTemperatures()
{
	tft.setTextColor(VGA_AQUA, VGA_BLACK);
//	tft.fillRect(255, 100, 48, 16, VGA_BLACK);
	tft.drawFloat(RW_temperature, 2, 255, 100, LINETEXT);

//	tft.fillRect(255, 120, 48, 16, VGA_BLACK);
	tft.drawFloat(target_RW_temperature, 2, 255, 120, LINETEXT);
}

time_t now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL); 
   	return tv.tv_sec;
    //return 1;
	//return system_get_rtc_time() / 100000 - start_time;
}

void drawUpdated()
{
	tft.setTextColor(VGA_SILVER, VGA_BLACK);
	cursecs = now();
//	tft.fillRect(100, 162, 48, 16, VGA_BLACK);
	tft.drawNumber(cursecs - room1_updated, 100, 162, LINETEXT);

//	tft.fillRect(100, 182, 48, 16, VGA_BLACK);
	tft.drawNumber(cursecs - room2_updated, 100, 182, LINETEXT);

//	tft.fillRect(100, 202, 48, 16, VGA_BLACK);
	tft.drawNumber(cursecs - outside_updated, 100, 202, LINETEXT);

//	tft.fillRect(260, 162, 48, 16, VGA_BLACK);
	tft.drawNumber(cursecs - heater_state_changed_time, 260, 162, LINETEXT);

	char buf[20];
	unsigned int days = cursecs / 86400ul;
	unsigned int hours = (cursecs - days * 86400ul) / 3600;
	unsigned int mins = (cursecs - days * 86400ul - hours * 3600ul) / 60ul;
	snprintf(buf, sizeof(buf), "%ud %02uh %02um", days, hours, mins);
	tft.drawString(buf, 210, 182, LINETEXT);
	
	tft.drawFloat(float(total_on_time) / (total_on_time + total_off_time) * 100, 1, 260, 202, LINETEXT);
	tft.drawFloat(float(last24h_on_time) / (last24h_on_time + last24h_off_time) * 100, 1, 260, 222, LINETEXT);
}

void drawTargetTemp()
{
	uint8_t colorvalue = (target_room_temperature - min_target_temp) / (max_target_temp - min_target_temp) * 255;
	int _color = color(colorvalue, 0, 255 - colorvalue);
	tft.setTextColor(_color, VGA_BLACK);
	tft.drawFloat(target_room_temperature, 1, 7, 24, BIGNUM);
}

void updateHeaterIcon()
{
	if (heater_enabled)	{
		tft.setTextColor(VGA_RED, VGA_BLACK);
		tft.drawString(" ON ", 52, 110, 2);
	}
	else {
		tft.setTextColor(VGA_GRAY, VGA_BLACK);
		tft.drawString("OFF", 52, 110, 2);
	}
}

void drawTargetTempScreen()
{
	uint8_t barHeight = 8;
	uint8_t barSpace = 4;
	uint8_t initialBarWidth = 8;
	int numBars = tft.height() / (barHeight + barSpace);
	for (int i = 0; i < numBars; i++)
	{
		float barTemp = min_target_temp + i * ((max_target_temp - min_target_temp) / numBars);
		int y = i * (barHeight + barSpace);
		int width = initialBarWidth + 1.0f / 1404 * y * y + 0.1624 * y;
		uint8_t colorvalue = float(i) / numBars * 255;
		int _color = color(colorvalue, 0, 255 - colorvalue);
		if (barTemp <= target_room_temperature)	{
			tft.fillRoundRect(0, tft.height() - y - barHeight, width, barHeight, 3, _color);
		}
		else {
			tft.fillRoundRect(1, tft.height() - y - barHeight + 1, width - 2, barHeight - 2, 3, VGA_BLACK);
			tft.drawRoundRect(0, tft.height() - y - barHeight, width, barHeight, 3, _color);
		}
	}
	uint8_t colorvalue = (target_room_temperature - min_target_temp) / (max_target_temp - min_target_temp) * 255;
	int _color = color(colorvalue, 0, 255 - colorvalue);
	tft.setTextColor(_color, VGA_BLACK);
	tft.drawFloat(target_room_temperature, 1, tft.width() / 2 - 64, tft.height() / 2 - 32, BIGNUM);
}

bool currentChangeTempMode = false;
void updateScreen(bool changeTempMode)
{
#ifdef TARGETTEMPSCREEN
	if (currentChangeTempMode != changeTempMode) {
		tft.fillScreen(VGA_BLACK);
		if (!changeTempMode) {
			drawWireFrame();
			drawTargetTemp();
		}
	}
	if (changeTempMode)	{
		drawTargetTempScreen();
	}
	else {
#endif
		drawTargetTemp();
		drawTemperatures();
		drawWaterTemperatures();
		updateHeaterIcon();
		drawUpdated();
#ifdef TARGETTEMPSCREEN
	}	
	currentChangeTempMode = changeTempMode;
#endif
}

void setupUI()
{
	struct timeval tv;
    gettimeofday(&tv, NULL); 
   	start_time = tv.tv_sec; //tv.tv_usec
	tft.setRotation(3);
	tft.setTextColor(VGA_GREEN, VGA_BLACK);
	tft.fillScreen(ILI9341_BLACK);
	drawWireFrame();
	drawTargetTemp();
	updateScreen(false);
}
