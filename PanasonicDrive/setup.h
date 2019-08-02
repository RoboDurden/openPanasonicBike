#define CIRCRUMFERENCE 208.0  // 2 pi r in centimeters

#include <D:\Projects Arduino\panasonicDrive.h>

#include <avr/wdt.h>
#include <EEPROM.h>


#define TIME_READ 200  // ms to integrate analogRead
#define LOGS_MINMAX 5   // LOGS_MINMAX * TIME_READ = time window to evaluate max- and min-torque
#define LOGS_MEAN 30   // LOGS_MEAN * TIME_READ = time window to calculate mean torque

#define TIME_READY 3000 // ms to read lowest torque before being ready to ride
#define TIME_DISPLAY 500  // ms to integrate analogRead


#define pin_torque_Pwm 3
#define pin_torque_read A3

#define pin_Speed  7  // pin 7 is no interrupt pin :-(
//#define SPEED_INTERRUPT 1

#define pin_Mot_on  8
#define pin_Mot 9

#define pin_BattI A0

#define pin_BattV A6
#define pin_Button A7

unsigned long iNow = 0;

#define VERSION_STORAGE 1
struct Storage {
  uint16_t iVersion = VERSION_STORAGE;
  float fTrip = 0;
  float fTripDay = 0;
};

Storage oStorage;
unsigned long iTimeStorage = 0;
void StorageUpdate(boolean bForce = false)
{
  if (bForce || (iNow > iTimeStorage))
  {
    DEBUGLN("StorageUpdate() bForce",bForce);
    iTimeStorage = iNow + 3000;
    oStorage.fTrip = oS.fTrip;
    oStorage.fTripDay = oS.fTripDay;
    EEPROM.put( 0, oStorage);
    return;
  }
}


#ifndef SPEED_INTERRUPT
  #define SPEED_COUNT 4
  unsigned long aSpeedTime[SPEED_COUNT];
  float aSpeed[SPEED_COUNT];
  unsigned long iSpeedTimeLast = 0;
#endif



//boolean bButton = false;

unsigned long iTimeSpeed;
//boolean bSpeedSensor;

unsigned long iLoop = 0;

unsigned long iTimeButton = 0;
unsigned long iTimeReady = 0;
unsigned long iTimeRead = 0;
unsigned long iReads = 0;

unsigned long iStopReads = 0;
float fStop = 0;

unsigned long iTimeDisplay = 0;

float fReadTorque = 0;
float fReadBattV = 0;
float fReadBattI = 0;
unsigned long iReadButton = 0;

float fTorqueLast = 0;

void Send2Client(byte iCommand)
{
  //return;
  //Serial.print("SerialWrite: oS");Serial.println(sizeof(oS)+1);
  
  oS.iCmd = iCommand;
  Wire.beginTransmission(SLAVE_ADDRESS);
  delay(22);
  SerialWrite(Wire,oS);
  Wire.endTransmission();
  //Serial.print("SerialWrite oS2: ");Serial.println(sizeof(oS2)+1);
  oS2.iCmd = iCommand;
  Wire.beginTransmission(SLAVE_ADDRESS);
  delay(22);
  SerialWrite(Wire,oS2);
  Wire.endTransmission();
}
