#define VERSION 0.8

//#define _DEBUG_ 1

//#define _DISPLAY_TFT 1


#include "include.h"


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
  Serial.print("open source Panasonic DISPLAY v ");Serial.println(VERSION);
  Serial.print("MCUSR: ");Serial.println(ch,BIN);
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
  
  //for (int i=0; i<Torque_Count; i++)  oS.aTorque[i] = random(100);
    
  pinMode(pin_Btn_Light,INPUT_PULLUP);
  pinMode(pin_Btn_OnOff,INPUT_PULLUP);
  pinMode(pin_Btn_Up,INPUT_PULLUP);
  pinMode(pin_Btn_Down,INPUT_PULLUP);
  pinMode(pin_Btn_Mode,INPUT_PULLUP);

  EEPROM.get( 0, oStorage);
  //oStorage.iVersion = 0;
  if (oStorage.iVersion == VERSION_STORAGE)
  {
    oC.iAssist = oStorage.iAssist;
  }
  else
  {
    DEBUGLN("eeprom version mismatch",oStorage.iVersion);
    oStorage.iVersion = VERSION_STORAGE;
    StorageUpdate(true);
  }

  BacklightOn();

  SetupDisplay();
  delay(1000);

  
  Wire.begin (SLAVE_ADDRESS);
  //Wire.setClock(10000);
  
  Serial.print("I2C slave, my address:");Serial.println(SLAVE_ADDRESS);
  Wire.onReceive (receiveEvent);
  Wire.onRequest(requestEvent); // register event

  //wdt_enable(WDTO_500MS);   // WDTO_1S Watchdog auf 1 s stellen 
  //wdt_enable(WDTO_1S);   // WDTO_1S Watchdog auf 1 s stellen 
  //WDTCSR |= (1 << WDIE); // set the WDIE flag to enable interrupt callback function.
  Serial.println("setup done");
}  // end of setup



boolean bDisplay = false;
byte wButtons = 0;
void loop()
{
  wdt_reset();  
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
  {
    if (oC.iAssist < 100) 
    {
      oC.iAssist += 5;
      StorageUpdate(true);
    }
    iTimeAssist = iNow + 3000;
  }
  if (oC.wButtons & Btn_Down)
  {
    if (oC.iAssist > 0) 
    {
      oC.iAssist -= 5;
      StorageUpdate(true);
    }
    iTimeAssist = iNow + 3000;
  }

  if (oC.wButtons & Btn_Light)
  {
    
  }
  else  if (oC.wButtons & Btn_Mode)
  {
    iModeDisplay = (iModeDisplay+1)%3;
    DisplayClear();
  }

#ifdef _DEBUG_
  Serial.print("received: ");Serial.print(iReceived); Serial.print("\treceived2: ");Serial.print(iReceived2); Serial.print("\trequest: ");Serial.print(iRequest); 
  Serial.print("\twButtons: ");Serial.println(oC.wButtons);
  PrintDataServer(String(iNow),oS);
#endif

  //BacklightOn(oS.wData&MOTOR_ON);
  BacklightOn(oS.wData&MOTOR_ON || oS.fStop < 0);

  Display();
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
