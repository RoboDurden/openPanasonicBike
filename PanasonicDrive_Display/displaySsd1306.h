#include "ssd1306.h"
#include "fonts.h"
int aOldY[Torque_Count];
byte iTorqueMin=255, iTorqueMax=0;


void SetupDisplay()
{
  memset(&aOldY,0,sizeof(aOldY));
  st7735_128x160_spi_init(TFT_RST, TFT_CS, TFT_DC);
  ssd1306_setMode( LCD_MODE_NORMAL );
  st7735_setRotation(3);
  ssd1306_fillScreen8( 0x00 );

  ssd1306_setColor(RGB_COLOR8(0,255,255));
  ssd1306_drawRect8(0,19,159,87);
  //ssd1306_setFixedFont(ssd1306xled_font6x8);  //ssd1306xled_font8x16
  ssd1306_setFreeFont(free_Targa11x24);
  ssd1306_setColor(RGB_COLOR8(255,255,255));
  //ssd1306_print8(25, 32, "open Flyer :-)", STYLE_BOLD);
  //ssd1306_printFixedN(25, 32, "open Flyer :-)", STYLE_NORMAL,2);
  ssd1306_setCursor8(20, 24); ssd1306_print8("open  Flyer :-)");

  char sV[5];
  dtostrf(VERSION, 4, 2, sV);
  char s[16];
  sprintf(s,"Version:  %s", sV);
  //ssd1306_printFixed8(25, 48, s, STYLE_BOLD);
  ssd1306_setCursor8(20, 58); ssd1306_print8(s);
 
  Serial.println(s);
}

void DisplayClear()
{
  ssd1306_fillScreen8( 0x00 );
}


int GetWidth(const char s[],const uint8_t* aFont, char cDot=0)
{
  if (!strlen(s)) return 0;
  
  //SCharInfo char_info;  ssd1306_getCharBitmap('.', &char_info);
  
  uint8_t iFrom = (uint8_t)pgm_read_word(&(aFont[5]));
  uint8_t iTo = iFrom + (uint8_t)pgm_read_word(&(aFont[6]));
  uint16_t  iWidth = (strlen(s)-1) * 1; //char_info.spacing;

  uint16_t i = 0;
  while(s[i])
  {
    uint8_t c = s[i];
    if (  (c>=iFrom) && (c<iTo)  )
      iWidth += (uint8_t)pgm_read_word(&(aFont[9 + (c-iFrom)*4]));
    if (s[i++]==cDot) break;
  }
  DEBUGLN(s,iWidth);
  return iWidth;
}

int GetMaxWidth(const uint8_t* aFont)
{
  uint8_t iWidth = 0;
  uint8_t iNum = (uint8_t)pgm_read_word(&(aFont[6]));
  for (int i=0; i<iNum; i++)
  {
    uint8_t iW = (uint8_t)pgm_read_word(&(aFont[9 + i*4]));
    if (iWidth < iW)  iWidth = iW;
  }
  return iWidth;
}
inline uint8_t GetMaxHeight(const uint8_t* aFont){return (uint8_t)pgm_read_word(&(aFont[2]));};

boolean DisplayFloat(int x, int y, int iWidthWindow, char* sLabel,float f, int iPrec=1,byte iSize=2,uint16_t color = RGB_COLOR8(255,255,255))
{
  int x0 = x;

  int iSizeLabel = iSize>2 ? 2 : iSize;
  int iWidth;
  int iWidthLabel;
  int iWidthFloat;
  const uint8_t* aFont;
  const uint8_t* aFontLabel;
  char s[11];  // max number before mcu crashes : '+23456.890' + 0x00
  boolean bNoLastTry = true;
  do
  {
    switch(iSizeLabel)
    {
    case 1: aFontLabel = free_Targa8x16; break;
    case 2: aFontLabel = free_Targa11x24; break;
    }
    switch(iSize)
    {
    case 1: aFont = free_Targa8x16; break;
    case 2: aFont = free_Targa11x24; break;
    case 3: aFont = free_comicbd26x29; break;
    }

    dtostrf(f, 1, iPrec, s);
    //Serial.print("'");Serial.print(s);Serial.println("'");
    DEBUG("s",s);
    iWidthFloat = GetWidth(s,aFont);
    int iWidthInt = GetWidth(s,aFont,'.');  // width from left including '.'
    uint8_t iMaxW = GetMaxWidth(aFont);

    iWidth = bNoLastTry ? iWidthInt+iPrec*iMaxW : iWidthFloat;
    iWidthLabel = GetWidth(sLabel,aFontLabel);
    DEBUG("iWidthLabel",iWidthLabel);DEBUG("iWidth",iWidth);DEBUG("iWidthFloat",iWidthFloat);
    if (iWidthLabel+iWidth > iWidthWindow)
    {
      DEBUG("iPrec",iPrec);
      DEBUG("iSize",iSize);
      if (iPrec > 0)  iPrec--;
      else if (iSizeLabel > 1) iSizeLabel--;
      else if (iSize > 1) iSize--;
      else if (iWidthLabel) sLabel[0] = 0; //iWidthLabel = 0;
      else if (bNoLastTry) bNoLastTry = false;
      else return false; // :-/
    }
    else  break;  // :-)
  } while (1);
  
  if (iWidthLabel)
  {
    ssd1306_setColor(RGB_COLOR8(255,255,0));
    ssd1306_setFreeFont(aFontLabel);
    ssd1306_setCursor8(x, y); ssd1306_print8(sLabel);
    //ssd1306_setFixedFont(ssd1306xled_font8x16);
    //ssd1306_printFixed8(x, y, sLabel, STYLE_BOLD);
    x += iWidthLabel;
    DEBUG("label",x);
  }

  uint8_t iMaxH = GetMaxHeight(aFont);
  int iWidthEmpty = iWidthWindow-(iWidthLabel+iWidth); 
  if (iWidthEmpty>0)
  {
    ssd1306_setColor(RGB_COLOR8(0,0,0));
    ssd1306_fillRect8(x,y,x+iWidthEmpty,y+iMaxH);    
    x +=  iWidthEmpty;
    DEBUG("empty",x);
  }

  ssd1306_setFreeFont(aFont);
  ssd1306_setColor(color);
  ssd1306_setCursor8(x, y);
  ssd1306_print8(s);
  x += iWidthFloat;
  if (x-x0 < iWidthWindow)
  {
    ssd1306_setColor(RGB_COLOR8(0,0,0));
    ssd1306_fillRect8(x,y,x0+iWidthWindow-1,y+iMaxH);    
    DEBUG("rest",x);
  }
  DEBUGLN("y",y);
  return true;
}



boolean bInit = true;
void Display()
{
  DEBUG("Display()",iModeDisplay);

  if (oS.fStop <= 0)
  {
    ssd1306_fillScreen8( 0x00 );
    //int x = (millis()%3000)/50;
    ssd1306_setFreeFont(free_Targa11x24);
    ssd1306_setCursor8(20, 0); ssd1306_print8("keine  Pedale  !");
    ssd1306_setCursor8(20, 24); ssd1306_print8("no  pedaling");
    
//    ssd1306_setFixedFont(ssd1306xled_font8x16);
//    ssd1306_printFixed8(x, 0, "keine Pedale !", STYLE_BOLD);
//    ssd1306_printFixed8(x, 16, "no pedaling !", STYLE_BOLD);
    
    DisplayFloat(0,48,80, "t",oS.fTorque,0,3);
    DisplayFloat(80,48,80, "s",oS.fStop,0,3);
    bInit = true;
  }
  else
  {
    if (bInit)
    {
      ssd1306_fillScreen8( 0x00 );
      bInit = false;
      iTimeAssist = iNow + 3000;
    }
    switch(iModeDisplay)
    {
    case 0:
        //DisplayFloat("a",0,oC.iAssist,0,13,0,3);
        DisplayFloat(0,0,80, "",oS.fSpeedBike,1,3);
        DisplayFloat(80,0,80, "",oS.fTripDay/1000.0,2,3,RGB_COLOR8(128,255,255));
        if (iNow > iTimeAssist)
        {
          //float fTest = 36.0+(float)(iNow%2000)/2000.0;
          float fFull = mapf(oS.fBattV,33.0,41.0,0.0,1.0,true);
          DisplayFloat(0,32,80, "",oS.fBattV,1,3,RGB_COLOR8(255,(int)(255*fFull),0));
          DisplayFloat(80,32,80, "",oS.fBattV*oS.fBattI,0,3,RGB_COLOR8(255,128,255)); //oS.fBattI,1,3
        }
        else
          DisplayFloat(20,32,120, "assist:",oC.iAssist,0,3,RGB_COLOR8(255,155,0));
      break;
    case 1:
      DisplayFloat(0,0,70, "sys",oS.wData,0);
      DisplayFloat(80,0,70, "btn",oC.wButtons,0);
       DisplayFloat(0,24,80, "V",oS.fBattV);
       DisplayFloat(80,24,80, "A",oS.fBattI);
      DisplayFloat(0,48,80, "km",oS.fTripDay/1000.0,2);
      DisplayFloat(80,48,80, "km",oS.fTrip/1000.0,1);

      
      break;
    case 2:
      DisplayFloat(0,0,76, "To",oS.fTorque);
      DisplayFloat(80,0,76, "St",oS.fStop);
       DisplayFloat(0,24,76, "speed",oS.iSpeed,0);
       DisplayFloat(80,24,76, "kmh",oS.fSpeedBike,1);
      DisplayFloat(0,48,76, "assist",oC.iAssist,0);
      DisplayFloat(80,48,76, "A",oS.fBattI,1);
       //DisplayFloat(0,48,50, "V",oS.fBattV,1,1);
       //DisplayFloat(53,48,50, "A",oS.fBattI,1,1);
      break;  
    }
  }
  
  //TFTscreen.background(0, 0, 0);
  //TFTscreen.text("i2c :-)", 0,(iReceived*8)%128);

//Serial.println(" done");return;

  //for (int i=0; i<Torque_Count; i++)  oS.aTorque[i] = random(100);
  //TFTscreen.noStroke();TFTscreen.fill(1, 1, 1);TFTscreen.rect(0,80,160,120);TFTscreen.noFill();

  if (oS.wData&BUTTON_ON)
  {
    iTorqueMin=255, iTorqueMax=0;
  }
  float dx = 160.0 / Torque_Count;
  float x0=0;
  int i;
  //ssd1306_setColor(RGB_COLOR8(0,0,0));
  for (i=1; i<Torque_Count; i++)
  {
    float x1 = x0 + dx;

    //ssd1306_drawLine8(x0,aOldY[i-1],x1,aOldY[i]);
    byte iTorque = oS2.aTorque[i];
    if (iTorque>0)
    {
      if (iTorqueMax < iTorque) iTorqueMax = iTorque;
      if (iTorqueMin > iTorque) iTorqueMin = iTorque;
    }
  }

  float yScale = 55.0/(float)(iTorqueMax-iTorqueMin);

  int y0 = 127 - (float)(oS2.aTorque[0]-iTorqueMin) * yScale;
  CLAMP(y0,72,127);

  x0=0;
  for (i=1; i<Torque_Count; i++)
  {
    int y1 = 128 - (float)(oS2.aTorque[i]-iTorqueMin) * yScale;
    CLAMP(y1,72,127);
    float x1 = x0 + dx;

    ssd1306_setColor(RGB_COLOR8(0,0,0));
    ssd1306_drawLine8(x0,aOldY[i-1],x1,aOldY[i]);
    ssd1306_setColor(RGB_COLOR8(255, 255, 255));
    ssd1306_drawLine8(x0,y0,x1,y1);

    aOldY[i-1] = y0;
    x0 = x1;
    y0 = y1;
  }
  aOldY[i-1] = y0;

  //Serial.println(" complete");
}
