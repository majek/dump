#include <Arduino.h>

int pin_geiger = 2; // ie: digital pin 2
int int_geiger = 0; // ie: digital pin 2 - interrupt 0

#define RBSIZE 60
volatile unsigned rbuffer[RBSIZE] = {0};
volatile unsigned geiger_idx = 0;
volatile unsigned long geiger_cpm = 0;

static void count_pulse() {
	rbuffer[geiger_idx] += 1;
	geiger_cpm += 1;
}

void geiger_setup() {
	pinMode(pin_geiger, INPUT);
	digitalWrite(pin_geiger, HIGH);

	attachInterrupt(int_geiger, count_pulse, FALLING);
}

// shall be run at least every sec
void geiger_loop(unsigned long time_ms) {
	unsigned new_idx = (time_ms / 1000L) % RBSIZE;
	if (geiger_idx != new_idx) {
		geiger_cpm -= rbuffer[new_idx];
		rbuffer[new_idx] = 0;
		geiger_idx = new_idx;
	}
}

float geiger_usvh() {
	// cpm to usv/h
	return (float)geiger_cpm * 0.00812;
}
