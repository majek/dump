#include <Adafruit_NeoPixel.h>

// 5 for lilly
#define PIN 2
#define PIXELS 256

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELS, PIN, NEO_GRB + NEO_KHZ800);


unsigned char buf[PIXELS * 3];
int buf_pos = 0;

static void refresh_led() {
	unsigned char *b = buf;
	unsigned i;
	for (i = 0; i < PIXELS; i++) {
		strip.setPixelColor(i, b[i*3], b[i*3+1], b[i*3+2]);
	}
	strip.show();
}



void setup() {
     int i;
     for (i = 0; i < PIXELS; i++) {
         buf[i*3+0]= 0;
         buf[i*3+1]= 0;
         buf[i*3+2]= 1;
}

	pinMode(PIN, OUTPUT);
	strip.begin();
	refresh_led();

	Serial.begin(115200, SERIAL_8N1);
}

static unsigned char serial_read() {
	while (Serial.available() == 0) {};
	return Serial.read();
}

unsigned count;
void loop() {
	while (1) {
		if (serial_read() == 0xFF)
			break;
	}

	if (serial_read() != 0xFF || serial_read() != 0xFF)
		return;

	count += 1;
        digitalWrite(13, count % 2 == 0 ? HIGH : LOW);

	unsigned i;
	for (i=0; i < sizeof(buf); i++) {
		buf[i] = serial_read();
                if (i >= 2 && buf[i-2] == 0xff && buf[i-1] == 0xff && buf[i-0] == 0xff) {
                      buf[i-2] = 0;
                      buf[i-1] = 0;
                      buf[i-0] = 0;
                        break;
                }
	}

	refresh_led();
}
