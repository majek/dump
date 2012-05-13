#include "geiger.h"

int pin_tmp36 = 0; // analog input pin
int pin_hih4030 = 1; // analog input pin
int pin_piezo = 9; // PWM output

void setup() {
	Serial.begin(9600);
	Serial.println("# tmp36 C");

	pinMode(pin_piezo, OUTPUT);
	analogWrite(pin_piezo, 0);

	geiger_setup();
}


static float readVoltage(int pin) {
	int reading = analogRead(pin);
	return (reading * 5.0) / 1024.0;
}


// in C
float read_tmp36() {
	return (readVoltage(pin_tmp36) - 0.5) * 100;
}

// In RH percentage
float read_hum(float temp_c) {
	float rh_at_25 = (readVoltage(pin_hih4030) - 0.958) / 0.0307;
	return rh_at_25 * (1.0546 - 0.00216 * temp_c);
}

void piezo_tone(int tone, int duration) {
	for (long i=0; i < duration * 1000L; i += tone * 2) {
		digitalWrite(pin_piezo, 1);
		delayMicroseconds(tone);
		digitalWrite(pin_piezo, 0);
		delayMicroseconds(tone);
	}
}

void piezo(int on) {
	if (on)
		analogWrite(pin_piezo, 500);
	else 
		analogWrite(pin_piezo, 0);
}

void loop() {
	float temp_c = read_tmp36();
	float hum = read_hum(temp_c);
	Serial.print(temp_c);
	Serial.print(",");
	Serial.print(hum);
	Serial.print(",");
	Serial.print(geiger_usvh());
	Serial.print(",");
	Serial.print(geiger_cpm);
	Serial.println();

	piezo_tone(1915, 100);
	delay(100);
	unsigned long time_ms = millis();
	geiger_loop(time_ms);
}
