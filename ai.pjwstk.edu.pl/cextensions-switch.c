/*
gcc -Wall cextensions-switch.c -o cextensions-switch && ./cextensions-switch
*/
#include <stdio.h>
#include <assert.h>

int main(){
	int value  = 7;
	switch(value){
	case 0 ... 5:
		printf("The value is 0 - 5\n");
		break;
	case 6 ... 10:
		printf("The value is 6 - 10\n");
		break;
	default:
		printf("The value is ?\n");
	}
	return(0);
}

