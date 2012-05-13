#include <EEPROM.h>
#include <Wire.h>
int HMC6352Address = 0x42;
int slaveAddress;
byte headingData[2];
byte magData[2];
int i, headingValue;
int magx,magy;
int adcntmax=2;
unsigned long time;
unsigned long startTime;
unsigned long endTime;
unsigned long timeTaken;
int gmode=0;
int ledPin=12;
int switchPin=5;
int tiltPin=7;
int gs1Pin=10;
int gs2Pin=11;
int ledStatus;
int switchStatus;
int tiltStatus;
int inByte = 0;
int Xpin=0;
int Ypin=1;
int Zpin=2;
int Xval;
int Yval;
int Zval;
int Xcentre;
int Ycentre;
int Zcentre;
int c;
int bt;
int cnt;

void navoutputdecimal()
{
  int cnt,ad;
  toggleled();
  getHeading();
  time = millis();
  Serial.print("$GARDO,");
  Serial.print(time,DEC);
  Serial.print(",");
  Serial.print(headingValue,DEC);
  Serial.print(",");
  for (cnt=0; cnt<=adcntmax; cnt++)
  {
    ad=analogRead(cnt);
    Serial.print(ad,DEC);
    Serial.print(",");
  }
  ad=analogRead(7);
  Serial.print(ad,DEC);
  Serial.print(13,BYTE);
  Serial.print(10,BYTE);
  toggleled();
}

void navcontinuousoutput()
{
  int cnt,ad;
  while(1)
  {
    toggleled();
    navoutputdecimal();
    toggleled();
    if(Serial.available() > 0)
    {
      inByte = Serial.read();
      if(inByte=='x') return;
    }
    delay(200);
  }
}

void magnetometer()
{
  // Set adjusted X magnetometer mode (03)
  Wire.beginTransmission(slaveAddress);
  Wire.send(0x47);
  Wire.send(0x4E);
  Wire.send(0x03);
  Wire.endTransmission();
  delay(1);
  // Get the X magnetometer data
  Wire.beginTransmission(slaveAddress);
  Wire.send("A");
  Wire.endTransmission();
  delay(10);
  Wire.requestFrom(slaveAddress, 2);
  i = 0;
  while(Wire.available() && i < 2)
  {
    magData[i] = Wire.receive();
    i++;
  }
  magx=magData[0]*256 + magData[1];
  // Set adjusted Y magnetometer mode (04)
  Wire.beginTransmission(slaveAddress);
  Wire.send(0x47);
  Wire.send(0x4E);
  Wire.send(0x04);
  Wire.endTransmission();
  delay(1);
  // Get the Y magnetometer data
  Wire.beginTransmission(slaveAddress);
  Wire.send("A");
  Wire.endTransmission();
  delay(10);
  Wire.requestFrom(slaveAddress, 2);
  i = 0;
  while(Wire.available() && i < 2)
  {
    magData[i] = Wire.receive();
    i++;
  }
  magy=magData[0]*256 + magData[1];
  // Reset compass to normal mode
  Wire.beginTransmission(slaveAddress);
  Wire.send(0x47);
  Wire.send(0x4E);
  Wire.send(0x00);
  Wire.endTransmission();
  delay(1);
  // Get the compass heading
  Wire.beginTransmission(slaveAddress);
  Wire.send("A");
  Wire.endTransmission();
  delay(10);
  Wire.requestFrom(slaveAddress, 2);
  i = 0;
  while(Wire.available() && i < 2)
  { 
    headingData[i] = Wire.receive();
    i++;
  }
  headingValue = headingData[0]*256 + headingData[1];
  headingValue=int(headingValue/10);
  // Output the data to serial comms
  Serial.print(headingValue,DEC);
  Serial.print(",");
  Serial.print(magx,DEC);
  Serial.print(",");
  Serial.print(magy,DEC);
  Serial.print(13,BYTE);
  Serial.print(10,BYTE);
}

void getHeading()
{
  Wire.beginTransmission(slaveAddress);
  Wire.send("A");
  Wire.endTransmission();
  delay(10);
  Wire.requestFrom(slaveAddress, 2);
  i = 0;
  while(Wire.available() && i < 2)
  { 
    headingData[i] = Wire.receive();
    i++;
  }
  headingValue = headingData[0]*256 + headingData[1];
  headingValue=int(headingValue/10);
}

void vrstatus()
{
  Xval=analogRead(Xpin);
  Yval=analogRead(Ypin);
  Zval=analogRead(Zpin);
  Xval=(Xval-Xcentre)+512;
  Yval=(Yval-Ycentre)+512;
  Zval=(Zval-Zcentre)+512;
  getbuttonstate();
  Serial.print(Xval,DEC);
  Serial.print(",");
  Serial.print(Yval,DEC);
  Serial.print(",");
  Serial.print(Zval,DEC);
  Serial.print(",");
  Serial.print(switchStatus,DEC);
  Serial.print(",");
  Serial.print(tiltStatus,DEC);
  Serial.print(",");
  Serial.print(ledStatus,DEC);
  Serial.print(13,BYTE);
  Serial.print(10,BYTE);
}

void onbuttonmode()
{
  do
  {
    switchStatus = digitalRead(switchPin);
    tiltStatus = digitalRead(tiltPin);
    if(switchStatus==1)
    {
      vrstatus();
      delay(100);
    }
//    else if(tiltStatus==0)
//    {
//      vrstatus();
//      delay(200);
//    }    
    if (Serial.available() > 0)
    {
      inByte = Serial.read();
      if (inByte=='*')
      {
       return; 
      }
      else if (inByte=='o')
      {
       toggleled(); 
      }
      else if (inByte=='c')
      {
        calibrate();
      }
      else if (inByte=='s')
      {
        saveCalib();
      }
      else if (inByte=='u')
      {
        Xcentre=512;
        Ycentre=512;
        Zcentre=512;
      }
      else if (inByte=='l')
      {
        loadCalib();
      }
    }
  }
  while (1);
}

void buffout(int x, int y, int z)
{
  bt=highByte(x);
  Serial.write(bt);
  bt=lowByte(x);
  Serial.write(bt);
  bt=highByte(y);
  Serial.write(bt);
  bt=lowByte(y);
  Serial.write(bt);
  bt=highByte(z);
  Serial.write(bt);
  bt=lowByte(z);
  Serial.write(bt);
}

void fastxyz()
{
  int accelbuffX[200];
  int accelbuffY[200];
  int accelbuffZ[200];
  toggleled();
  startTime = micros();
  for (c = 0; c <= 199; c++)
  {
    accelbuffX[c]=analogRead(Xpin);
    accelbuffY[c]=analogRead(Ypin);
    accelbuffZ[c]=analogRead(Zpin);
  }  
  endTime = micros();
  toggleled();
  for (c = 0; c <= 199; c++)
  {
    accelbuffX[c]=(accelbuffX[c]-Xcentre)+512;
    accelbuffY[c]=(accelbuffY[c]-Ycentre)+512;
    accelbuffZ[c]=(accelbuffZ[c]-Zcentre)+512;
    buffout(accelbuffX[c],accelbuffY[c],accelbuffZ[c]);
  }
  timeTaken=endTime-startTime;
  bt=highByte(timeTaken);
  Serial.write(bt);
  bt=lowByte(timeTaken);
  Serial.write(bt);
  Serial.write(gmode);
}
  
void setgmode()
{
  switch(gmode)
  {
    case 0:
      digitalWrite(gs1Pin, LOW);
      digitalWrite(gs2Pin, LOW);
      break; 
    case 1:
      digitalWrite(gs1Pin, HIGH);
      digitalWrite(gs2Pin, LOW);
      break; 
    case 2:
      digitalWrite(gs1Pin, LOW);
      digitalWrite(gs2Pin, HIGH);
      break; 
    case 3:
      digitalWrite(gs1Pin, HIGH);
      digitalWrite(gs2Pin, HIGH);
      break; 
  }
}

void getbuttonstate()
{
  switchStatus = digitalRead(switchPin);
  tiltStatus = digitalRead(tiltPin);
}

void toggleled()
{
  if (ledStatus==0)
  {
    digitalWrite(ledPin, HIGH);
    ledStatus=1;
  }
  else
  {
    digitalWrite(ledPin, LOW);
    ledStatus=0;
  }
}

void calibrate()
{
  Xcentre=analogRead(Xpin);
  Ycentre=analogRead(Ypin);
  Zcentre=analogRead(Zpin);
}

void saveCalib()
{
  EEPROM.write(1,1);
  EEPROM.write((gmode*6)+2,highByte(Xcentre));
  EEPROM.write((gmode*6)+3,lowByte(Xcentre));
  EEPROM.write((gmode*6)+4,highByte(Ycentre));
  EEPROM.write((gmode*6)+5,lowByte(Ycentre));
  EEPROM.write((gmode*6)+6,highByte(Zcentre));
  EEPROM.write((gmode*6)+7,lowByte(Zcentre));
}

void loadCalib()
{
  int a;
  int b;
  if (EEPROM.read(1)==1)
  {
    a=EEPROM.read((gmode*6)+2);
    b=EEPROM.read((gmode*6)+3);
    Xcentre=(a<<8)+b;
    a=EEPROM.read((gmode*6)+4);
    b=EEPROM.read((gmode*6)+5);
    Ycentre=(a<<8)+b;
    a=EEPROM.read((gmode*6)+6);
    b=EEPROM.read((gmode*6)+7);
    Zcentre=(a<<8)+b;
  }
}

void readRawAccel()
{
  Xval=analogRead(Xpin);
  Yval=analogRead(Ypin);
  Zval=analogRead(Zpin);
}

void readAccel()
{
  Xval=analogRead(Xpin);
  Yval=analogRead(Ypin);
  Zval=analogRead(Zpin);
  Xval=(Xval-Xcentre)+512;
  Yval=(Yval-Ycentre)+512;
  Zval=(Zval-Zcentre)+512;
}

void setup()
{
  slaveAddress = HMC6352Address >> 1;
  analogReference(EXTERNAL);
  pinMode(gs1Pin, OUTPUT);
  pinMode(gs2Pin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(switchPin, INPUT);     
  pinMode(tiltPin, INPUT);     
  ledStatus=0;  
  switchStatus=0;  
  tiltStatus=0;  
  Xcentre=512;
  Ycentre=512;
  Zcentre=512;
  loadCalib();
  Serial.begin(9600);
  Wire.begin();
}

void loop()
{
  if (Serial.available() > 0)
  {
    inByte = Serial.read();
    if (inByte=='m')
    {
      vrstatus();
    }
    else if (inByte=='v')
    {
      Serial.println("VRduino v1.0");
    }
    else if (inByte=='0')
    {
      gmode=0;
      setgmode();
    }
    else if (inByte=='1')
    {
      gmode=1;
      setgmode();
    }
    else if (inByte=='2')
    {
      gmode=2;
      setgmode();
    }
    else if (inByte=='3')
    {
      gmode=3;
      setgmode();
    }
    else if (inByte=='4')
    {
      navoutputdecimal();
    }
    else if (inByte=='5')
    {
      navcontinuousoutput();
    }
    else if (inByte=='6')
    {
      toggleled();
      magnetometer();
      toggleled();
    }
    else if (inByte=='7')
    {
      toggleled();
      for(cnt=1;cnt<=200;cnt++)
      {
        magnetometer();
        delay(5);
      }
      toggleled();
    }
    else if (inByte=='c')
    {
      calibrate();
    }
    else if (inByte=='o')
    {
      toggleled();
    }
    else if (inByte=='s')
    {
      saveCalib();
    }
    if (inByte=='b')
    {
      onbuttonmode();
    }
    else if (inByte=='r')
    {
      readRawAccel();
      Serial.print(Xval,DEC);
      Serial.print(",");
      Serial.print(Yval,DEC);
      Serial.print(",");
      Serial.print(Zval,DEC);
      Serial.print(13,BYTE);
      Serial.print(10,BYTE);
    }
    else if (inByte=='a')
    {
      readAccel();
      Serial.print(Xval,DEC);
      Serial.print(",");
      Serial.print(Yval,DEC);
      Serial.print(",");
      Serial.print(Zval,DEC);
      Serial.print(13,BYTE);
      Serial.print(10,BYTE);
    }
    else if (inByte=='t')
    {
      readAccel();
      Serial.print(Xcentre,DEC);
      Serial.print(",");
      Serial.print(Ycentre,DEC);
      Serial.print(",");
      Serial.print(Zcentre,DEC);
      Serial.print(13,BYTE);
      Serial.print(10,BYTE);
    }
    else if (inByte=='n')
    {
      Serial.print(13,BYTE);
      Serial.print(10,BYTE);
    }
    else if (inByte=='d')
    {
      toggleled();
      startTime = millis();
      for (c = 1; c <= 200; c++)
      {
        readAccel();
        bt=highByte(Xval);
        Serial.write(bt);
        bt=lowByte(Xval);
        Serial.write(bt);
        bt=highByte(Yval);
        Serial.write(bt);
        bt=lowByte(Yval);
        Serial.write(bt);
        bt=highByte(Zval);
        Serial.write(bt);
        bt=lowByte(Zval);
        Serial.write(bt);
        delay(30);
      }
      endTime = millis();
      toggleled();
      timeTaken=endTime-startTime;
      bt=highByte(timeTaken);
      Serial.write(bt);
      bt=lowByte(timeTaken);
      Serial.write(bt);
      Serial.write(gmode);
    }
    else if (inByte=='f')
    {
      toggleled();
      startTime = millis();
      for (c = 1; c <= 200; c++)
      {
        readAccel();
        bt=highByte(Xval);
        Serial.write(bt);
        bt=lowByte(Xval);
        Serial.write(bt);
        bt=highByte(Yval);
        Serial.write(bt);
        bt=lowByte(Yval);
        Serial.write(bt);
        bt=highByte(Zval);
        Serial.write(bt);
        bt=lowByte(Zval);
        Serial.write(bt);
      }
      endTime = millis();
      toggleled();
      timeTaken=endTime-startTime;
      bt=highByte(timeTaken);
      Serial.write(bt);
      bt=lowByte(timeTaken);
      Serial.write(bt);
      Serial.write(gmode);
    }
    else if (inByte=='e')
    {
      toggleled();
      startTime = millis();
      for (c = 1; c <= 200; c++)
      {
        readAccel();
        Serial.print(Xval,DEC);
        Serial.print(",");
        Serial.print(Yval,DEC);
        Serial.print(",");
        Serial.print(Zval,DEC);
        Serial.print(13,BYTE);
        Serial.print(10,BYTE);
      }
      endTime = millis();
      toggleled();
      timeTaken=endTime-startTime;
      Serial.print("Time (ms),");
      Serial.print(timeTaken);
      Serial.print(13,BYTE);
      Serial.print(10,BYTE);
    }
    else if (inByte=='u')
    {
      Xcentre=512;
      Ycentre=512;
      Zcentre=512;
    }
    else if (inByte=='l')
    {
      loadCalib();
    }
    else if (inByte=='g')
    {
      fastxyz();
    }
  }
}
