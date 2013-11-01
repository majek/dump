
obj-m = nullderef.o

M=$(shell pwd)

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(M) modules 
#KBUILD_VERBOSE=1 EXTRA_CFLAGS="-fomit-frame-pointer -no-pg" CONFIG_FRAME_POINTER="n"

