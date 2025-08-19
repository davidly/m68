* the BYTE magazine sieve benchmark in 68000 assembly
*    
*    #define SIZE 8190
*    
*    char flags[SIZE+1];
*    
*    int main()
*            {
*            int i,k;
*            int prime,count,iter;
*    
*            for (iter = 1; iter <= 10; iter++) {    /* do program 10 times */
*                    count = 0;                      /* initialize prime counter */
*                    for (i = 0; i <= SIZE; i++)     /* set all flags TRUE */
*                            flags[i] = TRUE;
*                    for (i = 0; i <= SIZE; i++) {
*                            if (flags[i]) {         /* found a prime */
*                                    prime = i + i + 3;      /* twice index + 3 */
*                                    for (k = i + prime; k <= SIZE; k += prime)
*                                            flags[k] = FALSE;       /* kill all multiples */
*                                    count++;                /* primes found */
*                                    }
*                            }
*                    }
*            printf("%d primes.\n",count);           /*primes found in 10th pass */
*            return 0;
*            }

* flags a2
* k d7
* iter d6
* i d5
* count d4
* prime d3
* #size d2

size equ 8190               
sizedw equ (size+2)/4
.globl _flags
.comm _flags,size+2         * multiple of 4 and includes 2 bonus bytes per algorithm and to stop inner loop

.text
_start:
    clr.l d6
  nextiter:
    movea.l #_flags, a2
    move.l #$01010101, d0
    move.l #sizedw-1, d1    * -1 because dbf loops until d1 is -1

  finit:                    * initialize _flags to true, 4 bytes at a time
    move.l d0, (a2)+
    dbf d1, finit

    movea.l #_flags, a2
    move.b #1, #size(a2)
    move.l #size, d2
    clr.l d4
    movea.l a2, a3

  outer:
    tst.b (a3)+             * this may be offset 8191, beyond the array. but a non-zero beyond_flag resides there
    beq outer
    move.l a3, d5           * find d5's array offset based on where a3 ended minus one
    sub.l a2, d5
    subq.l #1, d5           * a3 ends up pointing at the next entry, not the first non-zero entry
    cmp.l d2, d5
    bge alldone

    moveq.l #3, d3          * prime = 3
    add.l d5, d3            * prime += i
    add.l d5, d3            * prime += i
    move.l d3, d7           * k = prime
    add.l d5, d7            * k += i
    cmp.l d2, d7            * check before the inner loop to save code at the start of that loop
    bgt inccount

  inner:
    clr.b (a2,d7)           * flags[k] = FALSE
    add.l d3, d7            * k += prime
    cmp.l d2, d7            * k <= SIZE
    ble inner

  inccount:
    addq.l #1, d4           * count++
    bra outer
    
  alldone:
    addq.l #1, d6
    cmpi.l #10, d6
    bne nextiter

    move.l d4, d0
    jsr show_num

    move.l #primes_string, d1
    move.l #9, d0
    trap #2

    clr.l d0                * exit code of 0
    rts

show_num:                   * print the unsigned number in d0.w. d0, d1, a0, a1 are trashed
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
    move.l #9, d0
    move.l #num_buf, d1
    trap #2
    rts

.data

primes_string: .dc.b ' primes',13,10,'$'
num_buf: .dc.b '            '

.end

