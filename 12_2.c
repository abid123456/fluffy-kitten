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

int init;

int ff (int n)
{
    if (n == 1) return 1;
    if (!dr(n, 2)) return n / 2 * ff(n - 1);
    else return n * ff(n - 1);
}

int main()
{
	int n, l;
	char type;
	int i, ir;
	char str[101];
	int a1, a2, i1, i2;

	scanf("%d", &n);
	printf("%d\n", ff(n));
}
