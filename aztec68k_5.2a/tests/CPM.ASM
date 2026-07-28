; Start-up routine for cpm68k program for Aztec C
; these need to be implemented for full C runtime support (along with __ equivalents):
;   open lseek creat rename close unlink read ioctl write isatty

                dseg
                public  __mbot,__mtop,__mcur    ;*jd 15 Dec 86
                public  _defdma

_defdma ds.l    1               ; ptr to the default cp/m dma location / command tail length

arg0_value dc.b 'cpmapp',0

                bss argvals,256                 ; array of pointers to command line arguments
                bss cmdtail_copy,128            ; a copy of the command tail because it gets overwritten

                public __H1_org,__H1_end        ;*jd 27 Oct 86
                public __H2_org,__H2_end        ;*jd 27 Oct 86
                public __H0_org,__H0_end        ;*jd 27 Oct 86

                cseg

                public .begin
                public _main
                public _memcpy
                public _exit
                public _bdos_cpm

;##################################
.begin:
; Assume program uses small code &/or small data
; and that it uses A5 to access jmptabs and data
; ...if program uses large code and large data,
;    and you want to use A5 for other purposes (eg, register variable),
;    delete the next line of code
; ...if it uses a different address register, change the next line
                move.l  #__H1_org+32766,a5      ;*jd 17 Nov 86
; Calculate length of BSS segment and initialize address register
                move.l  #__H2_end,d0            ;*jd 27 Oct 86
                sub.l   #__H2_org,d0            ;*jd 27 Oct 86
                beq    .2end                    ; skip if no BSS segment
                lsr.l   #1,d0                   ; divide length by 2 and clear words
                move.l  #__H2_org,a0            ; BSS base address

; Initialize BSS segment to 0
.2
                clr.w   (a0)+
                dbra    d0,.2
;
.2end

; Initialize heap ptrs
                move.l  4(a7),a1                ; load a pointer to the base page
                move.l  $18(a1),d0
                move.l  $1c(a1),d1
                add.l   d1,d0
                move.l  d0,__mbot               ; bottom of brk space
                move.l  d0,__mcur               ; current brk space location
                add.l   $20(a1),d0
                move.l  d0,__mtop               ; top of brk space
                add.l   #4,d0                   ; stack base (bottom) is in d0. not sure where manx wants it for stack checks.
                                                ; __stkbase? is it used?

; initialize argc, argv, envp
                move.l #1,d4                    ; argc cp/m 68k has no way of knowing the app name!
                move.l #argvals,a3              ; argv
                clr.l d6                        ; envp
                move.l #arg0_value,a0
                move.l a0,(a3)
                move.l a3,a4
                adda.l #4,a4                    ; a4 points to the next argv to write

                move.l 4(a7),a0                 ; load the base page pointer
                adda.l #128,a0                  ; point to command tail len
                move.l a0,_defdma
                clr.l d3
                move.b (a0),d3                  ; tail length in d3

                adda.l #1,a0                    ; make a copy of the command tail because it'll get reused as the DMA later
                move.l d3,-(a7)                 ; size_t
                move.l a0,-(a7)
                move.l #cmdtail_copy,-(a7)
                jsr _memcpy
                adda.l #12,a7

                move.l #cmdtail_copy,a0
                clr.l d0                        ; offset into the command tail
.next_argument:                                 ; copy a pointer to each argument to the argvals array
                cmp.l d0,d3
                beq .call_main
                addi.l #1,d4
                move.l a0,(a4)
                adda.l #4,a4                    ; a4 points to the next argv to write
.next_sp_check:
                cmpi.b #32,(a0)
                bne .notspace
                clr.b (a0)
                adda.l #1,a0
                addi.l #1,d0
                bra .next_argument
.notspace:
                adda.l #1,a0
                addi.l #1,d0
                cmp.l d0,d3                     ; at the end of the string?
                beq .call_main
                bra .next_sp_check

.call_main:
                move.l d6,-(a7)                 ; push main()'s 3 arguments on the stack
                move.l a3,-(a7)
                move.l d4,-(a7)                 ; argc is a 32-bit int

                jsr _main

                adda.l #12,a7                   ; restore sp
                rts

;##################################
_exit:
                clr.l d1                        ; clear the high bits of the exit code
                move.w 4(a7),d1                 ; move the exit code to d1 and invoke the cp/m 68k exit trap
                clr.l d0
                trap #2

;##################################
_bdos_cpm:
                move.l 4(a7),d0                 ; cp/m syscall ID
                move.l 8(a7),d1                 ; the one and only argument
                trap #2                         ; no errno-style errors are returned; layer above must set errno
                rts

;##################################
                end

