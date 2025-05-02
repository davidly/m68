.data

vals: .dc.b 'values: %lx, %x',13,10,0
buffer: .dc.b 'tmovep completed with great success',13,10,0

l_buf:
dc.b 32
dc.b 32
dc.b 32
dc.b 32
dc.b 32
dc.b 32
dc.b 32
dc.b 32

w_buf:
dc.b 32
dc.b 32
dc.b 32
dc.b 32

dc.b 13
dc.b 10
dc.b 0

.globl _main
.text
_main:
    move.l #$69747a79, d3   * itzy
    move.l #$4955, d4       * IU

    move.l #w_buf, a0       * test writing from registers to memory
    movep.l d3, -8(a0)      * test relative addressing
    move.l #l_buf, a0
    movep.w d4, 8(a0)       * same

    move.l #l_buf, a0       * print out memory values
    move.l a0, -(a7)
    jsr _printf
    adda #4, a7

    move.l #l_buf, a0       * now load the values back into registers
    movep.l 0(a0), d5
    move.l #w_buf, a0
    movep.w 0(a0), d6

    move.w d6, -(a7)        * and print out the values in hex
    move.l d5, -(a7)
    lea vals, a0
    move.l a0, -(a7)
    jsr _printf
    adda #10, a7

    lea buffer, a0          * say good-bye
    move.l a0, -(a7)
    jsr _printf
    adda #4, a7

    clr.w d0                * standard cp/m bdos exit call
    clr.l d1
    trap 2


