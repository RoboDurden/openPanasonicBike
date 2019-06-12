// WARNING: DEBUG_COMM=1 will fuck up SerialWrite in Wire.onRequest :-(



/*	C:\Program Files\Arduino\hardware\arduino\avr\libraries\Wire\src\Wire.h
#ifndef BUFFER_LENGTH
	#define BUFFER_LENGTH 32
#endif
*/
#define TWI_BUFFER_LENGTH 64
#define BUFFER_LENGTH 64
#include <Wire.h>


#define SLAVE_ADDRESS 42

#define Btn_Light 1
#define Btn_OnOff 2
#define Btn_Up 4
#define Btn_Down 8
#define Btn_Mode 16

typedef struct Data_Client {
	byte wButtons = 0;
	byte iAssist = 50;
         } ;

Data_Client oC;

#define Torque_Count 30

#define MOTOR_ON 1
#define BUTTON_ON 2
#define SPEED_25 4

typedef struct __attribute((__packed__)) Data_Server {
	byte iCmd = 0;
	float fTorque = 0;
	float fStop = 0;
	byte iTorqueMaxDown = 0;
	byte iTorqueMaxUp = 0;
	int iSpeed = 0;
	float fBattV = 0;
	float fBattI = 0;
	float fSpeedBike = 0;
	byte wData = SPEED_25;
	float fT = 0;
	float fH = 0;
         } ;

typedef struct __attribute((__packed__)) Data_Server2 {
	byte iCmd = 0;
	byte aTorque[Torque_Count];
         } ;


Data_Server oS;
Data_Server2 oS2;

//CRC-8 - based on the CRC8 formulas by Dallas/Maxim
//code released under the therms of the GNU GPL 3.0 license
byte CRC8(const byte *data, byte len) {
  byte crc = 0x00;
  while (len--) {
    byte extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}



template <typename O,typename T> unsigned int SerialWrite (O& oSerial, const T& o)
{
	oSerial.write((byte *) &o, sizeof (o));
/*	
	byte* p = (byte*) &o;
	for (unsigned int i=0; i < sizeof o; i++)
		oSerial.write(*p++);
*/
	byte iCRC8 = CRC8((byte *) &o,sizeof (o));
	oSerial.write(iCRC8);
#ifdef DEBUG_COMM
	byte* p2 = (byte*) &o;
	for (unsigned int i=0; i < sizeof o; i++)
	{
	    Serial.print(i);Serial.print(" write = ");Serial.println(*p2++);
	}
	Serial.println(iCRC8);
#endif	
	return sizeof (o);
}


template <typename C,typename S, typename I> bool SerialRead(C& oSerial,S& o, I iAvailable)
{
	//int iAvailable = oSerial.available();
	if (iAvailable < sizeof(o)+1 ) 
	{
#ifdef DEBUG_COMM		
		if (iAvailable > 0)	Serial.println(String(millis()) + ": " + String(iAvailable) + " < " + String(sizeof(o)));
#endif
		return false;
	}
	
	byte buff[sizeof(o)];

	//byte* p = (byte*) &o;
	byte* p = buff;
	for (unsigned int i=0; i < sizeof o; i++)
	    *p++ = oSerial.read();
	byte iMust = CRC8(buff,sizeof(o) );
	byte iCRC8 = oSerial.read();
#ifdef DEBUG_COMM		
	for (unsigned int i=0; i < sizeof o; i++)
	{
	    Serial.print(i);Serial.print(" read = ");Serial.println(buff[i]);
	}   
	Serial.print(iMust); Serial.print(" = iMust == iCRC8 = "); Serial.println(iCRC8);
#endif
	//return true;
	if (iMust != iCRC8)	return false;
	memcpy(&o,buff,sizeof(o));
	return true;
}


