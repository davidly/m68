#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#pragma GCC optimize ("O0")

extern int main() {
  const int DIGITS_TO_FIND = 200; //9009;

  int N = DIGITS_TO_FIND;
  char buf[ 128 ];
  int x = 0;
  int a[ DIGITS_TO_FIND ];

  for (int n = N - 1; n > 0; --n) {
      a[n] = 1;
  }

  a[1] = 2, a[0] = 0;
  while (N > 9) {
      int n = N--;
      while (--n) {
          a[n] = x % n;

          x = 10 * a[n-1] + x/n;
      }
      printf("%d", x);
      fflush(stdout);
  }

  printf( "\ndone\n" );

  return 0;
}
