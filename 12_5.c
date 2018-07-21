#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

void swap(int *a, int *b)
{
	int t;
	t = *a;
	*a = *b;
	*b = t;
}

int reverse(int x)
{
	int temp = x;
	int ret = 0;
	while (temp > 0) {
		ret = (ret * 10) + temp - (temp / 10) * 10;
		temp /= 10;
	}
	return ret;
}

int dr(int a, int b)
{
	return a - (a / b) * b;
}

int isprime(int x)
{
	int f, i, a;

	if (x < 2) return 0;
	if (x < 4) return 1;
	a = floor(sqrt(x));
	for (i = 2; i <= a; i++) {
		if (!dr(x, i)) return 0;
	}
	return 1;
}

int a, b;
char str[40] = {'\0'};
char *ptr;

void f(int n)
{
    /*printf("f(%d) called\n", n);
    getchar();*/
    if (!n) return;
    if (!dr(n, 2)) *ptr = '0';
    else *ptr = '1';
    ptr++;
    n /= 2;
    f(n);
}

int main()
{
	int n, l;
	char type;
	int i, ir;
	int k, x;

	ptr = str + 1;
	scanf("%d", &n);
    f(n);
    while (*--ptr) putchar(*ptr);
    puts("");
}
