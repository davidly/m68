.text
  .global  main
  .type    main, @function
main:
# (d8,An,Xn) addressing tests

    clr.l %d0
    clr.l %d1
    clr.l %d2
    lea thing1, %a3
    lea thing2, %a4
    lea thing3, %a5

    moveq #97, %d0
    move %d0,(%a3)      | 97 = x61 = a
    addi #1, %d0
    move.l %d0, (%a4)   | 98 = x62 = b
    addi #1, %d0
    move.l %d0, (%a5)   | 99 = x63 = c
    addi #1, %d0
    move.l %d0, 4(%a3)  | 100 = x64 = d
    addi #1, %d0
    move.l %d0, 8(%a3)  | 101 = x65 = e
    addi #1, %d0
    move.l %d0, 4(%a4)  | 102 = x66 = f
    addi #1, %d0
    move.l %d0, 8(%a4)  | 103 = x67 = g
    addi #1, %d0
    move.l %d0, 4(%a5)  | 104 = x68 = h
    addi #1, %d0
    move.l %d0, 8(%a5)  | 105 = x69 = i
    addi #1, %d0
    move.l %d0, -4(%a4) | 106 = x6a = j
    addi #1, %d0
    move.l %d0, -8(%a4) | 107 = x6b = k
    addi #1, %d0
    move.l %d0, -4(%a5) | 108 = x6c = l
    addi #1, %d0
    move.l %d0, -8(%a5) | 109 = x6d = m

    moveq #4, %d0
    move.l 4(%a4,%d0.w), %d1
    move.l %d1, -(%a7)
    lea buffer, %a1
    move.l %a1, -(%a7)
    jsr printf
    adda #8, %a7

    moveq #-4, %d0
    move.l 4(%a4,%d0.l), %d1
    move.l %d1, -(%a7)
    lea buffer, %a1
    move.l %a1, -(%a7)
    jsr printf
    adda #8, %a7

    move.l #0x1234fffc, %d0
    move.l 4(%a4,%d0.w), %d1
    move.l %d1, -(%a7)
    lea buffer, %a1
    move.l %a1, -(%a7)
    jsr printf
    adda #8, %a7

    movea.l #-4, %a0
    move.l 4(%a4,%a0.l), %d1
    move.l %d1, -(%a7)
    lea buffer, %a1
    move.l %a1, -(%a7)
    jsr printf
    adda #8, %a7

    movea.l #0x1234fffc, %a0
    move.l 4(%a4,%a0.w), %d1
    move.l %d1, -(%a7)
    lea buffer, %a1
    move.l %a1, -(%a7)
    jsr printf
    adda #8, %a7

# offsets differ

    moveq #-8, %d0
    move.l 4(%a4,%d0.l), %d1
    move.l %d1, -(%a7)
    lea buffer, %a1
    move.l %a1, -(%a7)
    jsr printf
    adda #8, %a7

    move.l #0x1234fff8, %d0
    move.l 4(%a4,%d0.w), %d1
    move.l %d1, -(%a7)
    lea buffer, %a1
    move.l %a1, -(%a7)
    jsr printf
    adda #8, %a7

    movea.l #-8, %a0
    move.l 4(%a4,%a0.l), %d1
    move.l %d1, -(%a7)
    lea buffer, %a1
    move.l %a1, -(%a7)
    jsr printf
    adda #8, %a7

    movea.l #0x1234fff8, %a0
    move.l 4(%a4,%a0.w), %d1
    move.l %d1, -(%a7)
    lea buffer, %a1
    move.l %a1, -(%a7)
    jsr printf
    adda #8, %a7

    rts

buffer:
    .string "result is %u\n"

thing1:
    .zero 4096
thing2:
    .zero 4096
thing3:
    .zero 4096

