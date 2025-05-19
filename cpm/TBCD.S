.globl _main
.text
_main:

* abcd tests

    clr.l d0
    clr.l d1
    move #0, ccr
    moveq #3, d0
    moveq #4, d1
    abcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #3, d0
    moveq #4, d1
    abcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    moveq #$68, d0
    moveq #$34, d1
    abcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #$68, d0
    moveq #$31, d1
    abcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #$68, d0
    moveq #$21, d1
    abcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #$68, d0
    moveq #$71, d1
    abcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #0, d0
    moveq #0, d1
    abcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    moveq #0, d0
    moveq #0, d1
    abcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    lea aftera, a0
    lea afterb, a1
    abcd -(a0), -(a1)
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.b (a1), d0
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

* sbcd tests

    clr.l d0
    clr.l d1
    move #0, ccr
    moveq #3, d0
    moveq #4, d1
    sbcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #3, d0
    moveq #4, d1
    sbcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    moveq #$68, d0
    moveq #$34, d1
    sbcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #$68, d0
    moveq #$31, d1
    sbcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #$68, d0
    moveq #$21, d1
    sbcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #$68, d0
    moveq #$71, d1
    sbcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #0, d0
    moveq #0, d1
    sbcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    moveq #0, d0
    moveq #0, d1
    sbcd d1, d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    lea aftera, a0
    lea afterb, a1
    sbcd -(a0), -(a1)
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.b (a1), d0
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

* nbcd
    clr.l d0
    clr.l d1
    move #0, ccr
    moveq #3, d0
    nbcd d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #3, d0
    nbcd d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    moveq #$68, d0
    nbcd d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #$68, d0
    nbcd d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #$68, d0
    nbcd d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #$68, d0
    nbcd d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    moveq #0, d0
    nbcd d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    moveq #0, d0
    nbcd d0
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    lea aftera, a0
    nbcd (a0)
    move sr, d1
    andi #$1f, d1
    move.l d1, -(a7)
    move.b (a0), d0
    move.l d0, -(a7)
    pea buffer
    jsr _printf
    adda #12, a7

    rts

buffer: .dc.b 'result is %lx flags %lx',10,0

valuea:
    .dc.b $10,$34
aftera:
valueb:
    .dc.b $20,$27
afterb:
    .dc.b $10

.end
