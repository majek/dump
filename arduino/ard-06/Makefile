ARDUINO_DIR = /home/marek/src/arduino-1.0

#TARGET       = temp
#BOARD_TAG    = uno
#ARDUINO_PORT = /dev/ttyACM0
TARGET       = lilypad
BOARD_TAG    = lilypad328
ARDUINO_PORT = /dev/ttyUSB0

ARDUINO_LIBS = Wire Wire/utility
AVR_TOOLS_PATH = /usr/bin
AVRDUDE_CONF   = /etc/avrdude.conf


include /home/marek/src/arduino-mk-0.8/Arduino.mk


cat:
	stty -F /dev/ttyUSB0 cs8 115200 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
	cat /dev/ttyUSB0
