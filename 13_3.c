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
/* --- library ends, main code starts --- */

int n;
const int up = 1, down = -1;
typedef struct {
    char occurred[10];
    char str[10];
} strstruct;

void f(strstruct ss, int nd, int dir)
{
    int i;
    char arr2[10];
    strstruct ss2 = ss;

    if (dir == up) {
        for (i = last; i <= n; i++) {
            if (arr[i]) continue;
            arr2[i] = 1;
            printf("%d", i);
            f(arr2, i, down);
        }
    } else {
        for (i = last; i >= 1; i--) {
            if (arr[i]) continue;
            arr2[i] = 1;
            printf("%d", i);
            f(arr2, i, up);
        }
    }
    puts("");
}

strstruct init(int x)
{
    int i;
    strstruct ret;

    for (i = 0; i < 10; i++) ret.occurred[i] = 0;
    ret.occurred[x] = 1;
    ret.str[0] = x;
    for (i = 1; i <= 9; i++) ret.str[i] = 0;
    return ret;
}

int main()
{
	int i;
	char mainstr[10] = {0};
	char mainstr_z[10] = {0};

	printf("%d", sizeof(strstruct));
	scanf("%d", &n);
    if (n == 1) {
        puts("1");
        return 0;
    }

}
