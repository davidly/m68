#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DIGITS_TO_FIND 200 //9009;
int a[ DIGITS_TO_FIND ];

int main() {
  int high = DIGITS_TO_FIND;
  int x = 0;
  int n;

  for (n = high - 1; n > 0; --n) {
      a[n] = 1;
  }

  a[1] = 2, a[0] = 0;
  while (high > 9) {
      n = high--;
      while (--n) {
          a[n] = x % n;

          x = 10 * a[n-1] + x/n;
      }
      printf("%d", x);
  }

  printf( "\ndone\n" );

  return 0;
}