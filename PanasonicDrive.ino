#define VERSION 0.7

#define CIRCRUMFERENCE 208.0  // 2 pi r in centimeters

#include <D:\Projects Arduino\panasonicDrive.h>

#include <avr/wdt.h>
#include "setup.h"
//#include "display.h"

#include "Adafruit_Si7021.h"
Adafruit_Si7021 oTH = Adafruit_Si7021();

void setup() {
  pinMode(pin_Mot_on,OUTPUT);
  digitalWrite(pin_Mot_on,false);
  pinMode(pin_Mot,OUTPUT);
  digitalWrite(pin_Mot, false);

  Serial.begin(115200);
  Serial.print("open source Panasonic drive v ");Serial.println(VERSION);
  pinMode(pin_Button,INPUT_PULLUP);

  pinMode(pin_Speed,INPUT_PULLUP);
  
  pinMode(pin_torque_read,INPUT);
  pinMode(pin_torque_Pwm, OUTPUT);
  tone(pin_torque_Pwm,65535);

  pinMode(pin_BattV,INPUT);
  pinMode(pin_BattI,INPUT);
  Wire.begin();
  //setupDisplay();

  if (!oTH.begin())
    Serial.println("Did not find Si7021 sensor!");

  Wire.setClock(10000);
  iTimeRead = millis() + TIME_READ;

  Serial.println("setup done");
  wdt_enable(WDTO_500MS);   // WDTO_1S Watchdog auf 1 s stellen 
}

void loop()
{
  wdt_reset();  
  //unsigned int iTimeLastLoop = millis()-iNow; if (iTimeLastLoop>1) Serial.println(iTimeLastLoop);

  iNow = millis();
  if (!digitalRead(pin_Speed))
  {
    //Serial.println("click");
    if (iNow-iTimeSpeed > 50) // another revolution
    {
      oS.fSpeedBike = 36.0 * CIRCRUMFERENCE / (float)(iNow-iTimeSpeed);
    }
    iTimeSpeed = iNow;
  }
  else if (iNow-iTimeSpeed > 7500) // another revolution
      oS.fSpeedBike = 0;
  
  fReadTorque += analogRead(pin_torque_read);
  fReadBattV += analogRead(pin_BattV);
  fReadBattI += analogRead(pin_BattI);
  iReadButton += analogRead(pin_Button);
  iReads++;

  if (iNow < iTimeRead)
    return;

  //Serial.println();

  fReadTorque /= iReads;
  float fTorqueD = fReadTorque - oS.fTorque;
  if (fTorqueD > 0)
  {
    if (oS.iTorqueMaxUp < fTorqueD)
      oS.iTorqueMaxUp = fTorqueD;
  }
  else
  {
    if (oS.iTorqueMaxDown < -fTorqueD)
      oS.iTorqueMaxDown = -fTorqueD;
 }
  
  oS.fTorque = fReadTorque;
  oS.fBattV =  fReadBattV / iReads;
  oS.fBattI =  fReadBattI / iReads;
  int iButton = iReadButton / iReads;
  iReadButton = fReadBattI = fReadBattV = fReadTorque = 0;


  //Serial.print("fBattI: ");Serial.println(fBattI);
  oS.fBattV = mapf(oS.fBattV,0,1024,0.0,50.0);
  oS.fBattI = mapf(oS.fBattI,512,1023,0.0,-25.0);

  oS.wData = (oS.wData & ~BUTTON_ON) | (iButton<500 ? BUTTON_ON : 0);
  //Serial.print(iButton);Serial.print("\t-> ");Serial.println(oS.wData);

  unsigned long iTime = millis();
  Wire.requestFrom(SLAVE_ADDRESS, sizeof(oC)+1);
  //delay(1);
  unsigned int iAvail = Wire.available();
  if (!SerialRead(Wire,oC,iAvail))
  {
    Serial.print("\nCRC fail or avail: ");Serial.println(iAvail);
    //Serial.print("\ttwi_error: ");Serial.println(twi_error);
  }
  if (millis()-iTime > 50)
  {
    Serial.println(millis()-iTime);
    Wire.end();
    Wire.begin(); 
  }

  oS.fH = oTH.readHumidity();
  oS.fT = oTH.readTemperature();

  if (oC.wButtons)
  {
    if (  (oC.wButtons & Btn_Light) && (oC.wButtons & Btn_Mode))
    {
      if (iNow > iTimeButton)
      {
        oS.wData = (oS.wData & ~SPEED_25) | (oS.wData&SPEED_25 ? 0 : SPEED_25);
        iTimeButton = iNow + 3000;  // wait 3000 until next toggle
      }
    }
    else if (oC.wButtons & Btn_OnOff)
    {
      if (iNow > iTimeButton + 500)
      {
        if (oS.wData&MOTOR_ON)
        {
          oS.wData &= ~MOTOR_ON;
        }
        else
        {
          oS.fStop = oS.iTorqueMaxDown = oS.iTorqueMaxUp = 0; 
          iTimeReady = iNow;
          fStop = iStopReads = 0;
        }
        iTimeButton = iNow + 3000;  // wait 5000 + 3000 until next toggle
      }
    }
  }
  else
  {
    iTimeButton = iNow;
  }

  //if (0)
  if (oS.wData & BUTTON_ON)
  {
    Serial.println("* Panic *");
    digitalWrite(pin_Mot, 0);
    oS.wData &= ~MOTOR_ON;
  }
  //oS.wData |= MOTOR_ON;
  digitalWrite(pin_Mot_on,oS.wData&MOTOR_ON);
    
  memmove(oS2.aTorque,oS2.aTorque+1,Torque_Count-1);  // memcpy
  if (oS.fStop > 0)
  {
    oS2.aTorque[Torque_Count-1] = oS.fTorque;

/*
    byte iTorqueMin=255, iTorqueMax=0; 
    float fTorqueMeanS = 0;
    for (int i=1; i<=LOGS_MINMAX; i++)
    {
      byte iT = oS2.aTorque[Torque_Count-i];
      if (iTorqueMin > iT)  iTorqueMin = iT;
      if (iTorqueMax < iT)  iTorqueMax = iT;
      fTorqueMeanS += oS2.aTorque[Torque_Count-i];
    }
    fTorqueMeanS /= LOGS_MINMAX;

    float fTorqueMean = 0;
    for (int i=1; i<=LOGS_MEAN; i++)
    fTorqueMean += oS2.aTorque[Torque_Count-i];
    fTorqueMean /= LOGS_MEAN;
  
    oS.iTorqueMaxDown = iTorqueMin;
    oS.iTorqueMaxUp = iTorqueMax;
    //oS.fStop = fTorqueMean;

    byte iTorqueD = iTorqueMax-iTorqueMin;
    //if (oS.fStop > oS.fTorque)  oS.fStop = oS.fTorque;
*/
    //oS.wData &= (iLoop%42)<22 ? 255 : ~MOTOR_ON;
    //oS.wData |= MOTOR_ON;

    if (oS.wData&MOTOR_ON)
    {
      if ((oS.wData&SPEED_25) && (oS.fSpeedBike > 25))
        oS.iSpeed -= 10;
      else if (oS.fBattI > 15)
        oS.iSpeed -= 30;
/*      
      if (fTorqueMeanS < (fTorqueMean-4))
      {
        digitalWrite(pin_Mot_on,false);
        oS.iSpeed = 0;    
      }
      else if (fTorqueMeanS > (fTorqueMean+4))
      {
        oS.iSpeed += 10;    
      }
      if (iTorqueD < 3)
      {
        oS.iSpeed -= 10;    
      }
      else if (iTorqueD > 5)
      {
        oS.iSpeed += 10;    
      }

*/
//      else if (1){}
      else if (oS.fTorque<(oS.fStop+4))
      {
        Serial.println("Stop");
        digitalWrite(pin_Mot_on,false);
        oS.iSpeed = 0;    
      }
      else if (oS.fTorque<(oS.fStop+5))
      {
        oS.iSpeed -= 20;
      }
      else if (oS.fTorque>=(oS.fStop+5))
      {
        if (oS.fTorque > fTorqueLast)
          oS.iSpeed = (((255.0*(oS.fTorque-oS.fStop-5))/5)*oC.iAssist)/100;
        else
          oS.iSpeed -= 10;
          
        //oS.iSpeed += 20;
      }
      if (oS.iSpeed>255)  oS.iSpeed=255;
      else if (oS.iSpeed<0) oS.iSpeed=0;

/*
      int iSpeedMax = (255*oC.iAssist)/100;
      if (oS.iSpeed>iSpeedMax) 
        oS.iSpeed=iSpeedMax;
      else 
        if (oS.iSpeed<0) oS.iSpeed=0;
*/
    }

    //Serial.print(iReads); Serial.print(": ");Serial.print(oS.fTorque);Serial.print("  stop: ");Serial.print(oS.fStop);
    //Serial.print("\tiAdd: ");Serial.print(iAdd);
    //Serial.print(" -> ");Serial.println(oS.iSpeed);   
    
    //oS.iSpeed = (iLoop%21)*13;
    //oS.iSpeed = oC.wButtons&Btn_Light ? 255 : 0;
    analogWrite(pin_Mot, oS.iSpeed);
    iLoop++;
  }
  else if (iTimeReady>0)
  {
    oS2.aTorque[Torque_Count-1] = 0;
    if (iNow>(iTimeReady+500))
    {
      fStop += oS.fTorque;
      iStopReads++;

      float f = fStop/iStopReads;
      if (abs(f-oS.fTorque)>1)
      {
        Serial.print("keep pedals quiet! ");Serial.println(abs(f-oS.fTorque));
        iTimeReady = iNow-500;
        fStop = iStopReads = 0;
      }
      else
        oS.fStop = -f;
    }
//    if (oS.fStop > -oS.fTorque)
//      oS.fStop = -oS.fTorque;

    if (iNow > iTimeReady+TIME_READY)
    {
      oS.fStop = -oS.fStop;
      //oS.fStop = -oS.fStop / iReadsStop;
      oS.wData |= MOTOR_ON;
    }
  }
  fTorqueLast = oS.fTorque;

  //Serial.print(iReads); 
  //Serial.println();
  
  Serial.print(": ");Serial.print(oS.fTorque);Serial.print("  stop: ");Serial.print(oS.fStop);
  Serial.print("\tassist: ");Serial.print(oC.iAssist);Serial.print("  speed: ");Serial.print(oS.iSpeed);
  Serial.print("\tBattV: ");Serial.print(oS.fBattV);
  Serial.print("\tBattI: ");Serial.print(oS.fBattI);Serial.print("\tbt: ");Serial.print(iButton);
  Serial.print("\tkm/h: ");Serial.print(oS.fSpeedBike);Serial.print("\twData: ");Serial.print(oS.wData);
  Serial.print("\twButtons: ");Serial.println(oC.wButtons);
  
  iReads = 0;
  iTimeRead = millis() + TIME_READ;

  //Serial.print("\t cntrl: ");Serial.print(millis()-iNow);

  if (iTimeDisplay > iNow) return;


  //for (int i=0; i<Torque_Count; i++)  {Serial.print(", ");Serial.print(oS2.aTorque[i]);}Serial.println();
 
  Send2Client(1);
  iTimeRead = millis() + TIME_READ;
  iTimeDisplay = millis() + TIME_DISPLAY;
  //Serial.print("\t sent: ");Serial.print(millis()-iNow);
}
