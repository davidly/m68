.globl _g_board
.comm _g_board,10
.globl _g_moves
.comm _g_moves,2
.globl _winner2
.text
_winner2:
~~winner2:
~move=9
~_EnD__=10
link R14,#-6
~x=-2
*line 24
move.b 9(R14),R0
ext.w R0
bra L3
*line 25
L4:
*line 26
*line 27
*line 28
move.b _g_board,R0
ext.w R0
move R0,-2(R14)
*line 29
*line 30
*line 31
*line 32
move.b 1+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10001
move.b 2+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10000
L10001:move.b 3+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10002
move.b 6+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10000
L10002:move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L5
move.b 8+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L5
L10000:*line 32
move -2(R14),R0
bra L1
L5:
bra L2
*line 33
*line 34
L6:
*line 35
*line 36
*line 37
move.b 1+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 38
*line 39
*line 40
move.b _g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10004
move.b 2+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10003
L10004:move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L7
move.b 7+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L7
L10003:*line 40
move -2(R14),R0
bra L1
L7:
bra L2
*line 41
*line 42
L8:
*line 43
*line 44
*line 45
move.b 2+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 46
*line 47
*line 48
*line 49
move.b _g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10006
move.b 1+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10005
L10006:move.b 5+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10007
move.b 8+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10005
L10007:move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L9
move.b 6+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L9
L10005:*line 49
move -2(R14),R0
bra L1
L9:
bra L2
*line 50
*line 51
L10:
*line 52
*line 53
*line 54
move.b 3+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 55
*line 56
*line 57
move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10009
move.b 5+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10008
L10009:move.b _g_board,R0
ext.w R0
cmp -2(R14),R0
bne L11
move.b 6+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L11
L10008:*line 57
move -2(R14),R0
bra L1
L11:
bra L2
*line 58
*line 59
L12:
*line 60
*line 61
*line 62
move.b 4+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 63
*line 64
*line 65
*line 66
*line 67
move.b _g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10011
move.b 8+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10010
L10011:move.b 2+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10012
move.b 6+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10010
L10012:move.b 1+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10013
move.b 7+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10010
L10013:move.b 3+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L13
move.b 5+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L13
L10010:*line 67
move -2(R14),R0
bra L1
L13:
bra L2
*line 68
*line 69
L14:
*line 70
*line 71
*line 72
move.b 5+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 73
*line 74
*line 75
move.b 3+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10015
move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10014
L10015:move.b 2+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L15
move.b 8+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L15
L10014:*line 75
move -2(R14),R0
bra L1
L15:
bra L2
*line 76
*line 77
L16:
*line 78
*line 79
*line 80
move.b 6+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 81
*line 82
*line 83
*line 84
move.b 7+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10017
move.b 8+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10016
L10017:move.b _g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10018
move.b 3+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10016
L10018:move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L17
move.b 2+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L17
L10016:*line 84
move -2(R14),R0
bra L1
L17:
bra L2
*line 85
*line 86
L18:
*line 87
*line 88
*line 89
move.b 7+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 90
*line 91
*line 92
move.b 6+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10020
move.b 8+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10019
L10020:move.b 1+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L19
move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L19
L10019:*line 92
move -2(R14),R0
bra L1
L19:
bra L2
*line 93
*line 94
L20:
*line 95
*line 96
*line 97
move.b 8+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 98
*line 99
*line 100
*line 101
move.b 6+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10022
move.b 7+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10021
L10022:move.b 2+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L10023
move.b 5+_g_board,R0
ext.w R0
cmp -2(R14),R0
beq L10021
L10023:move.b _g_board,R0
ext.w R0
cmp -2(R14),R0
bne L21
move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L21
L10021:*line 101
move -2(R14),R0
bra L1
L21:
bra L2
*line 102
*line 103
bra L2
L3:
cmp #8,R0
bhi L2
asl #2,R0
move R0,R8
add.l #L22,R8
move.l (R8),R8
jmp (R8)
.data
L22:
.dc.l L4
.dc.l L6
.dc.l L8
.dc.l L10
.dc.l L12
.dc.l L14
.dc.l L16
.dc.l L18
.dc.l L20
.text
L2:
*line 106
clr R0
bra L1
L1:
unlk R14
rts
.globl _LookForW
.text
_LookForW:
~~LookForW:
~_EnD__=8
link R14,#-6
~p=-2
*line 112
move.b _g_board,R0
ext.w R0
move R0,-2(R14)
*line 113
*line 114
move -2(R14),R0
beq L24
*line 115
*line 116
move.b 1+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L25
move.b 2+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L25
*line 116
move -2(R14),R0
bra L23
*line 117
L25:
*line 118
*line 119
move.b 3+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L26
move.b 6+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L26
*line 119
move -2(R14),R0
bra L23
L26:
*line 120
*line 121
L24:
*line 122
move.b 3+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 123
*line 124
move -2(R14),R0
beq L27
move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L27
move.b 5+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L27
*line 124
move -2(R14),R0
bra L23
*line 125
L27:
*line 126
move.b 6+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 127
*line 128
move -2(R14),R0
beq L28
move.b 7+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L28
move.b 8+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L28
*line 128
move -2(R14),R0
bra L23
*line 129
L28:
*line 130
move.b 1+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 131
*line 132
move -2(R14),R0
beq L29
move.b 4+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L29
move.b 7+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L29
*line 132
move -2(R14),R0
bra L23
*line 133
L29:
*line 134
move.b 2+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 135
*line 136
move -2(R14),R0
beq L30
move.b 5+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L30
move.b 8+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L30
*line 136
move -2(R14),R0
bra L23
*line 137
L30:
*line 138
move.b 4+_g_board,R0
ext.w R0
move R0,-2(R14)
*line 139
*line 140
move -2(R14),R0
beq L31
*line 141
*line 142
move.b _g_board,R0
ext.w R0
cmp -2(R14),R0
bne L32
move.b 8+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L32
*line 142
move -2(R14),R0
bra L23
*line 143
L32:
*line 144
*line 145
move.b 2+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L33
move.b 6+_g_board,R0
ext.w R0
cmp -2(R14),R0
bne L33
*line 145
move -2(R14),R0
bra L23
L33:
*line 146
*line 147
L31:
*line 148
clr R0
bra L23
L23:
unlk R14
rts
.globl _pos0func
.text
_pos0func:
~~pos0func:
~_EnD__=8
link R14,#-6
~x=-2
*line 154
move.b _g_board,-2(R14)
*line 156
*line 157
*line 158
*line 159
move.b 1+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10025
move.b 2+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10024
L10025:move.b 3+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10026
move.b 6+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10024
L10026:move.b 4+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L35
move.b 8+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L35
L10024:*line 159
move.b -2(R14),R0
ext.w R0
bra L34
L35:
*line 160
clr R0
bra L34
L34:
unlk R14
rts
.globl _pos1func
.text
_pos1func:
~~pos1func:
~_EnD__=8
link R14,#-6
~x=-2
*line 166
move.b 1+_g_board,-2(R14)
*line 168
*line 169
*line 170
move.b _g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10028
move.b 2+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10027
L10028:move.b 4+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L37
move.b 7+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L37
L10027:*line 170
move.b -2(R14),R0
ext.w R0
bra L36
L37:
*line 171
clr R0
bra L36
L36:
unlk R14
rts
.globl _pos2func
.text
_pos2func:
~~pos2func:
~_EnD__=8
link R14,#-6
~x=-2
*line 177
move.b 2+_g_board,-2(R14)
*line 179
*line 180
*line 181
*line 182
move.b _g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10030
move.b 1+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10029
L10030:move.b 5+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10031
move.b 8+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10029
L10031:move.b 4+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L39
move.b 6+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L39
L10029:*line 182
move.b -2(R14),R0
ext.w R0
bra L38
L39:
*line 183
clr R0
bra L38
L38:
unlk R14
rts
.globl _pos3func
.text
_pos3func:
~~pos3func:
~_EnD__=8
link R14,#-6
~x=-2
*line 189
move.b 3+_g_board,-2(R14)
*line 191
*line 192
*line 193
move.b 4+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10033
move.b 5+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10032
L10033:move.b _g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L41
move.b 6+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L41
L10032:*line 193
move.b -2(R14),R0
ext.w R0
bra L40
L41:
*line 194
clr R0
bra L40
L40:
unlk R14
rts
.globl _pos4func
.text
_pos4func:
~~pos4func:
~_EnD__=8
link R14,#-6
~x=-2
*line 200
move.b 4+_g_board,-2(R14)
*line 202
*line 203
*line 204
*line 205
*line 206
move.b _g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10035
move.b 8+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10034
L10035:move.b 2+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10036
move.b 6+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10034
L10036:move.b 1+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10037
move.b 7+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10034
L10037:move.b 3+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L43
move.b 5+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L43
L10034:*line 206
move.b -2(R14),R0
ext.w R0
bra L42
L43:
*line 207
clr R0
bra L42
L42:
unlk R14
rts
.globl _pos5func
.text
_pos5func:
~~pos5func:
~_EnD__=8
link R14,#-6
~x=-2
*line 213
move.b 5+_g_board,-2(R14)
*line 215
*line 216
*line 217
move.b 3+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10039
move.b 4+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10038
L10039:move.b 2+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L45
move.b 8+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L45
L10038:*line 217
move.b -2(R14),R0
ext.w R0
bra L44
L45:
*line 218
clr R0
bra L44
L44:
unlk R14
rts
.globl _pos6func
.text
_pos6func:
~~pos6func:
~_EnD__=8
link R14,#-6
~x=-2
*line 224
move.b 6+_g_board,-2(R14)
*line 226
*line 227
*line 228
*line 229
move.b 7+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10041
move.b 8+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10040
L10041:move.b _g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10042
move.b 3+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10040
L10042:move.b 4+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L47
move.b 2+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L47
L10040:*line 229
move.b -2(R14),R0
ext.w R0
bra L46
L47:
*line 230
clr R0
bra L46
L46:
unlk R14
rts
.globl _pos7func
.text
_pos7func:
~~pos7func:
~_EnD__=8
link R14,#-6
~x=-2
*line 236
move.b 7+_g_board,-2(R14)
*line 238
*line 239
*line 240
move.b 6+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10044
move.b 8+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10043
L10044:move.b 1+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L49
move.b 4+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L49
L10043:*line 240
move.b -2(R14),R0
ext.w R0
bra L48
L49:
*line 241
clr R0
bra L48
L48:
unlk R14
rts
.globl _pos8func
.text
_pos8func:
~~pos8func:
~_EnD__=8
link R14,#-6
~x=-2
*line 247
move.b 8+_g_board,-2(R14)
*line 249
*line 250
*line 251
*line 252
move.b 6+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10046
move.b 7+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10045
L10046:move.b 2+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L10047
move.b 5+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
beq L10045
L10047:move.b _g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L51
move.b 4+_g_board,R0
ext.w R0
cmp.b -2(R14),R0
bne L51
L10045:*line 252
move.b -2(R14),R0
ext.w R0
bra L50
L51:
*line 253
clr R0
bra L50
L50:
unlk R14
rts
.globl _winner_f
.comm _winner_f,36
.globl _MinMax
.text
_MinMax:
~~MinMax:
~alpha=9
~beta=11
~depth=13
~move=15
~_EnD__=16
link R14,#-16
~pieceMov=-2
~score=-4
~p=-6
~value=-8
~winfunc=-12
*line 264
add #1,_g_moves
*line 266
*line 267
cmp.b #4,13(R14)
blt L53
*line 268
*line 269
move.b 15(R14),R0
ext.w R0
move R0,R8
add.l R8,R8
add.l R8,R8
add.l #_winner_f,R8
move.l (R8),-12(R14)
*line 270
move.l -12(R14),R8
jsr (R8)
move R0,-6(R14)
*line 271
*line 272
*line 273
move -6(R14),R0
beq L54
*line 274
*line 275
cmp #1,-6(R14)
bne L55
*line 275
move #6,R0
bra L52
*line 276
L55:
*line 277
move #4,R0
bra L52
*line 278
*line 279
L54:
*line 280
*line 281
cmp.b #8,13(R14)
bne L56
*line 281
move #5,R0
bra L52
L56:
*line 282
*line 283
L53:
*line 284
*line 285
btst #0,13(R14)
beq L57
*line 286
move #2,-8(R14)
*line 287
move.b #1,-2(R14)
*line 288
*line 289
bra L58
L57:
*line 290
*line 291
move #9,-8(R14)
*line 292
move.b #2,-2(R14)
L58:
*line 295
clr -6(R14)
bra L61
L62:
*line 296
*line 297
*line 298
move.l #_g_board,R8
move -6(R14),R9
add.l R9,R8
tst.b (R8)
bne L63
*line 299
move.l #_g_board,R8
move -6(R14),R9
add.l R9,R8
move.b -2(R14),(R8)
*line 300
move -6(R14),(sp)
move.b 13(R14),R0
ext.w R0
move R0,-(sp)
add #1,(sp)
move.b 11(R14),R0
ext.w R0
move R0,-(sp)
move.b 9(R14),R0
ext.w R0
move R0,-(sp)
jsr _MinMax
addq.l #6,sp
ext.w R0
move.b R0,-4(R14)
*line 301
move.l #_g_board,R8
move -6(R14),R9
add.l R9,R8
clr.b (R8)
*line 302
*line 303
*line 304
btst #0,13(R14)
beq L64
*line 305
*line 306
cmp.b #6,-4(R14)
bne L65
*line 306
move #6,R0
bra L52
*line 307
L65:
*line 308
*line 309
move.b -4(R14),R0
ext.w R0
cmp -8(R14),R0
ble L66
*line 310
move.b -4(R14),R0
ext.w R0
move R0,-8(R14)
*line 311
*line 312
*line 313
move -8(R14),R0
move.b 11(R14),R1
ext.w R1
cmp R1,R0
blt L67
*line 313
move -8(R14),R0
bra L52
L67:
*line 314
*line 315
move -8(R14),R0
move.b 9(R14),R1
ext.w R1
cmp R1,R0
ble L68
*line 315
move -8(R14),R0
ext.w R0
move.b R0,9(R14)
L68:
*line 316
L66:
*line 317
*line 318
bra L69
L64:
*line 319
*line 320
*line 321
cmp.b #4,-4(R14)
bne L70
*line 321
move #4,R0
bra L52
*line 322
L70:
*line 323
*line 324
move.b -4(R14),R0
ext.w R0
cmp -8(R14),R0
bge L71
*line 325
move.b -4(R14),R0
ext.w R0
move R0,-8(R14)
*line 326
*line 327
*line 328
move -8(R14),R0
move.b 9(R14),R1
ext.w R1
cmp R1,R0
bgt L72
*line 328
move -8(R14),R0
bra L52
L72:
*line 329
*line 330
move -8(R14),R0
move.b 11(R14),R1
ext.w R1
cmp R1,R0
bge L73
*line 330
move -8(R14),R0
ext.w R0
move.b R0,11(R14)
L73:
*line 331
L71:
L69:
*line 332
*line 333
L63:
L60:
*line 295
add #1,-6(R14)
L61:
*line 295
cmp #9,-6(R14)
blt L62
L59:
*line 336
move -8(R14),R0
bra L52
L52:
unlk R14
rts
.globl _FindSolu
.text
_FindSolu:
~~FindSolu:
~position=9
~_EnD__=10
link R14,#-4
*line 341
move.l #_g_board,R8
move.b 9(R14),R1
ext.w R1
ext.l R1
add.l R1,R8
move.b #1,(R8)
*line 342
move.b 9(R14),R0
ext.w R0
move R0,(sp)
clr -(sp)
move #9,-(sp)
move #2,-(sp)
jsr _MinMax
addq.l #6,sp
*line 343
move.l #_g_board,R8
move.b 9(R14),R1
ext.w R1
ext.l R1
add.l R1,R8
clr.b (R8)
L74:
unlk R14
rts
.globl _main
.text
_main:
~~main:
~argc=8
~argv=10
~_EnD__=14
link R14,#-8
~i=-2
~iteratio=-4
*line 350
clr -2(R14)
bra L78
L79:
*line 351
move.l #_g_board,R8
move -2(R14),R9
add.l R9,R8
clr.b (R8)
L77:
*line 350
add #1,-2(R14)
L78:
*line 350
cmp #9,-2(R14)
blt L79
L76:
*line 353
move.l #_pos0func,_winner_f
*line 354
move.l #_pos1func,4+_winner_f
*line 355
move.l #_pos2func,8+_winner_f
*line 356
move.l #_pos3func,12+_winner_f
*line 357
move.l #_pos4func,16+_winner_f
*line 358
move.l #_pos5func,20+_winner_f
*line 359
move.l #_pos6func,24+_winner_f
*line 360
move.l #_pos7func,28+_winner_f
*line 361
move.l #_pos8func,32+_winner_f
*line 363
*line 364
cmp #2,8(R14)
bne L80
*line 364
move.l R14,(sp)
sub.l #4,(sp)
move.l #L81,-(sp)
move.l 10(R14),R8
move.l 4(R8),-(sp)
jsr _sscanf
addq.l #8,sp
*line 365
bra L82
L80:
*line 366
move #10,-4(R14)
L82:
*line 368
clr -2(R14)
bra L85
L86:
*line 369
*line 370
clr _g_moves
*line 371
clr (sp)
jsr _FindSolu
*line 372
move #1,(sp)
jsr _FindSolu
*line 373
move #4,(sp)
jsr _FindSolu
L84:
*line 368
add #1,-2(R14)
L85:
*line 368
move -2(R14),R0
cmp -4(R14),R0
blt L86
L83:
*line 376
move _g_moves,(sp)
move.l #L87,-(sp)
jsr _printf
addq.l #4,sp
*line 377
move -4(R14),(sp)
move.l #L88,-(sp)
jsr _printf
addq.l #4,sp
*line 378
clr R0
bra L75
L75:
unlk R14
rts
.data
L81:
.dc.b $25,$64,$0
L87:
.dc.b $6D,$6F,$76,$65,$20,$63,$6F,$75,$6E,$74,$3A,$20,$20,$20,$20,$20,$20,$25,$64,$A,$0
L88:
.dc.b $69,$74,$65,$72,$61,$74,$69,$6F,$6E,$20,$63,$6F,$75,$6E,$74,$3A,$20,$25,$64,$A,$0

