#include <LiquidCrystal.h>
LiquidCrystal lcd(3,4,5,6,7,8);

unsigned long counter = 0;
#define DELAYSZ 10
unsigned long tick_micros = 0;
unsigned long delays[DELAYSZ] = {0};

void count_pulse() {
    unsigned long now = micros();
    unsigned long delay = now - tick_micros;
    tick_micros = now;
    delays[counter % DELAYSZ] = delay / 4;
    counter += 1;
}


void setup() {
    tick_micros = micros();
    attachInterrupt(0, count_pulse, FALLING);

    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Radiation Sensor 12345");
    //lcd.setCursor(0,1);
    //lcd.print("Board - Arduino");
    delay(1200);
}


unsigned long snapshot_cpm = 0;
float favg1 = 0., favg5 = 0.;
unsigned long avg1=0, avg5=0;

void print_stuff(unsigned long _d) {
    unsigned long c = counter-1;
    char buf1[17];
    char buf2[17];
    snprintf(buf1, sizeof(buf1), "%8lu %lu", delays[c % DELAYSZ], avg1);
    snprintf(buf2, sizeof(buf2), "cpm=%4lu %-3f", snapshot_cpm, favg1);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(buf1);
    lcd.setCursor(0,1);
    lcd.print(buf2);

//    lcd.setCursor(8,1);
//    lcd.print(favg1);
}


unsigned long last_counter = 0;
void snapshot(unsigned long _d) {
    unsigned long delta = counter - last_counter;
    last_counter = counter;
    snapshot_cpm = delta * 6;
}


#define FSHIFT   11                /* nr of bits of precision */
#define FIXED_1  (1<<FSHIFT)    /* 1.0 as fixed-point */
#define LOAD_FREQ (5*HZ)      /* 5 sec intervals */
#define EXP_1  1884           /* 1/exp(5sec/1min) as fixed-point */
#define EXP_5  2014           /* 1/exp(5sec/5min) */
#define EXP_15 2037           /* 1/exp(5sec/15min) */
 
#define CALC_LOAD(load,exp,n) \
   load *= exp; \
   load += n*(FIXED_1-exp); \
   load >>= FSHIFT;

float exp_1 = exp(-5./(60*1.));

unsigned long last_avg_counter = 0;
void count_avg(unsigned long _delta) {
    unsigned long delta = counter - last_avg_counter;
    last_avg_counter = counter;
    unsigned long cpm = delta * 12;

    CALC_LOAD(avg1, EXP_1, cpm);
    CALC_LOAD(avg5, EXP_5, cpm);
    //favg1 = avg1 / 2048.;
    //favg5 = avg5 / 2048.;
    favg1 = favg1*exp_1 + cpm*(1. - exp_1);
}


#define RUN_AFTER(timer, foo, interal) \
    if (now - timer > interal) {       \
	foo(now - timer);	       \
	timer = now;		       \
    }

unsigned long t1, t2, t3;
void loop() {
    unsigned long now = millis();
    RUN_AFTER(t1, count_avg,      5000UL);
    RUN_AFTER(t2, snapshot,      10000UL);
    RUN_AFTER(t3, print_stuff,     100UL);
}
