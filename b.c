//&>/dev/null;x="${0%.*}";[ ! "$x" -ot "$0" ]||(rm -f "$x";cc -o "$x" "$0")&&"./$x" $*;exit

#include <stdio.h>

int main() {
        int key= -(0x1);
        char name[13];
	unsigned char a = 255;
	unsigned char b = 1;
	unsigned char c = b - a;
        sprintf(name, "SYSV%08x", key);
        printf("%i %i %i\n", a, b, c);
        return 0;
}
