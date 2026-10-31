#include <Adafruit_NeoPixel.h>

#define PIN 5
#define PIXELS 60

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELS, PIN, NEO_GRB + NEO_KHZ800);


unsigned char buf[(PIXELS + 1)* 3] = {0};
int buf_pos = 0;

void setup() {
	strip.begin();
	strip.show();
	Serial.begin(115200);//, 0x2E); //SERIAL_8E2);
}

void show() {
	int i;
	for (i = 1; i < PIXELS+1; i++) {
		strip.setPixelColor(i-1, buf[i*3+0], buf[i*3+1], buf[i*3+2]);
	}
	strip.show();
}

void loop() {
        while (Serial.available() > 0) {
		unsigned char c = Serial.read();
		if (buf_pos < 3)
			if (c != buf_pos)
				continue;
		buf[buf_pos++] = c;
		if (buf_pos == sizeof(buf)) {
			buf_pos = 0;
			show();
		}
        }
}
