.text
  .global  main
  .type    main, @function
main:
# tests for addx.width / subx.width -(ay), -(ax)  (x is op_reg, y is ea_reg)

    clr.l %d0
    clr.l %d1
    lea printf, %a3

# addx/subx byte
    move #0, %ccr
    lea abyte + 1, %a0
    lea bbyte + 1, %a1
    subx.b -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.b (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    lea abyte + 1, %a0
    lea bbyte + 1, %a1
    addx.b -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.b (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    lea abyte + 1, %a0
    lea bbyte + 1, %a1
    subx.b -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.b (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    lea abyte + 1, %a0
    lea bbyte + 1, %a1
    addx.b -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.b (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# addx/subx word
    move #0, %ccr
    lea aword + 2, %a0
    lea bword + 2, %a1
    subx.w -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.w (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    lea aword + 2, %a0
    lea bword + 2, %a1
    addx.w -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.w (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    lea aword + 2, %a0
    lea bword + 2, %a1
    subx.w -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.w (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    lea aword + 2, %a0
    lea bword + 2, %a1
    addx.w -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.w (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7


# addx/subx long
    move #0, %ccr
    lea along + 4, %a0
    lea blong + 4, %a1
    subx.l -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    lea along + 4, %a0
    lea blong + 4, %a1
    addx.l -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    lea along + 4, %a0
    lea blong + 4, %a1
    subx.l -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    lea along + 4, %a0
    lea blong + 4, %a1
    addx.l -(%a0), -(%a1)
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l (%a1), %d0
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    rts

buffer:
    .string "  result %x flags %x\n"

abyte:
    .byte 0x66
bbyte:
    .byte 0x77
aword:
    .word 0x3333
bword:
    .word 0x4444
along:
    .long 0x88888888
blong:
    .long 0x99999999


