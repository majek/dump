#include <Wire.h>

//#define SERIAL 1
#define I2CADDR 0x21

void setup() {
#if SERIAL
	Serial.begin(57600);
#endif
	Wire.begin();
	pinMode(5, OUTPUT);

	// set summing to zero
	Wire.beginTransmission(I2CADDR);
	Wire.write(0x47);
	Wire.write(0x06);
	Wire.write(0x01);
	Wire.endTransmission();
	delay(1);

}


int heading() {
	Wire.beginTransmission(I2CADDR);
	Wire.write("A");
	Wire.endTransmission();
	delay(10);

	Wire.requestFrom(I2CADDR, 2);
	
	while (Wire.available() < 2) {
		delay(1);
	}
	byte msb = Wire.read();
	byte lsb = Wire.read();

	return (msb << 8) | lsb;
}

float magnetometer() {
	float x, y;
	// Set adjusted X magnetometer mode (03)
	Wire.beginTransmission(I2CADDR);
	Wire.write(0x47);
	Wire.write(0x4E);
	Wire.write(1);//0x03);
	Wire.endTransmission();
	delay(1);

	x = heading();

	// Set adjusted Y magnetometer mode (04)
	Wire.beginTransmission(I2CADDR);
	Wire.write(0x47);
	Wire.write(0x4E);
	Wire.write(2);//0x04);
	Wire.endTransmission();
	delay(1);

	y = heading();

#if 0
	// Reset compass to normal mode
	Wire.beginTransmission(I2CADDR);
	Wire.write(0x47);
	Wire.write(0x4E);
	byte val = 0;
	Wire.write(val);
	Wire.endTransmission();
	delay(1);

	float norm = heading() / 10;
	Serial.print(norm);
#endif

	float dist = sqrt(x*x + y*y);
	return dist;
}

int prev;
void loop() {
	/* float norm = heading(); */
	/* Serial.print(norm); */
	/* Serial.println(); */

	float g = abs(3000 - magnetometer());

#if SERIAL
	Serial.print(g);
	Serial.println();
#else
	if (g > 500) {
		int i = (g - 500) / 10;
		if (i > 10) i = 10;
		analogWrite(5, 16*(6+i) - 1);
		if (!prev)
			delay(400);
		prev = 1;
	} else {
		if (prev) {
			analogWrite(5, 0);
		}
		prev = 0;
	}
	delay(50);
#endif
	/* int i; */
	/* for (i=6; i<16; i++) { */
	/* 	analogWrite(5, 16*i); */
	/* 	delay(1000); */
	/* 	analogWrite(5, 0); */
	/* 	delay(200); */
	/* } */
	/* delay(200); */
}
