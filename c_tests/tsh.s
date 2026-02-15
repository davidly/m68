    .text
    .globl main

    .equ __NR_exit,  93
    .equ __NR_write, 64
    .equ STDOUT,     1

    .equ CCR_MASK,   0x001F      | X N Z V C bits

| ------------------------------------------------------------
| Syscall helpers (preserve %d0-%d3/%a0)
| ------------------------------------------------------------
.macro SYSCALL_WRITE buf, len
    movem.l %d0-%d3/%a0, -(%sp)
    move.l  #__NR_write, %d0
    move.l  #STDOUT,     %d1
    lea     \buf,        %a0
    move.l  %a0,         %d2
    move.l  #\len,       %d3
    trap    #0
    movem.l (%sp)+, %d0-%d3/%a0
.endm

.macro SYSCALL_EXIT code
    move.l  #__NR_exit, %d0
    move.l  #\code,     %d1
    trap    #0
.endm

.macro FAIL msg, msglen
    SYSCALL_WRITE \msg, \msglen
    SYSCALL_EXIT  1
.endm

| ------------------------------------------------------------
| Test harness helpers
| ------------------------------------------------------------

.macro SET_CCR imm8
    move.w  #\imm8, %ccr
.endm

| Save CCR (low 5 bits) immediately after op-under-test into %d7.
.macro SAVE_CCR
    move.w  %sr, %d7
    and.w   #CCR_MASK, %d7
.endm

.macro ASSERT_D0_EQ imm32, failmsg, failmsglen
    cmp.l   #\imm32, %d0
    beq.s   1f
    FAIL \failmsg, \failmsglen
1:
.endm

.macro ASSERT_SAVED_CCR_EQ imm16, failmsg, failmsglen
    cmp.w   #\imm16, %d7
    beq.s   1f
    FAIL \failmsg, \failmsglen
1:
.endm

.macro MASK_D0_8
    and.l   #0xFF, %d0
.endm

.macro MASK_D0_16
    and.l   #0xFFFF, %d0
.endm

| ------------------------------------------------------------
| Tests
| ------------------------------------------------------------
main:

| 1) LSL.B #1, %d0 : 0x80 -> 0x00
| Expect CCR = 0x15 (X=1,Z=1,C=1)
    SET_CCR 0x00
    moveq   #0, %d0
    move.b  #0x80, %d0
    lsl.b   #1, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0x00, msg_fail_1_val, msg_fail_1_val_len
    ASSERT_SAVED_CCR_EQ 0x15, msg_fail_1_ccr, msg_fail_1_ccr_len

| 2) ASL.B #1, %d0 : 0x40 -> 0x80
| Expect CCR = 0x0A (N=1,V=1)
    SET_CCR 0x00
    moveq   #0, %d0
    move.b  #0x40, %d0
    asl.b   #1, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0x80, msg_fail_2_val, msg_fail_2_val_len
    ASSERT_SAVED_CCR_EQ 0x0A, msg_fail_2_ccr, msg_fail_2_ccr_len

| 3) ASR.B #1, %d0 : 0x81 -> 0xC0
| Expect CCR = 0x19 (X=1,N=1,C=1)
    SET_CCR 0x00
    moveq   #0, %d0
    move.b  #0x81, %d0
    asr.b   #1, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0xC0, msg_fail_3_val, msg_fail_3_val_len
    ASSERT_SAVED_CCR_EQ 0x19, msg_fail_3_ccr, msg_fail_3_ccr_len

| 4) LSR.B #1, %d0 : 0x01 -> 0x00
| Expect CCR = 0x15 (X=1,Z=1,C=1)
    SET_CCR 0x00
    moveq   #0, %d0
    move.b  #0x01, %d0
    lsr.b   #1, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0x00, msg_fail_4_val, msg_fail_4_val_len
    ASSERT_SAVED_CCR_EQ 0x15, msg_fail_4_ccr, msg_fail_4_ccr_len

| 5) ROL.B #1, %d0 with X preset = 1 : 0x81 -> 0x03
| Expect CCR = 0x11 (X=1,C=1)
    SET_CCR 0x10
    moveq   #0, %d0
    move.b  #0x81, %d0
    rol.b   #1, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0x03, msg_fail_5_val, msg_fail_5_val_len
    ASSERT_SAVED_CCR_EQ 0x11, msg_fail_5_ccr, msg_fail_5_ccr_len

| 6) ROXL.B #1, %d0 with X=1 : 0x7F -> 0xFF
| Expect CCR = 0x08 (N=1)
    SET_CCR 0x10
    moveq   #0, %d0
    move.b  #0x7F, %d0
    roxl.b  #1, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0xFF, msg_fail_6_val, msg_fail_6_val_len
    ASSERT_SAVED_CCR_EQ 0x08, msg_fail_6_ccr, msg_fail_6_ccr_len

| 7) ROXR.B #1, %d0 with X=1 : 0x00 -> 0x80
| Expect CCR = 0x08 (N=1)
    SET_CCR 0x10
    moveq   #0, %d0
    move.b  #0x00, %d0
    roxr.b  #1, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0x80, msg_fail_7_val, msg_fail_7_val_len
    ASSERT_SAVED_CCR_EQ 0x08, msg_fail_7_ccr, msg_fail_7_ccr_len

| 8) LSL.W %d1,%d0 with count=0 (register count)
| Expect CCR = 0x18 (X=1,N=1) and C cleared
    SET_CCR 0x10
    moveq   #0, %d0
    move.w  #0x8001, %d0
    moveq   #0, %d1
    lsl.w   %d1, %d0
    SAVE_CCR
    MASK_D0_16
    ASSERT_D0_EQ 0x8001, msg_fail_8_val, msg_fail_8_val_len
    ASSERT_SAVED_CCR_EQ 0x18, msg_fail_8_ccr, msg_fail_8_ccr_len

| ------------------------------------------------------------
| Multi-bit count tests
| ------------------------------------------------------------

| 11) LSL.B #3, %d0 : 0x11 -> 0x88
| last bit out = orig bit (8-3)=bit5=0 => X=0,C=0; N=1; Z=0; V=0 => 0x08
    SET_CCR 0x00
    moveq   #0, %d0
    move.b  #0x11, %d0
    lsl.b   #3, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0x88, msg_fail_11_val, msg_fail_11_val_len
    ASSERT_SAVED_CCR_EQ 0x08, msg_fail_11_ccr, msg_fail_11_ccr_len

| 12) ASL.B #2, %d0 : 0x20 -> 0x80
| last bit out = orig bit6=0 => X=0,C=0; N=1; V=1 (sign flips during the shift) => 0x0A
    SET_CCR 0x00
    moveq   #0, %d0
    move.b  #0x20, %d0
    asl.b   #2, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0x80, msg_fail_12_val, msg_fail_12_val_len
    ASSERT_SAVED_CCR_EQ 0x0A, msg_fail_12_ccr, msg_fail_12_ccr_len

| 13) ASR.B #3, %d0 : 0xE1 -> 0xFC
| last bit out = orig bit2=0 => X=0,C=0; N=1; V=0; Z=0 => 0x08
    SET_CCR 0x00
    moveq   #0, %d0
    move.b  #0xE1, %d0
    asr.b   #3, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0xFC, msg_fail_13_val, msg_fail_13_val_len
    ASSERT_SAVED_CCR_EQ 0x08, msg_fail_13_ccr, msg_fail_13_ccr_len

| 14) ROL.B #3, %d0 with X preset = 1 : 0x96 -> 0xB4
| ROL leaves X unchanged; carry after final step = result bit0 = 0
| N=1 => X=1,N=1 => 0x18
    SET_CCR 0x10
    moveq   #0, %d0
    move.b  #0x96, %d0
    rol.b   #3, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0xB4, msg_fail_14_val, msg_fail_14_val_len
    ASSERT_SAVED_CCR_EQ 0x18, msg_fail_14_ccr, msg_fail_14_ccr_len

| 15) ROR.W #4, %d0 with X preset = 1 : 0x1234 -> 0x4123
| carry after final step = result msb = 0; X unchanged; N=0,Z=0 => 0x10
    SET_CCR 0x10
    moveq   #0, %d0
    move.w  #0x1234, %d0
    ror.w   #4, %d0
    SAVE_CCR
    MASK_D0_16
    ASSERT_D0_EQ 0x4123, msg_fail_15_val, msg_fail_15_val_len
    ASSERT_SAVED_CCR_EQ 0x10, msg_fail_15_ccr, msg_fail_15_ccr_len

| 16) ROXL.W #2, %d0 with X preset = 1 : 0x8000 -> 0x0003
| 17-bit ring: expected new X=0 and C=0; result 0x0003 => N=0,Z=0 => CCR = 0x00
    SET_CCR 0x10
    moveq   #0, %d0
    move.w  #0x8000, %d0
    roxl.w  #2, %d0
    SAVE_CCR
    MASK_D0_16
    ASSERT_D0_EQ 0x0003, msg_fail_16_val, msg_fail_16_val_len
    ASSERT_SAVED_CCR_EQ 0x00, msg_fail_16_ccr, msg_fail_16_ccr_len

| 17) ROL.B %d1,%d0 with %d1=11 (tests count mod 8 => 3)
| X preset=1, value 0x96 => same as test 14: 0xB4 and CCR 0x18
    SET_CCR 0x10
    moveq   #0, %d0
    move.b  #0x96, %d0
    moveq   #11, %d1
    rol.b   %d1, %d0
    SAVE_CCR
    MASK_D0_8
    ASSERT_D0_EQ 0xB4, msg_fail_17_val, msg_fail_17_val_len
    ASSERT_SAVED_CCR_EQ 0x18, msg_fail_17_ccr, msg_fail_17_ccr_len

| All tests passed
    SYSCALL_WRITE msg_ok, msg_ok_len
    SYSCALL_EXIT  0

| ------------------------------------------------------------
| Messages
| ------------------------------------------------------------
    .data
msg_ok:                 .ascii "success\n"
msg_ok_len = . - msg_ok

msg_fail_1_val:         .ascii "FAIL 1 value\n"
msg_fail_1_val_len = . - msg_fail_1_val
msg_fail_1_ccr:         .ascii "FAIL 1 ccr\n"
msg_fail_1_ccr_len = . - msg_fail_1_ccr

msg_fail_2_val:         .ascii "FAIL 2 value\n"
msg_fail_2_val_len = . - msg_fail_2_val
msg_fail_2_ccr:         .ascii "FAIL 2 ccr\n"
msg_fail_2_ccr_len = . - msg_fail_2_ccr

msg_fail_3_val:         .ascii "FAIL 3 value\n"
msg_fail_3_val_len = . - msg_fail_3_val
msg_fail_3_ccr:         .ascii "FAIL 3 ccr\n"
msg_fail_3_ccr_len = . - msg_fail_3_ccr

msg_fail_4_val:         .ascii "FAIL 4 value\n"
msg_fail_4_val_len = . - msg_fail_4_val
msg_fail_4_ccr:         .ascii "FAIL 4 ccr\n"
msg_fail_4_ccr_len = . - msg_fail_4_ccr

msg_fail_5_val:         .ascii "FAIL 5 value\n"
msg_fail_5_val_len = . - msg_fail_5_val
msg_fail_5_ccr:         .ascii "FAIL 5 ccr\n"
msg_fail_5_ccr_len = . - msg_fail_5_ccr

msg_fail_6_val:         .ascii "FAIL 6 value\n"
msg_fail_6_val_len = . - msg_fail_6_val
msg_fail_6_ccr:         .ascii "FAIL 6 ccr\n"
msg_fail_6_ccr_len = . - msg_fail_6_ccr

msg_fail_7_val:         .ascii "FAIL 7 value\n"
msg_fail_7_val_len = . - msg_fail_7_val
msg_fail_7_ccr:         .ascii "FAIL 7 ccr\n"
msg_fail_7_ccr_len = . - msg_fail_7_ccr

msg_fail_8_val:         .ascii "FAIL 8 value\n"
msg_fail_8_val_len = . - msg_fail_8_val
msg_fail_8_ccr:         .ascii "FAIL 8 ccr\n"
msg_fail_8_ccr_len = . - msg_fail_8_ccr

msg_fail_11_val:        .ascii "FAIL 11 value\n"
msg_fail_11_val_len = . - msg_fail_11_val
msg_fail_11_ccr:        .ascii "FAIL 11 ccr\n"
msg_fail_11_ccr_len = . - msg_fail_11_ccr

msg_fail_12_val:        .ascii "FAIL 12 value\n"
msg_fail_12_val_len = . - msg_fail_12_val
msg_fail_12_ccr:        .ascii "FAIL 12 ccr\n"
msg_fail_12_ccr_len = . - msg_fail_12_ccr

msg_fail_13_val:        .ascii "FAIL 13 value\n"
msg_fail_13_val_len = . - msg_fail_13_val
msg_fail_13_ccr:        .ascii "FAIL 13 ccr\n"
msg_fail_13_ccr_len = . - msg_fail_13_ccr

msg_fail_14_val:        .ascii "FAIL 14 value\n"
msg_fail_14_val_len = . - msg_fail_14_val
msg_fail_14_ccr:        .ascii "FAIL 14 ccr\n"
msg_fail_14_ccr_len = . - msg_fail_14_ccr

msg_fail_15_val:        .ascii "FAIL 15 value\n"
msg_fail_15_val_len = . - msg_fail_15_val
msg_fail_15_ccr:        .ascii "FAIL 15 ccr\n"
msg_fail_15_ccr_len = . - msg_fail_15_ccr

msg_fail_16_val:        .ascii "FAIL 16 value\n"
msg_fail_16_val_len = . - msg_fail_16_val
msg_fail_16_ccr:        .ascii "FAIL 16 ccr\n"
msg_fail_16_ccr_len = . - msg_fail_16_ccr

msg_fail_17_val:        .ascii "FAIL 17 value\n"
msg_fail_17_val_len = . - msg_fail_17_val
msg_fail_17_ccr:        .ascii "FAIL 17 ccr\n"
msg_fail_17_ccr_len = . - msg_fail_17_ccr
