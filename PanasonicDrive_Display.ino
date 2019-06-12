#define VERSION 0.3

#include <D:\Projects Arduino\panasonicDrive.h>

// include TFT and SPI libraries
#include <TFT.h>  
#include <SPI.h>
// pin definition for Arduino UNO
#define cs   10
#define dc   9
#define rst  8
// create an instance of the library
TFT TFTscreen = TFT(cs, dc, rst);


unsigned long iReceived = 0, iReceived2 = 0;
unsigned long iRequest = 0;


byte iModeDisplay = 0;

#define pin_Btn_Light 5
#define pin_Btn_OnOff 6
#define pin_Btn_Up 3
#define pin_Btn_Down 2
#define pin_Btn_Mode 4

#define pin_Backlight 7

int iBacklight = 0;
unsigned long iTimeDisplay = 0;
unsigned long iNow = 0;

int aOldY[Torque_Count];
byte iTorqueMin=255, iTorqueMax=0;

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

void setup()
{
  Serial.begin (115200);
  Serial.print("open source Panasonic DISPLAY v ");Serial.println(VERSION);
  //for (int i=0; i<Torque_Count; i++)  oS.aTorque[i] = random(100);
    
  pinMode(pin_Btn_Light,INPUT_PULLUP);
  pinMode(pin_Btn_OnOff,INPUT_PULLUP);
  pinMode(pin_Btn_Up,INPUT_PULLUP);
  pinMode(pin_Btn_Down,INPUT_PULLUP);
  pinMode(pin_Btn_Mode,INPUT_PULLUP);

  BacklightOn();
  
  
  TFTscreen.begin();
  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);
  //set the text size
  TFTscreen.setTextSize(2);
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text("open Flyer :-)", 0, 0);
  
  Wire.begin (SLAVE_ADDRESS);
  //Wire.setClock(10000);
  
  Serial.println("I2C slave, my address:" + String(SLAVE_ADDRESS));
  Wire.onReceive (receiveEvent);
  Wire.onRequest(requestEvent); // register event
}  // end of setup


void DisplayFloat(char* sLabel,int x1, float f, int iPrec,int x2, int y, byte iSize=2)
{
  TFTscreen.setTextSize(2);
  TFTscreen.noStroke();
  TFTscreen.fill(1, 1, 1);
  TFTscreen.rect(x1,y,80,8*iSize);
  TFTscreen.noFill();

  TFTscreen.stroke(0, 0, 255);
  TFTscreen.text(sLabel, x1,y);

  TFTscreen.setTextSize(iSize);

  int iWidth = (!iPrec) ? 3 : 4;
  char s[iWidth+1]; // Buffer big enough for 7-character float
  dtostrf(f, iWidth, iPrec, s);
  TFTscreen.stroke(0, 255, 255);
  TFTscreen.text(s, x1+strlen(sLabel)*12,y);
}

boolean bDisplay = false;
byte wButtons = 0;
void loop()
{
  iNow = millis();

  //BacklightOn(iNow%3000 > 1000);
  
  if ((!bDisplay) && (iTimeDisplay > iNow)) return;

  iTimeDisplay = iNow + 250;
  bDisplay = false;

  oC.wButtons = 0;
  oC.wButtons |= digitalRead(pin_Btn_Light) ? 0 : Btn_Light;
  oC.wButtons |= digitalRead(pin_Btn_OnOff) ? 0 : Btn_OnOff;
  oC.wButtons |= digitalRead(pin_Btn_Up) ? 0 : Btn_Up;
  oC.wButtons |= digitalRead(pin_Btn_Down) ? 0 : Btn_Down;
  oC.wButtons |= digitalRead(pin_Btn_Mode) ? 0 : Btn_Mode;

  if (oC.wButtons & Btn_Up)
    if (oC.iAssist < 100) oC.iAssist += 5;
  if (oC.wButtons & Btn_Down)
    if (oC.iAssist > 0) oC.iAssist -= 5;

  if (oC.wButtons & Btn_Light)
  {
    
  }
  else  if (oC.wButtons & Btn_Mode)
  {
    iModeDisplay = (iModeDisplay+1)%2;
    TFTscreen.background(0, 0, 0);
  }


  Serial.print("received: ");Serial.print(iReceived); Serial.print("\treceived2: ");Serial.print(iReceived2); Serial.print("\trequest: ");Serial.print(iRequest); 
  Serial.print("\twButtons: ");Serial.println(oC.wButtons);

  Serial.print("T: ");Serial.print(oS.fT);Serial.print(" H: ");Serial.print(oS.fH);
  Serial.print("\tfTorque: ");Serial.print(oS.fTorque);Serial.print("\tstop: ");Serial.print(oS.fStop);
  Serial.print("\tspeed: ");Serial.print(oS.iSpeed);Serial.print("\tBattV: ");Serial.print(oS.fBattV);
  Serial.print("\tfBattI: ");Serial.print(oS.fBattI);//Serial.print("\tbt: ");Serial.print(iButton);
  Serial.print("\tkm/h: ");Serial.print(oS.fSpeedBike);Serial.print("\tmot: ");Serial.println(oS.wData&MOTOR_ON ? "on":"off");


  //BacklightOn(oS.wData&MOTOR_ON);
  BacklightOn(oS.wData&MOTOR_ON || oS.fStop < 0);

  switch(iModeDisplay)
  {
  case 0:
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
    else
    {
      DisplayFloat("a",0,oC.iAssist,0,13,0,3);
      DisplayFloat("s",80,oS.fSpeedBike,1,93,0,3);
       DisplayFloat("V",0,oS.fBattV,1,13,24,3);
       DisplayFloat("A",80,oS.fBattI,1,93,24,3);
      DisplayFloat("T:",0,oS.fT,1,13,48);
      DisplayFloat("H:",80,oS.fH,1,93,48);
    }
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
  TFTscreen.stroke(0, 0, 0);
  for (i=1; i<Torque_Count; i++)
  {
    float x1 = x0 + dx;
    TFTscreen.line(x0,aOldY[i-1],x1,aOldY[i]);
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
    TFTscreen.stroke(0, 255, 255);
    TFTscreen.line(x0,y0,x1,y1);

    aOldY[i-1] = y0;
    x0 = x1;
    y0 = y1;
  }
  aOldY[i-1] = y0;
  //delay(5000);
  
  //Serial.println(iReceived);
    
}  // end of loop

// called by interrupt service routine when incoming data arrives
void receiveEvent (int iSize)
{
  //Serial.print("receiveEvent: ");Serial.println(iSize); 
  switch(iSize)
  {
  case sizeof(oS)+1:
    if (SerialRead(Wire,oS,iSize))
    {
      switch(oS.iCmd)
      {
      case 1:
        iReceived++;
        break;
      }
      return;
    }
    break;
  case sizeof(oS2)+1:
    if (SerialRead(Wire,oS2,iSize))
    {
      switch(oS2.iCmd)
      {
      case 1:
        iReceived2++;
        bDisplay = true;
        break;
      }
      return;
    }
    break;
  }
  Serial.print("\nCRC failed or iSize: ");Serial.print(iSize);
  Serial.print("\texpected: ");Serial.println(sizeof(oS)+1);
 }  // end of receiveEvent

void requestEvent() 
{
  iRequest++;
  SerialWrite(Wire,oC);
  //Serial.print("request: ");Serial.print(iRequest++); Serial.print("\tsending: wButtons = ");Serial.println(oC.wButtons);
}
