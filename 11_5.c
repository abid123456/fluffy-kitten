#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

int init;

int main()
{
	int n, q, l;
	int i, ir;
	char str[101];
	int a1, a2, i1, i2;

	scanf("%s", str);
	l = strlen(str);
	for (i = 0; i < l; i++) {
        if ('a' <= str[i] && str[i] <= 'z')
            putchar(str[i] - 0x20);
        else putchar(str[i] + 0x20);
	}
	puts("");
}
