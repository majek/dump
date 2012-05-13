#include <LiquidCrystal.h>
LiquidCrystal lcd(3,4,5,6,7,8);


volatile unsigned long sum = 0;

#define RBSIZE 60
volatile unsigned rbuffer[RBSIZE] = {0};
volatile unsigned rindex = 0;

void count_pulse() {
	rbuffer[rindex] += 1;
	sum += 1;
}

void count_pulse_original() {
	detachInterrupt(0);
	rbuffer[rindex] += 1;
	sum += 1;
	while (digitalRead(2) == 0) {};
	attachInterrupt(0, count_pulse_original, FALLING);
}


void setup() {
	Serial.begin(9600);
	Serial.println("#uSv/h,cpm,cps");
	
	pinMode(2, INPUT);
	digitalWrite(2, HIGH);

	attachInterrupt(0, count_pulse_original, FALLING);
	//attachInterrupt(0, count_pulse, FALLING);

	lcd.begin(16, 2);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Geiger counter");
	lcd.print("            v03");

	delay(400);
}

static inline float cpm_to_usv(unsigned long cpm) {
	return (float)cpm * 0.00812;
}

char okay = 0;

static inline char judge(float usvh) {
	if (usvh < 0.081) {
		return okay ? ' ' : '?';
	}
	if (usvh < 5.70) {
		return '.';
	}
	return '!';
}

void loop() {
	unsigned long time_ms = millis();
	unsigned long idx1 = (time_ms / 1000) % RBSIZE;
	if (idx1 != rindex) {
		if (idx1 < rindex) {
			okay = 1;
		}

		unsigned long prev_sum = sum;
		sum -= rbuffer[idx1];
		rbuffer[idx1] = 0;
		unsigned prev_idx = rindex;
		rindex = idx1;
		unsigned counter = rbuffer[prev_idx];
		
		char buf[32];
		snprintf(buf, sizeof(buf), "%.4f,%lu,%u",
			 cpm_to_usv(prev_sum), prev_sum, counter);
		Serial.println(buf);
	} else {
		delay(250 - (time_ms % 250));
	}

	char buf1[17], buf2[17];
	snprintf(buf1, sizeof(buf1), "cpm:%5lu%4u#%02u", sum, rbuffer[rindex], rindex);
	float usvh = cpm_to_usv(sum);
	snprintf(buf2, sizeof(buf2), "uSv/h:%9.4f%c", usvh, judge(usvh));

	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print(buf1);
	lcd.setCursor(0, 1);
	lcd.print(buf2);
}
