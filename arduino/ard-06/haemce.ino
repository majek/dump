/* -*-mode:c;  -*- */
#include <Wire.h>
#include <hmc5883L.h>

//#define SERIAL_DO
#define BUZZER_PIN 5

HMC5883L compass;



void setupHMC5883L(){
	compass.SetScale(8.1);
	compass.SetRegisterA(0, 5, 0);
	compass.SetMeasurementMode(Measurement_Continuous); // Set the measurement mode to Continuous
}

float getHeading(){
	//Get the reading from the HMC5883L and calculate the heading
	MagnetometerScaled scaled = compass.ReadScaledAxis(); //scaled values from compass

#ifdef SERIAL_DO
	Serial.print(scaled.XAxis);
	Serial.print(" ");
	Serial.print(scaled.YAxis);
	Serial.print(" ");
	Serial.print(scaled.ZAxis);
	Serial.print(" ");
#endif

	/* float heading = atan2(scaled.YAxis, scaled.XAxis); */

	// Correct for when signs are reversed.
	/* if(heading < 0) heading += 2*PI; */
	/* if(heading > 2*PI) heading -= 2*PI; */

	/* return heading * RAD_TO_DEG; //radians to degrees */

	return sqrt(scaled.XAxis * scaled.XAxis +
		    scaled.YAxis * scaled.YAxis +
		    scaled.ZAxis * scaled.ZAxis);
}

int buzz_prev = 0;
void buzzer(int v) {
	if (v) {
		if (v < 0) v = 0;
		if (v > 10) v = 10;
		analogWrite(BUZZER_PIN, 16 * (6 + v) - 1);
		if (!buzz_prev) {
			buzz_prev = 1;
		}
	} else {
		if (buzz_prev) {
			analogWrite(BUZZER_PIN, 0);
			buzz_prev = 0;
		}
	}
}

void setup(){
#ifdef SERIAL_DO
	Serial.begin(115200);
 	Serial.println("abc");
#endif
 	Serial.println("abc");

	Wire.begin();
 	pinMode(5, OUTPUT);
	delay(1);

	compass = HMC5883L(); //new instance of HMC5883L library
	setupHMC5883L(); //setup the HMC5883L
}

// Our main program loop.
void loop(){
	float delta = getHeading();

#ifdef SERIAL_DO
 	Serial.println(delta);
#endif

	if (delta > 700.0) {
		if (delta > 2700) delta = 2700;
		// 1..10
		buzzer(1 + (delta - 700.)/200.);
	} else {
		buzzer(0);
	}

	delay(34);
}
