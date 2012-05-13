//&>/dev/null;x="${0%.*}";[ ! "$x" -ot "$0" ]||(rm -f "$x";cc -o "$x" "$0")&&"./$x" $*;exit

#include <stdio.h>

typedef char arr[5];

void print_arr(arr *a) {
	printf("%p = [%c,%c,%c,%c,%c]\n", *a, (*a)[0],(*a)[1],(*a)[2],(*a)[3],(*a)[4]);
}

void arr_by_value(arr a)
{
	a[1] = 'X';
}


#if 0
/* This is illegal: */
arr arr_return_value(arr a)
{
	a[1] = 'Y';
	return a;
}
#endif


int main() {
	arr a = "main!";
	printf("By value:\n");
	print_arr(&a);
	arr_by_value(a);
	print_arr(&a);

#if 0
	printf("\nAs return value:\n");
	print_arr(&a);
	arr_return_value(a);
	print_arr(&a);
#endif

	return 0;
}
