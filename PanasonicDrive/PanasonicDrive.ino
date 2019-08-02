#define VERSION 0.8

#define _DISPLAY 1

//#define _HANG 1
//#define _DEBUG_ 1


#include "setup.h"

#ifdef _DISPLAY
#include "Adafruit_Si7021.h"
Adafruit_Si7021 oTH = Adafruit_Si7021();
#endif


///////////// Watchdog callback to soft-reset because old Arduino Pro Mini bootloaders from Chinese clones will hang once the watchdog triggers :-(
ISR(WDT_vect)
{
  //oS.wData &= ~MOTOR_ON;
  digitalWrite(pin_Mot_on,false);
  analogWrite(pin_Mot, 0);
  Serial.println("wdt soft reset");
  asm volatile ("jmp 0");  
}

void setup() 
{
  uint8_t ch = MCUSR;
  MCUSR = 0;

  wdt_reset();
  
/*  // don't change the order or optimize anything in this block
  cli(); // important: disable interrupts
  wdt_reset();
  WDTCSR |= _BV(WDCE) | _BV(WDE);
  WDTCSR = 0;
  sei();
*/
  Serial.begin(115200);
  Serial.print("open source Panasonic drive v ");Serial.println(VERSION);
  //Serial.print("MCUSR: ");Serial.println(ch,BIN);
  //PrintDataServer("oS",oS);
  //if (! (ch &  _BV(EXTRF)))
  if (ch == 0)
  {
    PrintDataServer("watchdog reset",oS);
  }
  else
  {
    InitDataServer(oS);
    memset(&oS2,0,sizeof(oS2));
  }

#ifndef SPEED_INTERRUPT
      memset(aSpeed,0,SPEED_COUNT);
      memset(aSpeedTime,0,SPEED_COUNT);
#endif
  
  pinMode(pin_Mot_on,OUTPUT);
  digitalWrite(pin_Mot_on,false);
  pinMode(pin_Mot,OUTPUT);
  digitalWrite(pin_Mot, false);

  pinMode(pin_Button,INPUT_PULLUP);

  pinMode(pin_Speed,INPUT_PULLUP);
  
  pinMode(pin_torque_read,INPUT);
  pinMode(pin_torque_Pwm, OUTPUT);
  tone(pin_torque_Pwm,65535);

  pinMode(pin_BattV,INPUT);
  pinMode(pin_BattI,INPUT);


  EEPROM.get( 0, oStorage);
  //oStorage.iVersion = 0;
  if (oStorage.iVersion == VERSION_STORAGE)
  {
    oS.fTrip = oStorage.fTrip;
    oS.fTripDay = oStorage.fTripDay;
  }
  else
  {
    DEBUGLN("eeprom version mismatch",oStorage.iVersion);
    oStorage.iVersion = VERSION_STORAGE;
    StorageUpdate(true);
  }

#ifdef _DISPLAY
  Wire.begin();
  //setupDisplay();

  if (!oTH.begin())
    Serial.println("Did not find Si7021 sensor!");

  Wire.setClock(10000);
#endif

  iTimeRead = millis() + TIME_READ;

  Serial.println("setup done");
  wdt_enable(WDTO_500MS);   // WDTO_1S Watchdog auf 1 s stellen 
  //wdt_enable(WDTO_2S);   // WDTO_1S Watchdog auf 1 s stellen 
  WDTCSR |= (1 << WDIE); // set the WDIE flag to enable interrupt callback function.

}

void CheckSpeed()
{
  if (!digitalRead(pin_Speed))
  {
    if (iNow-iTimeSpeed > 50) // another revolution
    {
#ifndef SPEED_INTERRUPT
      memmove(aSpeed+1,aSpeed,SPEED_COUNT-1);  // memcpy
      memmove(aSpeedTime+1,aSpeedTime,SPEED_COUNT-1);  // memcpy
      unsigned long iSpeedTime = aSpeedTime[0] = iNow;
      float fSpeed = aSpeed[0] = 36.0 * CIRCRUMFERENCE / (float)(iNow-iTimeSpeed);
      for (byte i=1; i<SPEED_COUNT; i++)
      {
        if (iNow > aSpeedTime[i] + 1000)  break;  // at 25 km/h one revolution takes 300 ms
        if (fSpeed < aSpeed[i]) // take max speed to avoid wrong speed due to missed revs
        {
          fSpeed = aSpeed[i];
          iSpeedTime = aSpeedTime[i];
        }
      }
      oS.fSpeedBike = fSpeed;
      if (iSpeedTime > iSpeedTimeLast)  // we have a new max speed within 1 second
      {
        float fDist = fSpeed * (float)(iSpeedTime-iSpeedTimeLast)/3600.0;
        oS.fTrip += fDist;
        oS.fTripDay += fDist;
        iSpeedTimeLast = iSpeedTime;
      }
#else
      oS.fSpeedBike = 36.0 * CIRCRUMFERENCE / (float)(iNow-iTimeSpeed);
      oS.fTrip += CIRCRUMFERENCE/100.0;
      oS.fTripDay += CIRCRUMFERENCE/100.0;
#endif      

      if (oS.fTrip > oStorage.fTrip+1000) StorageUpdate(true);
      else if ( (oS.fTripDay > 20000) && (oS.fBattV > 40.0) & (iNow > 60000)  )
      {
        oS.fTripDay = 0;
        StorageUpdate(true);
      }
    }
    iTimeSpeed = iNow;
  }
  else if (iNow-iTimeSpeed > 2000) // 7488 ms would be 1 kmh for a 26" wheel
  {
      oS.fSpeedBike = 0;
      StorageUpdate();
  }
}

void loop()
{
  wdt_reset();  
  //unsigned int 0iTimeLastLoop = millis()-iNow; if (iTimeLastLoop>1) Serial.println(iTimeLastLoop);

  iNow = millis();
  //while(1)  iNow++;

  CheckSpeed();
  
  fReadTorque += analogRead(pin_torque_read);
  fReadBattV += analogRead(pin_BattV);
  fReadBattI += analogRead(pin_BattI);
  iReadButton += analogRead(pin_Button);
  iReads++;
  //DEBUG(iNow,iReads);
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
  oS.fBattV = mapf(oS.fBattV,0,1023,0.0,48.5);
  oS.fBattI = mapf(oS.fBattI,512,1023,0.0,-25.0);

  oS.wData = (oS.wData & ~BUTTON_ON) | (iButton<500 ? BUTTON_ON : 0);
//Serial.print(iNow);
#ifdef _DISPLAY
  //Serial.print(iButton);Serial.print("\t-> ");Serial.println(oS.wData);

  unsigned long iTime = millis();
  Wire.requestFrom(SLAVE_ADDRESS, sizeof(oC)+1);
  //delay(1);
  unsigned int iAvail = Wire.available();
  if (!SerialRead(Wire,oC,iAvail))
  {
    if (iAvail)
    {
      DEBUGLN("\nCRC fail or avail: ",iAvail);
    }
    //Serial.print("\ttwi_error: ");Serial.println(twi_error);
  }
  if (millis()-iTime > 50)
  {
    DEBUG("Wire reset",millis()-iTime);
    Wire.end();
    Wire.begin(); 
  }
  CheckSpeed();
//Serial.print(": ");

  //Serial.print(" 1 ");
  //oS.fH = oTH.readHumidity();
  //oS.fT = oTH.readTemperature();
  //Serial.print(" 2 ");

  if (oC.wButtons)
  {
    //if (  (oC.wButtons & Btn_Light) && (oC.wButtons & Btn_Mode))
    if (oC.wButtons & Btn_Mode)
    {
      if (iNow > iTimeButton + 3000)
      {
        oS.wData = (oS.wData & ~SPEED_25) | (oS.wData&SPEED_25 ? 0 : SPEED_25);
        DEBUGLN("SPEED_25",oS.wData & SPEED_25);
        iTimeButton = iNow + 10000;  // wait 13000 until next toggle
      }
    }
    else if (oC.wButtons & Btn_Light)
    {
      if (iNow > iTimeButton + 3000)
      {
        oS.fTripDay = 0;
        StorageUpdate(true);
        iTimeButton = iNow;  // wait 3000 until next toggle
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
  if (oS.wData & BUTTON_ON)
  {
    Serial.println("* Panic *");
    digitalWrite(pin_Mot, 0);
    oS.wData &= ~MOTOR_ON;
  }
#else
  if (oS.wData & BUTTON_ON)
    if (iNow > iTimeButton)
    {
      if (oS.wData&MOTOR_ON)
      {
        Serial.println("* motor off *");
        oS.wData &= ~MOTOR_ON;
      }
      else
      {
        Serial.println("* motor on *");
        oS.fStop = oS.iTorqueMaxDown = oS.iTorqueMaxUp = 0; 
        iTimeReady = iNow;
        fStop = iStopReads = 0;
      }
      iTimeButton = iNow + 3000;  // wait 5000 + 3000 until next toggle
    }
#endif
//Serial.print("3 ");

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
#ifdef _HANG
      if (iNow > 10000)
      {
        Serial.println("*hang*");
        while (1) delay(10);
      }
#endif   
      int iSpeedMin = 0;  
      int iSpeedStep = 10; 
      if ((oS.wData&SPEED_25) && (oS.fSpeedBike > 25))
      {
        oS.iSpeed -= iSpeedStep;
        DEBUG("limit",oS.iSpeed);
      }
      else if (oS.fBattI > 10)
      {
        oS.iSpeed -= 16*(oS.fBattI-8);
        iSpeedMin = 0;
        DEBUG("Overcurrent",oS.iSpeed);
      }
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
        digitalWrite(pin_Mot_on,false);
        DEBUG("Stop",oS.iSpeed);
        oS.iSpeed = 0;    
        iSpeedMin = 0;
      }
      else if (oS.fTorque<(oS.fStop+5))
      {
        oS.iSpeed -= iSpeedStep;
        DEBUG("slower",oS.iSpeed);
      }
      //else if (oS.fTorque>=(oS.fStop+5))
      else
      {
        if (iNow > iTimeSpeed +2000) // fSpeedBike is set to 0 km/h after 3000 ms no new iTimeSpeed
        {
          // start from 0 kmh need +30 torque
          if (oS.fTorque>=(oS.fStop+30))
          {
            if (oS.iSpeed)  oS.iSpeed += iSpeedStep;
            else  oS.iSpeed = 100;
            DEBUG("start",oS.iSpeed);
          }
          else
          {
            DEBUG("wait",oS.iSpeed);
            iSpeedMin = 0;
          }
        }
        else
        {
          if (oS.iSpeed)  oS.iSpeed += iSpeedStep;
          else  oS.iSpeed = 100;
          DEBUG("faster",oS.iSpeed);
        }                                                
/*        
        if (oS.fTorque > fTorqueLast)
          oS.iSpeed = (((255.0*(oS.fTorque-oS.fStop-5))/5)*oC.iAssist)/100;
        else
          oS.iSpeed -= 10;
  */        
        //oS.iSpeed += 20;
      }
      //CLAMP(oS.iSpeed,0,255);
      CLAMP(oS.iSpeed,iSpeedMin,  iSpeedMin+ ((255-iSpeedMin)*oC.iAssist)/100  );

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
        DEBUG("keep pedals quiet! ",abs(f-oS.fTorque));
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

#ifdef _DEBUG_
//Serial.print("4 ");

  //Serial.print(iReads); 
  //Serial.println();
  PrintDataServer("",oS,false);
  Serial.print(" | ass: ");Serial.print(oC.iAssist);Serial.print(" bt: ");Serial.print(iButton);Serial.print(" Bt: ");Serial.println(oC.wButtons);
#endif
  
  iReads = 0;
  iTimeRead = millis() + TIME_READ;

  //Serial.print("\t cntrl: ");Serial.print(millis()-iNow);
#ifdef _DISPLAY
  if (iTimeDisplay > iNow) return;

//Serial.print("5 ");

  //for (int i=0; i<Torque_Count; i++)  {Serial.print(", ");Serial.print(oS2.aTorque[i]);}Serial.println();
 
  Send2Client(1);
  iTimeRead = millis() + TIME_READ;
  iTimeDisplay = millis() + TIME_DISPLAY;
  //Serial.print("\t sent: ");Serial.print(millis()-iNow);

  //oS.fTrip += 100;  oS.fTripDay += 100;
#endif
}
