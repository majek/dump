/*
HMC5883L.h - Header file for the HMC5883L Triple Axis Magnetometer Arduino Library.
Copyright (C) 2011 Love Electronics (loveelectronics.co.uk) / 2012 bildr.org (Arduino 1.0 compatible)

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

WARNING: THE HMC5883L IS NOT IDENTICAL TO THE HMC5883!
Datasheet for HMC5883L:
http://www51.honeywell.com/aero/common/documents/myaerospacecatalog-documents/Defense_Brochures-documents/HMC5883L_3-Axis_Digital_Compass_IC.pdf

*/

#ifndef HMC5883L_h
#define HMC5883L_h

#include <Arduino.h>
#include <Wire.h>



#define HMC5883L_Address 0x1E
#define ConfigurationRegisterA 0x00
#define ConfigurationRegisterB 0x01
#define ModeRegister 0x02
#define DataRegisterBegin 0x03

#define Measurement_Continuous 0x00
#define Measurement_SingleShot 0x01
#define Measurement_Idle 0x03

#define ErrorCode_1 "Entered scale was not valid, valid gauss values are: 0.88, 1.3, 1.9, 2.5, 4.0, 4.7, 5.6, 8.1"
#define ErrorCode_1_Num 1

struct MagnetometerScaled
{
    float XAxis;
    float YAxis;
    float ZAxis;
};

struct MagnetometerRaw
{
    int XAxis;
    int YAxis;
    int ZAxis;
};

class HMC5883L
{
    public:
     HMC5883L();

     MagnetometerRaw ReadRawAxis();
     MagnetometerScaled ReadScaledAxis();

     int SetMeasurementMode(uint8_t mode);
     int SetScale(float gauss);
     int SetRegisterA(uint8_t reg);
     int SetRegisterA(uint8_t avg, uint8_t rate, uint8_t bias);

     char* GetErrorText(int errorCode);

    protected:
     void Write(int address, int byte);
     uint8_t* Read(int address, int length);

    private:
     float m_Scale;
};
#endif
