.data

str_failures: .dc.b 'chk trap invocations (4 expected): %lu',13,10,0,$
buffer: .dc.b 'tchk completed with great success',13,10,0,$

ex_vector: .dc.w 6
ex_new: .dc.l 0
ex_old: .dc.l 0

.globl _main
.text
_main:

* install a chk trap handler that counts failures
    move.l #ex_new, a0
    move.l #chktrap, (a0)
    move.l #61, d0
    move.l #ex_vector, d1
    trap 2

    lea _printf, a3

    clr.l d7         * count of failures here
    move.l #8, d0
    move.l #6, d1
    move.l #3, d2
    move.l #6, d6
    move.l #$ffffffed, d3

    chk.w d1, d2
    chk.l d1, d2     * I can't get the assembler to emit a .l vesion of this. 68000 docs are ambiguous.

    chk.w d6, d2
    chk.w d2, d6     * fail!

    chk.w d6, d1
    chk.w d1, d6

    chk.w d1, d0     * fail!
    chk.l d1, d0     * fail!

    chk.l d1, d3     * fail!
    chk.w d0, d6

    move.l d7, -(a7)
    lea str_failures, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #8, a7

    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0         * exit code of 0
    rts

chktrap:
    add.l #1, d7
    rte

