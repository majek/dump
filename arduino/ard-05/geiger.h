extern "C" {

void geiger_setup();
void geiger_loop(unsigned long time_ms);
float geiger_usvh();

	extern volatile unsigned long geiger_cpm;

};
