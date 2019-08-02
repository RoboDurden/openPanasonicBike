#include <stdio.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#include <D:\Projects Arduino\panasonicDrive.h>


#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8

byte iModeDisplay = 0;
unsigned long iTimeAssist  = 0;
unsigned long iNow = 0;

#ifdef _DISPLAY_TFT
  #include "displayTFT.h"
#else
  #include "displaySsd1306.h"
#endif

unsigned long iReceived = 0, iReceived2 = 0;
unsigned long iRequest = 0;



#define pin_Btn_Light 5
#define pin_Btn_OnOff 6
#define pin_Btn_Up 3
#define pin_Btn_Down 2
#define pin_Btn_Mode 4

#define pin_Backlight 7

int iBacklight = 0;
unsigned long iTimeDisplay = 0;


void BacklightOn(boolean bOn=true)
{
  if (bOn)
  {
    pinMode(pin_Backlight,OUTPUT);
    digitalWrite(pin_Backlight,LOW);
  }
  else
    pinMode(pin_Backlight,INPUT_PULLUP);
}

///////////// Watchdog callback to soft-reset because old Arduino Pro Mini bootloaders from Chinese clones will hang once the watchdog triggers :-(
ISR(WDT_vect)
{
  Serial.println("wdt soft reset");
  asm volatile ("jmp 0");  
}



#define VERSION_STORAGE 1
struct Storage {
  uint16_t iVersion;
  uint8_t iAssist;
};

Storage oStorage;
unsigned long iTimeStorage = 0;
void StorageUpdate(boolean bForce = false)
{
  if (bForce || (iNow > iTimeStorage))
  {
    DEBUGLN("StorageUpdate() bForce",bForce);
    iTimeStorage = iNow + 3000;
    oStorage.iAssist = oC.iAssist;
    EEPROM.put( 0, oStorage);
    return;
  }
}
