.data

.text
  .global  main
  .type    main, @function
main:

# install a chk trap handler that counts failures

# there is no memory protection for the vector table, so just write to it
     move.l #0x18, %a0
     move.l #chktrap, (%a0)

# this is how it's done on CP/M 68K, and it'd work here in the emulator
#    move.l #ex_new, %a0
#    move.l #chktrap, (%a0)
#    move.l #61, %d0
#    move.l #ex_vector, %d1
#    trap #2

    clr.l %d7          | count of failures here
    move.l #8, %d0
    move.l #6, %d1
    move.l #3, %d2
    move.l #6, %d6
    move.l #0xffffffed, %d3

    chk.w %d1, %d2
# 68020+    chk.l %d1, %d2

    chk.w %d6, %d2
    chk.w %d2, %d6     | fail!

    chk.w %d6, %d1
    chk.w %d1, %d6

    chk.w %d1, %d0     | fail!
# 68020    chk.l %d1, %d0     | fail!

# 68020    chk.l %d1, %d3     | fail!
    chk.w %d0, %d6

    move.l %d7, -(%a7)
    lea str_failures, %a0
    move.l %a0, -(%a7)
    jsr printf
    adda #8, %a7

    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr printf
    adda #4, %a7

    clr.l %d0          | exit code of 0
    rts

chktrap:
    add.l #1, %d7
    rte

ex_vector: .word 6
ex_new: .long 0
ex_old: .long 0

str_failures: .string "chk trap invocations (2 expected): %lu\n"
buffer: .string "tchk completed with great success\n"


