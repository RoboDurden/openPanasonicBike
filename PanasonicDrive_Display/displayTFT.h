#include <TFT.h>  
#include <SPI.h>
TFT TFTscreen = TFT(TFT_CS, TFT_DC, TFT_RST);

int aOldY[Torque_Count];
byte iTorqueMin=255, iTorqueMax=0;

void SetupDisplay()
{
  TFTscreen.begin();
  TFTscreen.background(0, 0, 0);
  TFTscreen.setTextSize(2);
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text("open Flyer :-)", 0, 0);
}

void DisplayFloat(char* sLabel,int x1, float f, int iPrec,int x2, int y, byte iSize=2)
{
  TFTscreen.setTextSize(2);
  TFTscreen.noStroke();
  TFTscreen.fill(1, 1, 1);
  TFTscreen.rect(x1,y,80,8*iSize);
  TFTscreen.noFill();

  TFTscreen.stroke(0, 255, 255);
  TFTscreen.text(sLabel, x1,y);

  TFTscreen.setTextSize(iSize);

  int iWidth = (!iPrec) ? 3 : 4;
  char s[iWidth+1]; // Buffer big enough for 7-character float
  dtostrf(f, iWidth, iPrec, s);
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text(s, x1+strlen(sLabel)*12,y);
}

void DisplayClear()
{
    TFTscreen.background(0, 0, 0);
}

void Display()
{
  if (oS.fStop <= 0)
  {
    TFTscreen.background(0, 0, 0);
    TFTscreen.stroke(255, 255, 255);
    TFTscreen.setTextSize(3);
    TFTscreen.text("keine", 0,0);
    TFTscreen.text("Pedale", 0,24);
    DisplayFloat("t",0,oS.fTorque,1,13,48);
    DisplayFloat("s",80,oS.fStop,1,93,48);
  }
  else switch(iModeDisplay)
  {
  case 0:
    DisplayFloat("a",0,oC.iAssist,0,13,0,3);
    DisplayFloat("s",80,oS.fSpeedBike,1,93,0,3);
     DisplayFloat("V",0,oS.fBattV,1,13,24,3);
     DisplayFloat("A",80,oS.fBattI,1,93,24,3);
    DisplayFloat("T:",0,oS.fT,1,13,48);
    DisplayFloat("H:",80,oS.fH,1,93,48);
    break;
  case 1:
    DisplayFloat("a",0,oC.iAssist,0,13,0,3);
    DisplayFloat("s",80,oS.fSpeedBike,1,93,0,3);
     DisplayFloat("V",0,oS.fBattV,1,13,24,3);
     DisplayFloat("A",80,oS.fBattI,1,93,24,3);
    DisplayFloat("T:",0,oS.fT,1,13,48);
    DisplayFloat("H:",80,oS.fH,1,93,48);
    break;
  case 1:
    DisplayFloat("t",0,oS.fTorque,1,13,0);
    DisplayFloat("s",80,oS.fStop,1,93,0);
     DisplayFloat("a",0,oC.iAssist,0,13,16);
     DisplayFloat("i",80,oS.iSpeed,0,93,16);
    DisplayFloat("d:",0,oS.wData,0,13,32);
    DisplayFloat("k",80,oS.fSpeedBike,1,93,32);
     DisplayFloat("V",0,oS.fBattV,1,13,48);
     DisplayFloat("A",80,oS.fBattI,1,93,48);
    break;  
  }

  //TFTscreen.background(0, 0, 0);
  //TFTscreen.text("i2c :-)", 0,(iReceived*8)%128);




  //for (int i=0; i<Torque_Count; i++)  oS.aTorque[i] = random(100);
  //TFTscreen.noStroke();TFTscreen.fill(1, 1, 1);TFTscreen.rect(0,80,160,120);TFTscreen.noFill();

  if (oS.wData&BUTTON_ON)
  {
    iTorqueMin=255, iTorqueMax=0;
  }
  float dx = 160.0 / Torque_Count;
  float x0=0;
  int i;
  //TFTscreen.stroke(0, 0, 0);
  for (i=1; i<Torque_Count; i++)
  {
    float x1 = x0 + dx;
    //TFTscreen.line(x0,aOldY[i-1],x1,aOldY[i]);
    byte iTorque = oS2.aTorque[i];
    if (iTorque>0)
    {
      if (iTorqueMax < iTorque) iTorqueMax = iTorque;
      if (iTorqueMin > iTorque) iTorqueMin = iTorque;
    }
  }

  float yScale = 55.0/(float)(iTorqueMax-iTorqueMin);

  int y0 = 120 - (float)(oS2.aTorque[0]-iTorqueMin) * yScale;
  x0=0;
  for (i=1; i<Torque_Count; i++)
  {
    int y1 = 120 - (float)(oS2.aTorque[i]-iTorqueMin) * yScale;
    float x1 = x0 + dx;

    TFTscreen.stroke(0, 0, 0);
    TFTscreen.line(x0,aOldY[i-1],x1,aOldY[i]);
    TFTscreen.stroke(255, 255, 255);
    TFTscreen.line(x0,y0,x1,y1);

    aOldY[i-1] = y0;
    x0 = x1;
    y0 = y1;
  }
  aOldY[i-1] = y0;
  
}
