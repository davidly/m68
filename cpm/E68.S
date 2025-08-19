* compute the first 192 digits of e
* replicates this C code:
*    #define DIGITS_TO_FIND 200 /*9009*/
*    int main() {
*      int N = DIGITS_TO_FIND;
*      int x = 0;
*      int a[ DIGITS_TO_FIND ];
*      int n;
*    
*      for (n = N - 1; n > 0; --n)
*          a[n] = 1;
*    
*      a[1] = 2, a[0] = 0;
*      while (N > 9) {
*          n = N--;
*          while (--n) {
*              a[n] = x % n;
*              x = 10 * a[n-1] + x/n;
*          }
*          printf("%d", x);
*      }
*    
*      printf( "\ndone\n" );
*      return 0;
*    }

* array is in a2
* & a[n] is in a3
* integer constant 10 is in d6
* n is in d5
* x is in d4
* N is in d3

digits equ 200
.comm array,digits*2

.text
_start:
    move.l #$10001, d1      * two word values of 1
    move.l #digits, d0
    add.l d0, d0
    subq.l #4, d0
    movea.l #array, a2

  ainit:
    move.l d1, (a2,d0)      * initialize 2 array entries at a time
    subq.l #4, d0
    bne ainit

    move.w #2, 2(a2)
    clr.w (a2)

    moveq.l #10, d6         * integer constant 10 for faster multiplies
    move.l #digits, d3      * N = DIGITS_TO_FIND
    clr.l d4                * x = 0

  outer:
    cmpi.w #9, d3           * while ( N > 9 )
    beq loop_done
    subq.l #1, d3           * N = N - 1
    move.l d3, d5           * n = N    
    move.l a2, a3           * move pointer to a[0] to a3
    add.l d5, a3            * add n twice because each element is 2 bytes
    add.l d5, a3            * a3 now points to a[n]

  inner:
    divu d5, d4             * x / n also x % n. quotient in d4.w
    swap d4                 * remainder now in d4.w
    move.w d4, (a3)         * a[n] = x % n
    clr.w d4                * zero out the remainder portion
    swap d4                 * quotient back in d4.w
    move.w -(a3), d0        * load a[ n - 1 ]
    mulu.w d6, d0           * 10 * a[ n - 1 ]
    add.w d0, d4            * x = 10 * a[ n - 1 ] + x / n
    subq.l #1, d5           * while ( --n )
    bne inner

  prdigit:
    move.w d4, d0
    jsr show_num
    bra outer

  loop_done:
    move.l #done_string, d1
    move.l #9, d0
    trap #2

    clr.l d0                * exit code of 0
    rts

show_num:                   * print the unsigned number in d0.w
    movea.l #num_buf, a0

  next_digit:
    tst.w d0
    beq swap_digits
    andi.l #$ffff, d0
    divu #10, d0
    move.l d0, d1
    swap d1
    add.w #48, d1
    move.b d1, (a0)+
    bra next_digit

  swap_digits:
    movea.l #num_buf, a1
    move.b #'$', (a0)
    subq.l #1, a0

  next_swap:
    cmpa.l a1, a0
    ble show_string
    move.b (a0), d0
    move.b (a1), (a0)
    move.b d0, (a1)+
    subq.l #1, a0
    bra next_swap

  show_string:
    moveq.l #9, d0          * 9 means print a $ terminated string
    move.l #num_buf, d1
    trap #2
    rts

.data

done_string: .dc.b 13,10,'done',13,10,'$'
num_buf: .dc.b '            '

.end

