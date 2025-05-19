.globl _main
.text
_main:

* tests for addx.width / subx.width -(ay), -(ax)  (x is op_reg, y is ea_reg)

    clr.l d0
    clr.l d1
    lea _printf, a3

* addx/subx byte
    clr.l d0
    clr.l d1
    move #0, ccr
    lea p1abyte, a0
    lea p1bbyte, a1
    subx.b -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.b (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    lea p1abyte, a0
    lea p1bbyte, a1
    addx.b -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.b (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    lea p1abyte, a0
    lea p1bbyte, a1
    subx.b -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.b (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    lea p1abyte, a0
    lea p1bbyte, a1
    addx.b -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.b (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* addx/subx word
    clr.l d0
    clr.l d1
    move #0, ccr
    lea p2aword, a0
    lea p2bword, a1
    subx.w -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.w (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    lea p2aword, a0
    lea p2bword, a1
    addx.w -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.w (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    lea p2aword, a0
    lea p2bword, a1
    subx.w -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.w (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    lea p2aword, a0
    lea p2bword, a1
    addx.w -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.w (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* addx/subx long
    clr.l d0
    clr.l d1
    move #0, ccr
    lea p4along, a0
    lea p4blong, a1
    subx.l -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.l (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    lea p4along, a0
    lea p4blong, a1
    addx.l -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.l (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    lea p4along, a0
    lea p4blong, a1
    subx.l -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.l (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    lea p4along, a0
    lea p4blong, a1
    addx.l -(a0), -(a1)
    move sr, d1
    move.l d1, -(a7)
    move.l (a1), d0
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    rts

buffer: .dc.b '  result %lx flags %lx',10,0
abyte: .dc.b $66
bbyte: .dc.b $77
aword: .dc.w $3333
bword: .dc.w $4444
along: .dc.l $88888888
blong: .dc.l $99999999

p1abyte .equ (abyte+1)
p1bbyte .equ (bbyte+1)
p2aword .equ (aword+2)
p2bword .equ (bword+2)
p4along .equ (along+4)
p4blong .equ (blong+4)

