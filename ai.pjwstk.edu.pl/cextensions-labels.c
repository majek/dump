/*
gcc -Wall cextensions-labels.c -o cextensions-labels && ./cextensions-labels
*/
#include <stdio.h>
#include <assert.h>

int main(){
	int value  = 2;
	assert(value >= 0 && value < 4);
	const void *labels[] = {&&val_0, &&val_1, &&val_2, &&val_3};
	goto *labels[value];
	assert(0);
	val_0:
		printf("The value is 0\n");
		goto end;
	val_1:
		printf("The value is 1\n");
		goto end;
	val_2:
		printf("The value is 2\n");
		goto end;
	val_3:
		printf("The value is 3\n");
		goto end;
	end:
	return(0);
}

