/* @(#)mem_vec_sparc.s 1.1 94/10/31 SMI */

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

	.seg	"text"			! [internal]
	.proc	4
	.global	_mem_vector
_mem_vector:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-128,%sp
	sra	%i5,5,%l6
	tst	%l6
	st	%i0,[%fp+68]
	mov	%i0,%l2
	ld	[%l2+16],%i0
	bne	L77003
	and	%i5,1,%l0
	ld	[%fp+92],%l6
L77003:
	ld	[%l2+12],%l7
	sra	%i5,1,%i5
	cmp	%l7,1
	bne	L77010
	and	%i5,15,%i5
	ldsh	[%i0+18],%o4
	mov	0,%l3
	andcc	%o4,1,%g0
	be,a	LY148
	tst	%l6
	xor	%i5,-1,%i5
	sra	%i5,1,%o0
	and	%i5,5,%i5
	sll	%i5,1,%i5
	and	%o0,5,%o0
	or	%i5,%o0,%i5
	tst	%l6
LY148:					! [internal]
	be,a	LY27
	and	%i5,3,%i5
	sra	%i5,2,%i5
LY27:					! [internal]
	sll	%i5,2,%l7
	b	L77030
	or	%i5,%l7,%l7
L77010:
	ldsh	[%i0+18],%o2
	mov	-1,%l5
	andcc	%o2,4,%g0
	be,a	LY147
	cmp	%l7,8
	ld	[%i0+20],%l5
	cmp	%l7,8
LY147:					! [internal]
	be	L77013
	cmp	%l7,16
	be	L77014
	cmp	%l7,32
	be,a	LY146
	mov	3,%l3
	b	L77542
	mov	-1,%i2
L77013:
	mov	1,%l3
	b	L77018
	mov	255,%l7
L77014:
	sethi	%hi(0xffff),%o4
	mov	2,%l3
	b	L77018
	add	%o4,%lo(0xffff),%l7
LY146:					! [internal]
	mov	-1,%l7
L77018:
	and	%l5,%l7,%l4
	and	%l6,%l4,%l5
	tst	%l5
	bne,a	LY145
	cmp	%l5,%l4
	and	%i5,3,%i5
	sll	%i5,2,%o1
	b	LY28
	or	%i5,%o1,%i5
LY145:					! [internal]
	bne,a	LY144
	cmp	%l4,%l7
	sra	%i5,2,%i5
	sll	%i5,2,%o4
	or	%i5,%o4,%i5
	mov	0,%l5
LY28:					! [internal]
	cmp	%l4,%l7
LY144:					! [internal]
	bne,a	LY143
	sethi	%hi(L45),%o7
	inc	16,%i5
	sethi	%hi(L45),%o7
LY143:					! [internal]
	or	%o7,%lo(L45),%o7	! [internal]
	ldsb	[%i5+%o7],%i5
	andcc	%i5,32,%g0
	be,a	LY142
	andcc	%i5,16,%g0
	mov	0,%l5
	andcc	%i5,16,%g0
LY142:					! [internal]
	be,a	LY141
	st	%l5,[%fp+92]
	xor	%l5,-1,%l5
	and	%l4,%l5,%l5		! rcolor &= rplanes
	st	%l5,[%fp+92]
LY141:					! [internal]
	mov	%l4,%l6
	and	%i5,15,%l7
L77030:
	sub	%i3,%i1,%l5
	tst	%l5
	bne	L77161
	mov	%l7,%l1
	cmp	%i2,%i4
	bg	L77033
	mov	%i2,%i3
	b	LY29
	sub	%i4,%i2,%i5
L77033:
	sub	%i2,%i4,%i5
	sub	%i2,%i5,%i3
LY29:					! [internal]
	tst	%l0
	bne,a	LY139
	ld	[%i0],%i4
	ld	[%l2+4],%o2
	cmp	%i1,%o2
	bcc,a	L77542
	mov	0,%i2
	tst	%i3
	bge,a	LY140
	ld	[%l2+8],%l2
	addcc	%i5,%i3,%i5
	bneg,a	L77542
	mov	0,%i2
	mov	0,%i3
	ld	[%l2+8],%l2
LY140:					! [internal]
	add	%i3,%i5,%o5
	cmp	%o5,%l2
	bl,a	LY139
	ld	[%i0],%i4
	sub	%l2,%i3,%i5
	deccc	%i5
	bneg,a	L77542
	mov	0,%i2
	ld	[%i0],%i4
LY139:					! [internal]
	ld	[%i0+4],%l4
	ld	[%i0+8],%l5
	ld	[%i0+12],%o1
	sll	%i4,16,%o0
	add	%i3,%o1,%o1
	sll	%o1,16,%o1
	sra	%o1,16,%o1
	call	.mul,2
	sra	%o0,16,%o0
	add	%i1,%l5,%l5
	add	%l4,%o0,%i1
	cmp	%l3,3
	mov	%i5,%i2
	bgu	LY22
	sll	%l3,2,%o0
	sethi	%hi(L2000006),%o1
	or	%o1,%lo(L2000006),%o1	! [internal]
	ld	[%o0+%o1],%o0
	jmp	%o0
	nop
L2000006:
	.word	L77047
	.word	L77060
	.word	L77110
	.word	L77134
L77047:
	sethi	%hi(0x8000),%o3
	and	%l5,15,%i0
	srl	%o3,%i0,%i0
	sra	%l5,4,%o7
	sll	%o7,1,%o7
	sll	%i0,16,%i0
	tst	%l7
	srl	%i0,16,%i0
	add	%i1,%o7,%i1
	be	L110
	cmp	%l7,5
	be	L77569
	cmp	%l7,15
	bne,a	L77542
	mov	0,%i2
	mov	%i0,%i5
	lduh	[%i1],%o0
LY138:					! [internal]
	deccc	%i2
	or	%o0,%i5,%o0
	sll	%o0,16,%o0
	srl	%o0,16,%o0
	sth	%o0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY138
	lduh	[%i1],%o0
L77569:
	mov	%i0,%i5
	lduh	[%i1],%o5
LY137:					! [internal]
	deccc	%i2
	xor	%o5,%i5,%o5
	sll	%o5,16,%o5
	srl	%o5,16,%o5
	sth	%o5,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY137
	lduh	[%i1],%o5
L110:
	xor	%i0,-1,%i5
	sll	%i5,16,%i5
	srl	%i5,16,%i5
	lduh	[%i1],%o7
LY136:					! [internal]
	deccc	%i2
	and	%o7,%i5,%o7
	sll	%o7,16,%o7
	srl	%o7,16,%o7
	sth	%o7,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY136
	lduh	[%i1],%o7
L77060:
	ld	[%fp+92],%i0
	and	%l6,255,%l6
	mov	%l6,%i3
	cmp	%i3,255
	add	%i1,%l5,%i1
	bne	L180
/*
 * depth 8 vertical unmasked PIX_SRC vectors
 *
	and	%i0,255,%i0
	call	__line_tag,1
	mov	400,%o0
	mov	%i1,%o0		! d
	mov	%i0,%o1		! k
	mov	%i5,%o2		! count (really dy)
	call	__line_tag_args,4
	mov	%i4,%o3		! offset
	cmp	%l7,12
	bne,a	LY135
	dec	4,%l7
	deccc	%i2
LY134:					! [internal]
	stb	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY134
	deccc	%i2
 *
 */

/* begin */
	cmp	%l7,12
	beq,a	2f
	deccc	%i2
	b	LY135
	dec	4,%l7

1:   	add	%i1,%i4,%i1
	deccc	%i2
2:	bpos	1b
	stb	%i0,[%i1]
	ret
	restore	%g0,%g0,%o0
/* end */
	

L139:
	ldub	[%i1],%o4
LY133:					! [internal]
	deccc	%i2
	xor	%o4,%i0,%o4
	stb	%o4,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY133
	ldub	[%i1],%o4
L147:
	ldub	[%i1],%o2
LY132:					! [internal]
	deccc	%i2
	or	%o2,%i0,%o2
	stb	%o2,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY132
	ldub	[%i1],%o2
L155:
	ldub	[%i1],%o0
LY131:					! [internal]
	deccc	%i2
	and	%o0,%i0,%o0
	stb	%o0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY131
	ldub	[%i1],%o0
L163:
	ldub	[%i1],%o5
LY130:					! [internal]
	deccc	%i2
	xor	%o5,-1,%o5
	and	%o5,%i0,%o5
	stb	%o5,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY130
	ldub	[%i1],%o5
L171:
	ldub	[%i1],%o4
LY129:					! [internal]
	deccc	%i2
	and	%o4,%i0,%o4
	xor	%o4,-1,%o4
	stb	%o4,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY129
	ldub	[%i1],%o4
LY135:					! [internal]
	cmp	%l7,10
	bgu	LY22
	sll	%l7,2,%l7
	sethi	%hi(L2000002),%o1
	or	%o1,%lo(L2000002),%o1	! [internal]
	ld	[%l7+%o1],%o0
	jmp	%o0
	nop
L2000002:
	.word	L163
	.word	L77082
	.word	L139
	.word	L171
	.word	L155
	.word	L77082
	.word	L77082
	.word	L77082
	.word	L77082
	.word	L77082
	.word	L147
L77082:
	b	L77542
	mov	0,%i2
L180:
	cmp	%l7,12
	bne,a	LY128
	dec	4,%l7
	and	%i3,%i0,%i5
	xor	%i3,-1,%i3
	and	%i3,255,%i3
	and	%i5,255,%i5
	ldub	[%i1],%o1
LY127:					! [internal]
	deccc	%i2
	and	%o1,%i3,%o1
	or	%o1,%i5,%o1
	and	%o1,255,%o1
	stb	%o1,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY127
	ldub	[%i1],%o1
L196:
	and	%i3,%i0,%i5
	and	%i5,255,%i5
	ldub	[%i1],%o3
LY126:					! [internal]
	deccc	%i2
	xor	%o3,%i5,%o3
	and	%o3,255,%o3
	stb	%o3,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY126
	ldub	[%i1],%o3
L204:
	and	%i3,%i0,%i5
	and	%i5,255,%i5
	ldub	[%i1],%o4
LY125:					! [internal]
	deccc	%i2
	or	%o4,%i5,%o4
	and	%o4,255,%o4
	stb	%o4,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY125
	ldub	[%i1],%o4
L212:
	xor	%i3,-1,%i3
	or	%i3,%i0,%i5
	and	%i5,255,%i5
	ldub	[%i1],%o7
LY124:					! [internal]
	deccc	%i2
	and	%o7,%i5,%o7
	and	%o7,255,%o7
	stb	%o7,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY124
	ldub	[%i1],%o7
L220:
	mov	%l6,%i3
	mov	%i0,%i5
	ldub	[%i1],%i0
LY123:					! [internal]
	deccc	%i2
	or	%i0,%i5,%o7
	and	%i3,%o7,%o7
	xor	%i0,%o7,%i0
	and	%i0,255,%i0
	stb	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY123
	ldub	[%i1],%i0
L228:				! 8 bit vert op 7
	mov	%l6,%i3
	mov	%i0,%i5
	ldub	[%i1],%i0
LY122:					! [internal]
	deccc	%i2
!	and	%i3,%i0,%o0		! t = d & m
!	and	%o0,%i5,%o0		! t &= k
	xor	%i0,-1,%o0
	or	%o0,%i5,%o0
	and	%i3,%o0,%o0
	xor	%i0,%o0,%i0
!	and	%i0,255,%i0
	stb	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY122
	ldub	[%i1],%i0
LY128:					! [internal]
	cmp	%l7,10
	bgu	LY22
	sll	%l7,2,%l7
	sethi	%hi(L2000003),%o1
	or	%o1,%lo(L2000003),%o1	! [internal]
	ld	[%l7+%o1],%o0
	jmp	%o0
	nop
L2000003:
	.word	L220
	.word	L77106
	.word	L196
	.word	L228
	.word	L212
	.word	L77106
	.word	L77106
	.word	L77106
	.word	L77106
	.word	L77106
	.word	L204
L77106:
	b	L77542
	mov	0,%i2
L77110:
	ld	[%fp+92],%i0
	cmp	%l7,12
	sll	%i0,16,%i0
	sll	%l6,16,%i3
	sll	%l5,1,%l5
	srl	%i0,16,%i0
	srl	%i3,16,%i3
	bne	L77130
	add	%i1,%l5,%i1
	and	%i3,%i0,%i5
	xor	%i3,-1,%i3
	sll	%i3,16,%i3
	sll	%i5,16,%i5
	srl	%i5,16,%i5
	srl	%i3,16,%i3
	lduh	[%i1],%o4
LY121:					! [internal]
	deccc	%i2
	and	%o4,%i3,%o4
	or	%o4,%i5,%o4
	sll	%o4,16,%o4
	srl	%o4,16,%o4
	sth	%o4,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY121
	lduh	[%i1],%o4
L253:
	and	%i3,%i0,%i5
	sll	%i5,16,%i5
	srl	%i5,16,%i5
	lduh	[%i1],%o0
LY120:					! [internal]
	deccc	%i2
	xor	%o0,%i5,%o0
	sll	%o0,16,%o0
	srl	%o0,16,%o0
	sth	%o0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY120
	lduh	[%i1],%o0
L261:
	and	%i3,%i0,%i5
	sll	%i5,16,%i5
	srl	%i5,16,%i5
	lduh	[%i1],%o2
LY119:					! [internal]
	deccc	%i2
	or	%o2,%i5,%o2
	sll	%o2,16,%o2
	srl	%o2,16,%o2
	sth	%o2,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY119
	lduh	[%i1],%o2
L269:
	xor	%i3,-1,%i5
	or	%i5,%i0,%i5
	sll	%i5,16,%i5
	srl	%i5,16,%i5
	lduh	[%i1],%o5
LY118:					! [internal]
	deccc	%i2
	and	%o5,%i5,%o5
	sll	%o5,16,%o5
	srl	%o5,16,%o5
	sth	%o5,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY118
	lduh	[%i1],%o5
L277:
	mov	%i0,%i5
	lduh	[%i1],%i0
LY117:					! [internal]
	deccc	%i2
	or	%i0,%i5,%o5
	and	%i3,%o5,%o5
	xor	%i0,%o5,%i0
	sll	%i0,16,%i0
	srl	%i0,16,%i0
	sth	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY117
	lduh	[%i1],%i0
L285:
	mov	%i0,%i5
	lduh	[%i1],%i0
LY116:					! [internal]
	deccc	%i2
!	and	%i3,%i0,%o7
!	and	%o7,%i5,%o7
	xor	%i0,-1,%o7
	or	%o7,%i5,%o7
	and	%i3,%o7,%o7
	xor	%i0,%o7,%i0
!	sll	%i0,16,%i0
!	srl	%i0,16,%i0
	sth	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY116
	lduh	[%i1],%i0
L77130:
	dec	4,%l7
	cmp	%l7,10
	bgu	LY22
	sll	%l7,2,%l7
	sethi	%hi(L2000004),%o1
	or	%o1,%lo(L2000004),%o1	! [internal]
	ld	[%l7+%o1],%o0
	jmp	%o0
	nop
L2000004:
	.word	L277
	.word	L77131
	.word	L253
	.word	L285
	.word	L269
	.word	L77131
	.word	L77131
	.word	L77131
	.word	L77131
	.word	L77131
	.word	L261
L77131:
	b	L77542
	mov	0,%i2
L77134:
	ld	[%fp+92],%i5
	cmp	%l7,12
	sll	%l5,2,%l5
	mov	%l6,%i3
	bne	L77154
	add	%i1,%l5,%i1
	xor	%l6,-1,%i3
	and	%l6,%i5,%i5
	ld	[%i1],%i0
LY115:					! [internal]
	deccc	%i2
	and	%i0,%i3,%i0
	or	%i0,%i5,%i0
	st	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY115
	ld	[%i1],%i0
L310:
	and	%l6,%i5,%i5
	ld	[%i1],%i0
LY114:					! [internal]
	deccc	%i2
	xor	%i0,%i5,%i0
	st	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY114
	ld	[%i1],%i0
L318:
	and	%l6,%i5,%i5
	ld	[%i1],%i0
LY113:					! [internal]
	deccc	%i2
	or	%i0,%i5,%i0
	st	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY113
	ld	[%i1],%i0
L326:
	xor	%l6,-1,%l6
	or	%l6,%i5,%i5
	ld	[%i1],%i0
LY112:					! [internal]
	deccc	%i2
	and	%i0,%i5,%i0
	st	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY112
	ld	[%i1],%i0
L77149:
	ld	[%i1],%i0
LY111:					! [internal]
	deccc	%i2
	or	%i0,%i5,%o2
	and	%i3,%o2,%o2
	xor	%i0,%o2,%i0
	st	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY111
	ld	[%i1],%i0
L77152:
	ld	[%i1],%i0
LY110:					! [internal]
	deccc	%i2
!	and	%i3,%i0,%o7
!	and	%o7,%i5,%o7
	xor	%i0,-1,%o7
	or	%i0,%i5,%o7
	and	%o7,%i3,%o7
	xor	%i0,%o7,%i0
	st	%i0,[%i1]
	bneg	LY22
	add	%i1,%i4,%i1
	b	LY110
	ld	[%i1],%i0
L77154:
	dec	4,%l7
	cmp	%l7,10
	bgu	LY22
	sll	%l7,2,%l7
	sethi	%hi(L2000005),%o1
	or	%o1,%lo(L2000005),%o1	! [internal]
	ld	[%l7+%o1],%o0
	jmp	%o0
	nop
L2000005:
	.word	L77149
	.word	L77155
	.word	L310
	.word	L77152
	.word	L326
	.word	L77155
	.word	L77155
	.word	L77155
	.word	L77155
	.word	L77155
	.word	L318
L77155:
	b	L77542
	mov	0,%i2
L77161:
	sub	%i4,%i2,%i5
	tst	%i5
	bne	LY109
	tst	%l5
	mov	0,%i5
	bg	LY1
	mov	0,%l4
	mov	%i3,%i1
	sub	%g0,%l5,%l5
LY1:					! [internal]
	tst	%l0
	bne	L77177
	add	%l5,1,%i4
	ld	[%l2+8],%o7
	cmp	%i2,%o7
	bcc,a	L77542
	mov	0,%i2
	tst	%i1
	bge,a	LY108
	ld	[%l2+4],%l2
	add	%i4,%i1,%i4
	tst	%i4
	ble,a	L77542
	mov	0,%i2
	mov	0,%i1
	ld	[%l2+4],%l2
LY108:					! [internal]
	add	%i1,%i4,%o2
	cmp	%o2,%l2
	bl,a	LY107
	ld	[%i0+8],%i3
	sub	%l2,%i1,%i4
	tst	%i4
	ble,a	L77542
	mov	0,%i2
L77177:
	ld	[%i0+8],%i3
LY107:					! [internal]
	ld	[%i0+12],%o1
	ld	[%i0],%o0
	add	%i2,%o1,%o1
	sll	%o1,16,%o1
	sll	%o0,16,%o0
	sra	%o0,16,%o0
	call	.mul,2
	sra	%o1,16,%o1
	ld	[%i0+4],%i2
	cmp	%l3,3
	add	%i2,%o0,%i2
	add	%i1,%i3,%i3
	ld	[%fp+92],%i1
	mov	%l6,%i0
	bgu	L77190
	sll	%l3,2,%o0
	sethi	%hi(L2000008),%o1
	or	%o1,%lo(L2000008),%o1	! [internal]
	ld	[%o0+%o1],%o0
	jmp	%o0
	nop
L2000008:
	.word	L77178
	.word	L77185
	.word	L77186
	.word	L77187
L77178:
	tst	%l7
	mov	-1,%i0
	be	L77179
	cmp	%l7,5
	be	L77180
	cmp	%l7,15
	be,a	LY106
	mov	12,%l1
	b	LY105
	andcc	%i2,2,%g0
L77179:
	mov	12,%l1
	b	L77190
	mov	0,%i1
L77180:
	mov	6,%l1
	b	L77190
	mov	-1,%i1
LY106:					! [internal]
	b	L77190
	mov	-1,%i1
L77185:
	sll	%l6,8,%i0
	sll	%i1,8,%o5
	or	%i1,%o5,%i1
	or	%l6,%i0,%i0
	sll	%i0,16,%o4
	sll	%i1,16,%o0
	sll	%i3,3,%i3
	sll	%i4,3,%i4
	or	%i0,%o4,%i0
	b	L77190
	or	%i1,%o0,%i1
L77186:
	sll	%l6,16,%i0
	sll	%i1,16,%o7
	sll	%i3,4,%i3
	sll	%i4,4,%i4
	or	%l6,%i0,%i0
	b	L77190
	or	%i1,%o7,%i1
L77187:
	sll	%i4,5,%i4
	sll	%i3,5,%i3
L77190:
	andcc	%i2,2,%g0
LY105:					! [internal]
	be,a	LY104
	sra	%i3,5,%o4
	inc	16,%i3
	dec	2,%i2
	sra	%i3,5,%o4
LY104:					! [internal]
	and	%i3,31,%i3
	tst	%i3
	sll	%o4,2,%o4
	be	L77195
	add	%i2,%o4,%i2
	mov	32,%o5
	mov	-1,%i5
	srl	%i5,%i3,%i5
	sub	%o5,%i3,%o5
	sub	%i4,%o5,%i4
	and	%i5,%i0,%i5
L77195:
	and	%i4,31,%o0
	tst	%o0
	be,a	LY103
	tst	%i4
	mov	32,%o1
	sub	%o1,%o0,%o1
	mov	-1,%l4
	sll	%l4,%o1,%l4
	and	%l4,%i0,%l4
	sub	%i4,%o0,%i4
	tst	%i4
LY103:					! [internal]
	bge	L77199
	nop
	and	%i5,%l4,%i5
	mov	0,%l4
	mov	0,%i4
L77199:
/*
 * horizontal PIX_SRC vectors
 */
!	call	__line_tag,1
!	mov	532,%o0
	sra	%i4,5,%i4	! dx >>= 5
	mov	%i4,%l7		! count = dx
!	mov	%l7,%o2		! count
!	mov	%i2,%o0		! d
!	call	__line_tag_args,3
!	mov	%i1,%o1		! k
	cmp	%l1,12		! op == 12
	bne	L77285
	mov	%i1,%i3
	tst	%i5		! if (lm)
	be,a	LY102
	deccc	%i4		! --count
	ld	[%i2],%l7
	xor	%l7,%i1,%o1
	and	%i5,%o1,%o1
	xor	%l7,%o1,%l7
	st	%l7,[%i2]
	inc	4,%i2		! d++
	deccc	%i4
LY102:					! [internal]
	bneg,a	LY101
	tst	%l4
	cmp	%i0,-1		! if (m == ~0)
	bne,a	LY100
	and	%i0,%i1,%i5

/* begin */
1:	deccc	%i4
	st	%i1,[%i2]
	bpos	1b
	inc	4,%i2
	b	LY101
	tst	%l4
/* end */

LY100:					! [internal]
	xor	%i0,-1,%i0
L77208:
	ld	[%i2],%i1
	deccc	%i4
	and	%i1,%i0,%i1
	or	%i1,%i5,%i1
	st	%i1,[%i2]
	bpos	L77208
	inc	4,%i2
L77211:
	tst	%l4
LY101:					! [internal]
	be,a	L77542
	mov	0,%i2
	ld	[%i2],%i4
	xor	%i4,%i3,%i3
	and	%l4,%i3,%l4
	xor	%i4,%l4,%i4
	b	LY22
	st	%i4,[%i2]
L419:
	tst	%i5
	be,a	LY98
	deccc	%i4
	ld	[%i2],%l7
	and	%i5,%i1,%i5
	xor	%l7,%i5,%l7
	st	%l7,[%i2]
	inc	4,%i2
	deccc	%i4
LY98:					! [internal]
	bneg,a	LY97
	tst	%l4
	cmp	%i0,-1
	bne,a	L77223
	and	%i0,%i1,%i0
	ld	[%i2],%o4
LY96:					! [internal]
	deccc	%i4
	xor	%o4,%i1,%o4
	st	%o4,[%i2]
	bneg	L77226
	inc	4,%i2
	b	LY96
	ld	[%i2],%o4
L77223:
	ld	[%i2],%i1
	deccc	%i4
	xor	%i1,%i0,%i1
	st	%i1,[%i2]
	bpos	L77223
	inc	4,%i2
L77226:
	tst	%l4
LY97:					! [internal]
	be,a	L77542
	mov	0,%i2
	ld	[%i2],%i0
	and	%l4,%i3,%l4
	xor	%i0,%l4,%i0
	b	LY22
	st	%i0,[%i2]
L440:
	tst	%i5
	be,a	LY95
	deccc	%i4
	ld	[%i2],%l7
	and	%i5,%i1,%i5
	or	%l7,%i5,%l7
	st	%l7,[%i2]
	inc	4,%i2
	deccc	%i4
LY95:					! [internal]
	bneg,a	LY94
	tst	%l4
	cmp	%i0,-1
	bne,a	L77237
	and	%i0,%i1,%i0
	ld	[%i2],%o2
LY93:					! [internal]
	deccc	%i4
	or	%o2,%i1,%o2
	st	%o2,[%i2]
	bneg	L77240
	inc	4,%i2
	b	LY93
	ld	[%i2],%o2
L77237:
	ld	[%i2],%i1
	deccc	%i4
	or	%i1,%i0,%i1
	st	%i1,[%i2]
	bpos	L77237
	inc	4,%i2
L77240:
	tst	%l4
LY94:					! [internal]
	be,a	L77542
	mov	0,%i2
	ld	[%i2],%i0
	and	%l4,%i3,%l4
	or	%i0,%l4,%i0
	b	LY22
	st	%i0,[%i2]
L461:
	tst	%i5
	be,a	LY92
	deccc	%i4
	ld	[%i2],%l7
	xor	%i5,-1,%i5
	or	%i5,%i1,%i5
	and	%l7,%i5,%l7
	st	%l7,[%i2]
	inc	4,%i2
	deccc	%i4
LY92:					! [internal]
	bneg,a	LY91
	tst	%l4
	cmp	%i0,-1
	bne,a	LY90
	xor	%i0,-1,%i0
	ld	[%i2],%o1
LY89:					! [internal]
	deccc	%i4
	and	%o1,%i1,%o1
	st	%o1,[%i2]
	bneg	L77254
	inc	4,%i2
	b	LY89
	ld	[%i2],%o1
LY90:					! [internal]
	or	%i0,%i1,%i0
L77251:
	ld	[%i2],%i1
	deccc	%i4
	and	%i1,%i0,%i1
	st	%i1,[%i2]
	bpos	L77251
	inc	4,%i2
L77254:
	tst	%l4
LY91:					! [internal]
	be,a	L77542
	mov	0,%i2
	ld	[%i2],%i0
	xor	%l4,-1,%l4
	or	%l4,%i3,%l4
	and	%i0,%l4,%i0
	b	LY22
	st	%i0,[%i2]
L482:
	tst	%i5
	be,a	LY88
	deccc	%i4
	ld	[%i2],%l7
	or	%l7,%i1,%o5
	and	%i5,%o5,%o5
	xor	%l7,%o5,%l7
	st	%l7,[%i2]
	inc	4,%i2
	deccc	%i4
LY88:					! [internal]
	bneg,a	LY87
	tst	%l4
	cmp	%i0,-1
	bne,a	LY86
	ld	[%i2],%i5
	ld	[%i2],%o2
LY85:					! [internal]
	deccc	%i4
	xor	%o2,-1,%o2
	and	%o2,%i1,%o2
	st	%o2,[%i2]
	bneg	L77268
	inc	4,%i2
	b	LY85
	ld	[%i2],%o2
L77265:
	ld	[%i2],%i5
LY86:					! [internal]
	deccc	%i4
	or	%i5,%i1,%o5
	and	%i0,%o5,%o5
	xor	%i5,%o5,%i5
	st	%i5,[%i2]
	bpos	L77265
	inc	4,%i2
L77268:
	tst	%l4
LY87:					! [internal]
	be,a	L77542
	mov	0,%i2
	ld	[%i2],%i4
	or	%i4,%i3,%i3
	and	%l4,%i3,%l4
	xor	%i4,%l4,%i4
	b	LY22
	st	%i4,[%i2]
L503:
	tst	%i5
	be,a	LY84
	deccc	%i4
	ld	[%i2],%l7
!	and	%i5,%l7,%i5
!	and	%i5,%i1,%i5
	xor	%l7,-1,%o5
	or	%i1,%o5,%o5
	and	%i5,%o5,%i5
	xor	%l7,%i5,%l7
	st	%l7,[%i2]
	inc	4,%i2
	deccc	%i4
LY84:					! [internal]
	bneg,a	LY83
	tst	%l4
	cmp	%i0,-1
	bne,a	LY82
	ld	[%i2],%i5
	ld	[%i2],%o4
LY81:					! [internal]
	deccc	%i4
	and	%o4,%i1,%o4
	xor	%o4,-1,%o4
	st	%o4,[%i2]
	bneg	L77282
	inc	4,%i2
	b	LY81
	ld	[%i2],%o4
L77279:
	ld	[%i2],%i5
LY82:					! [internal]
	deccc	%i4
!	and	%i0,%i5,%o0
!	and	%o0,%i1,%o0
	xor	%i5,-1,%o0
	or	%i1,%o0,%o0
	and	%i0,%o0,%o0
	xor	%i5,%o0,%i5
	st	%i5,[%i2]
	bpos	L77279
	inc	4,%i2
L77282:
	tst	%l4
LY83:					! [internal]
	be,a	L77542
	mov	0,%i2
	ld	[%i2],%i4
!	and	%l4,%i4,%l4
!	and	%l4,%i3,%l4
	xor	%i4,-1,%o5
	or	%o5,%i3,%o5
	and	%l4,%o5,%l4
	xor	%i4,%l4,%i4
	b	LY22
	st	%i4,[%i2]
L77285:
	dec	4,%l1
	cmp	%l1,10
	bgu	LY22
	sll	%l1,2,%l1
	sethi	%hi(L2000009),%o1
	or	%o1,%lo(L2000009),%o1	! [internal]
	ld	[%l1+%o1],%o0
	jmp	%o0
	nop
L2000009:
	.word	L482
	.word	L77286
	.word	L419
	.word	L503
	.word	L461
	.word	L77286
	.word	L77286
	.word	L77286
	.word	L77286
	.word	L77286
	.word	L440
L77286:
	b	L77542
	mov	0,%i2
LY109:					! [internal]
	bge,a	LY80
	ld	[%l2+4],%l1
	sub	%g0,%i5,%i5
	mov	%i1,%l4
	mov	%i3,%i1
	tst	%i5
	sub	%g0,%l5,%l5
	be	L77293
	mov	%l4,%i3
	mov	%i2,%l4
	mov	%i4,%i2
	mov	%l4,%i4
L77293:
	ld	[%l2+4],%l1
LY80:					! [internal]
	ld	[%l2+8],%l4
	tst	%i5
	bge,a	L77296
	st	%g0,[%fp-4]
	sub	%l4,1,%o4
	mov	2,%o0
	sub	%g0,%i5,%i5
	sub	%o4,%i4,%i4
	sub	%o4,%i2,%i2
	st	%o0,[%fp-4]
L77296:
	tst	%l0
	bne,a	LY79
	cmp	%l5,%i5
	tst	%i4
	bl,a	L77542
	mov	0,%i2
	cmp	%i2,%l4
	bge,a	L77542
	mov	0,%i2
	tst	%i3
	bl,a	L77542
	mov	0,%i2
	cmp	%i1,%l1
	bge,a	L77542
	mov	0,%i2
	cmp	%l5,%i5
LY79:					! [internal]
	bge,a	LY78
	sra	%l5,1,%o3
	ld	[%fp-4],%o1
	mov	%i1,%l2
	mov	%i2,%i1
	mov	%l2,%i2
	mov	%i3,%l2
	mov	%i4,%i3
	mov	%l2,%i4
	mov	%l5,%l2
	mov	%i5,%l5
	mov	%l2,%i5
	mov	%l1,%l2
	mov	%l4,%l1
	or	%o1,1,%o1
	st	%o1,[%fp-4]
	mov	%l2,%l4
	sra	%l5,1,%o3
LY78:					! [internal]
	st	%o3,[%fp-16]
	sub	%g0,%o3,%o3
	tst	%l0
	mov	%i1,%l2
	st	%i2,[%fp-12]
	st	%o3,[%fp-8]
	bne	L77319
	st	%o3,[%fp-20]
	tst	%i1
	bge,a	LY77
	ld	[%fp-12],%o0
	mov	%i5,%o1
	call	.mul,2
	sub	%g0,%i1,%o0
	ld	[%fp-20],%o7
	sub	%l5,1,%o1
	add	%o7,%o0,%o0
	st	%o0,[%fp-8]
	add	%o0,%o1,%o0
	call	.div,2
	mov	%l5,%o1
	add	%i2,%o0,%o2
	cmp	%o2,%l4
	mov	0,%l2
	bge	LY22
	st	%o2,[%fp-12]
	call	.mul,2
	mov	%l5,%o1
	ld	[%fp-8],%o4
	sub	%o4,%o0,%o4
	st	%o4,[%fp-8]
	ld	[%fp-12],%o0
LY77:					! [internal]
	tst	%o0
	bge,a	LY76
	cmp	%i3,%l1
	call	.mul,2
	mov	%l5,%o1
	ld	[%fp-8],%o7
	st	%g0,[%fp-12]
	add	%o7,%o0,%o7
	sub	%g0,%l5,%o0
	add	%o0,%i5,%o0
	mov	%i5,%o1
	sub	%o0,%o7,%o0
	st	%o7,[%fp-8]
	call	.div,2
	nop
	add	%l2,%o0,%l2
	call	.mul,2
	mov	%i5,%o1
	ld	[%fp-8],%o2
	cmp	%l2,%l1
	add	%o2,%o0,%o2
	bge	LY22
	st	%o2,[%fp-8]
	cmp	%i3,%l1
LY76:					! [internal]
	bl,a	LY75
	cmp	%i4,%l4
	sub	%l1,1,%o0
	mov	%o0,%i3
	sra	%l5,1,%o4
	st	%o4,[%fp-16]
	sub	%o0,%i1,%o0
	call	.mul,2
	mov	%i5,%o1
	ld	[%fp-20],%o5
	mov	%l5,%o1
	add	%o5,%o0,%o0
	add	%o0,%l5,%o0
	call	.div,2
	dec	%o0
	add	%i2,%o0,%i4
	cmp	%i4,%l4
LY75:					! [internal]
	bl,a	LY74
	ld	[%fp-4],%l0
	sub	%l4,1,%o0
	sub	%o0,%i2,%o0
	call	.mul,2
	mov	%l5,%o1
	ld	[%fp-16],%o1
	add	%o1,%o0,%o0
	call	.div,2
	mov	%i5,%o1
	add	%i1,%o0,%i3
L77319:
	ld	[%fp-4],%l0
LY74:					! [internal]
	sub	%i3,%l2,%i1
	ld	[%i0],%i3
	and	%l0,1,%l0
	tst	%l0
	mov	%i0,%i2
	be	L77321
	ld	[%i0+4],%i0
	ld	[%fp-12],%o7
	mov	%l2,%i4
	ld	[%i2+8],%l2
	mov	%l1,%l4
	b	L77322
	add	%o7,%l2,%l2
L77321:
	ld	[%i2+8],%o2
	ld	[%fp-12],%i4
	add	%l2,%o2,%l2
L77322:
	ld	[%fp-4],%o4
	andcc	%o4,2,%g0
	be	LY73
	ld	[%i2+12],%i2
	sub	%i4,%l4,%i4
	inc	%i4
	sub	%g0,%i3,%i3
	b	LY30
	sub	%i4,%i2,%i2
LY73:					! [internal]
	add	%i4,%i2,%i2
LY30:					! [internal]
	sll	%i2,16,%o1
	sll	%i3,16,%o0
	sra	%o0,16,%o0
	call	.mul,2
	sra	%o1,16,%o1
	ld	[%fp-8],%i2
	add	%i0,%o0,%i4
	tst	%l3
	be	L77539
	mov	1,%i0
	tst	%l0
	be,a	LY72
	cmp	%l3,3
	mov	%i3,%i0
	b	L77539
	mov	1,%i3
L77329:
	sethi	%hi(0x8000),%o2
	sll	%o2,16,%l6
	srl	%l6,16,%l6
	mov	%l6,%l4
	and	%l2,15,%i0
	srl	%l4,%i0,%i0
	sra	%l2,4,%o7
	sll	%o7,1,%o7
	sll	%i0,16,%i0
	tst	%l0
	add	%i4,%o7,%i4
	be	L77389
	srl	%i0,16,%i0
	tst	%l7
	be	L624
	cmp	%l7,5
	be	L608
	cmp	%l7,15
	bne,a	L77542
	mov	0,%i2
	mov	%l4,%l6
	tst	%i0
LY69:					! [internal]
	bne,a	LY71
	mov	%i0,%l7
	inc	2,%i4
	mov	%l6,%i0
	mov	%i0,%l7
LY71:					! [internal]
	lduh	[%i4],%o7
LY70:					! [internal]
	add	%i2,%i5,%i2
	or	%o7,%l7,%o7
	sth	%o7,[%i4]
	tst	%i2
	bg	L598
	add	%i4,%i3,%i4
	deccc	%i1
	bpos,a	LY70
	lduh	[%i4],%o7
L598:
	srl	%i0,1,%i0
	sll	%i0,16,%i0
	srl	%i0,16,%i0
	sll	%i0,16,%i0
	deccc	%i1
	sub	%i2,%l5,%i2
	bneg	LY22
	srl	%i0,16,%i0
	b	LY69
	tst	%i0
L608:
	mov	%l4,%l6
	tst	%i0
LY66:					! [internal]
	bne,a	LY68
	mov	%i0,%l7
	inc	2,%i4
	mov	%l6,%i0
	mov	%i0,%l7
LY68:					! [internal]
	lduh	[%i4],%o2
LY67:					! [internal]
	add	%i2,%i5,%i2
	xor	%o2,%l7,%o2
	sth	%o2,[%i4]
	tst	%i2
	bg	L614
	add	%i4,%i3,%i4
	deccc	%i1
	bpos,a	LY67
	lduh	[%i4],%o2
L614:
	srl	%i0,1,%i0
	sll	%i0,16,%i0
	srl	%i0,16,%i0
	sll	%i0,16,%i0
	deccc	%i1
	sub	%i2,%l5,%i2
	bneg	LY22
	srl	%i0,16,%i0
	b	LY66
	tst	%i0
L624:
	xor	%l4,-1,%l6
	xor	%i0,-1,%l7
	sll	%l7,16,%l7
	sll	%l6,16,%l6
	sethi	%hi(0xffff),%o7
	sethi	%hi(0x8000),%o5
	mov	%o5,%l4
	add	%o7,%lo(0xffff),%l3
	srl	%l6,16,%l6
	srl	%l7,16,%l7
	cmp	%l7,%l3
LY63:					! [internal]
	bne,a	LY65
	mov	%l7,%i0
	inc	2,%i4
	mov	%l6,%l7
	mov	%l7,%i0
LY65:					! [internal]
	lduh	[%i4],%o4
LY64:					! [internal]
	add	%i2,%i5,%i2
	and	%o4,%i0,%o4
	sth	%o4,[%i4]
	tst	%i2
	bg	L630
	add	%i4,%i3,%i4
	deccc	%i1
	bpos,a	LY64
	lduh	[%i4],%o4
L630:
	srl	%l7,1,%l7
	sll	%l7,16,%l7
	srl	%l7,16,%l7
	or	%l7,%l4,%l7
	sll	%l7,16,%l7
	deccc	%i1
	sub	%i2,%l5,%i2
	bneg	LY22
	srl	%l7,16,%l7
	b	LY63
	cmp	%l7,%l3
L77364:
	tst	%i0
LY61:					! [internal]
	bne,a	LY62
	or	%l7,%i0,%l7
	sth	%l7,[%i4]
	inc	2,%i4
	lduh	[%i4],%l7
	mov	%l6,%i0
	or	%l7,%i0,%l7
LY62:					! [internal]
	srl	%i0,1,%i0
	add	%i2,%i5,%i2
	tst	%i2
	sll	%i0,16,%i0
	sll	%l7,16,%l7
	srl	%i0,16,%i0
	bg	L651
	srl	%l7,16,%l7
	deccc	%i1
	bpos,a	LY61
	tst	%i0
L651:
	deccc	%i1
	sth	%l7,[%i4]
	sub	%i2,%l5,%i2
	bneg	LY22
	add	%i4,%i3,%i4
	b	L77364
	lduh	[%i4],%l7
L77372:
	lduh	[%i4],%l7
L77373:
	tst	%i0
LY59:					! [internal]
	bne,a	LY60
	xor	%l7,%i0,%l7
	sth	%l7,[%i4]
	inc	2,%i4
	lduh	[%i4],%l7
	mov	%l6,%i0
	xor	%l7,%i0,%l7
LY60:					! [internal]
	srl	%i0,1,%i0
	add	%i2,%i5,%i2
	tst	%i2
	sll	%i0,16,%i0
	sll	%l7,16,%l7
	srl	%i0,16,%i0
	bg	L667
	srl	%l7,16,%l7
	deccc	%i1
	bpos,a	LY59
	tst	%i0
L667:
	deccc	%i1
	sth	%l7,[%i4]
	sub	%i2,%l5,%i2
	bneg	LY22
	add	%i4,%i3,%i4
	b	L77373
	lduh	[%i4],%l7
L77381:
	lduh	[%i4],%l7
L77382:
	tst	%i0
LY57:					! [internal]
	bne,a	LY58
	andn	%l7,%i0,%l7
	sth	%l7,[%i4]
	inc	2,%i4
	lduh	[%i4],%l7
	mov	%l6,%i0
	andn	%l7,%i0,%l7
LY58:					! [internal]
	srl	%i0,1,%i0
	add	%i2,%i5,%i2
	tst	%i2
	sll	%i0,16,%i0
	sll	%l7,16,%l7
	srl	%i0,16,%i0
	bg	L683
	srl	%l7,16,%l7
	deccc	%i1
	bpos,a	LY57
	tst	%i0
L683:
	deccc	%i1
	sth	%l7,[%i4]
	sub	%i2,%l5,%i2
	bneg	LY22
	add	%i4,%i3,%i4
	b	L77382
	lduh	[%i4],%l7
L77389:
	tst	%l7
	be	L77381
	cmp	%l7,5
	be	L77372
	cmp	%l7,15
	be,a	L77364
	lduh	[%i4],%l7
	b	L77542
	mov	0,%i2

L77393:

/*
 * depth 8, generic, no mask, op = PIX_SRC
 */
	ld	[%fp+92],%l4	! k = color
	and	%l6,255,%l3	! m = planes
!	mov	%l3,%l6
	cmp	%l6,255
	bne	L767		! used to follow next inst.
	add	%i4,%l2,%i4	! d = daddr + startx
!	and	%l4,255,%l4
!	call	__line_tag,1
!	mov	808,%o0
!	ld	[%fp-8],%o3	! error = initerror (already in i2)
!	st	%i0,[%sp+92]	! maj_offset
!	st	%i3,[%sp+96]	! min_offset
!	mov	%i4,%o0		! d
!	mov	%l4,%o1		! k
!	mov	%i1,%o2		! count
!	mov	%l5,%o4		! dx
!	call	__line_tag_args,6
!	mov	%i5,%o5		! dy
	cmp	%l7,12		! op == 12
	bne,a	LY56
	dec	4,%l7
/*
	add	%i2,%i5,%i2	! error += dy
LY55:					! [internal]
	tst	%i2
	ble	L77398
	stb	%l4,[%i4]	! *d = k
	sub	%i2,%l5,%i2	! error -= dx
	add	%i4,%i3,%i4	! d += min_offset
L77398:
	deccc	%i1		! --count
	bneg	LY22
	add	%i4,%i0,%i4	! d += maj_offset
	b	LY55
	add	%i2,%i5,%i2	! error += dy
*/

/* begin */
1:	addcc	%i2,%i5,%i2	! error += dy
	ble	2f
	stb	%l4,[%i4]	! *d = k
	sub	%i2,%l5,%i2	! error -= dx
	add	%i4,%i3,%i4	! d += min_offset
2:	deccc	%i1		! --count
	bpos	1b
	add	%i4,%i0,%i4	! d += maj_offset
	ret
	restore	%g0,%g0,%o0
/* end */

L716:
	mov	%l4,%l7
	ldub	[%i4],%o1
LY54:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	xor	%o1,%l7,%o1
	ble	L77404
	stb	%o1,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77404:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY54
	ldub	[%i4],%o1
L726:
	mov	%l4,%l7
	ldub	[%i4],%o0
LY53:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	or	%o0,%l7,%o0
	ble	L77409
	stb	%o0,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77409:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY53
	ldub	[%i4],%o0
L736:
	mov	%l4,%l7
	ldub	[%i4],%o7
LY52:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	and	%o7,%l7,%o7
	ble	L77414
	stb	%o7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77414:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY52
	ldub	[%i4],%o7
L746:
	mov	%l4,%l7
	ldub	[%i4],%o5
LY51:					! [internal]
	add	%i2,%i5,%i2
	xor	%o5,-1,%o5
	and	%o5,%l7,%o5
	tst	%i2
	ble	L77419
	stb	%o5,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77419:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY51
	ldub	[%i4],%o5
L756:
	mov	%l4,%l7
	ldub	[%i4],%o5
LY50:					! [internal]
	add	%i2,%i5,%i2
	and	%o5,%l7,%o5
	xor	%o5,-1,%o5
	tst	%i2
	ble	L77424
	stb	%o5,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77424:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY50
	ldub	[%i4],%o5
LY56:					! [internal]
	cmp	%l7,10
	bgu	LY22
	sll	%l7,2,%l7
	sethi	%hi(L2000012),%o1
	or	%o1,%lo(L2000012),%o1	! [internal]
	ld	[%l7+%o1],%o0
	jmp	%o0
	nop
L2000012:
	.word	L746
	.word	L77427
	.word	L716
	.word	L756
	.word	L736
	.word	L77427
	.word	L77427
	.word	L77427
	.word	L77427
	.word	L77427
	.word	L726
L77427:
	b	L77542
	mov	0,%i2
L767:
	cmp	%l7,12
	bne,a	LY49
	dec	4,%l7
	and	%l6,%l4,%l7
	xor	%l6,-1,%l6
	and	%l6,255,%l6
	and	%l7,255,%l7
	ldub	[%i4],%o4
LY48:					! [internal]
	add	%i2,%i5,%i2
	and	%o4,%l6,%o4
	or	%o4,%l7,%o4
	tst	%i2
	ble	L77434
	stb	%o4,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77434:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY48
	ldub	[%i4],%o4
L785:
	and	%l6,%l4,%l7
	and	%l7,255,%l7
	ldub	[%i4],%o0
LY47:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	xor	%o0,%l7,%o0
	ble	L77440
	stb	%o0,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77440:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY47
	ldub	[%i4],%o0
L795:
	and	%l6,%l4,%l7
	and	%l7,255,%l7
	ldub	[%i4],%o2
LY46:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	or	%o2,%l7,%o2
	ble	L77445
	stb	%o2,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77445:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY46
	ldub	[%i4],%o2
L805:
	xor	%l6,-1,%l6
	or	%l6,%l4,%l7
	and	%l7,255,%l7
	ldub	[%i4],%o5
LY45:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	and	%o5,%l7,%o5
	ble	L77450
	stb	%o5,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77450:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY45
	ldub	[%i4],%o5
L815:
	mov	%l4,%l6
	mov	%l3,%l4
	ldub	[%i4],%l7
LY44:					! [internal]
	add	%i2,%i5,%i2
	or	%l7,%l6,%o7
	and	%l4,%o7,%o7
	xor	%l7,%o7,%l7
	tst	%i2
	ble	L77455
	stb	%l7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77455:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY44
	ldub	[%i4],%l7
L825:
	mov	%l4,%l6
	mov	%l3,%l4
	ldub	[%i4],%l7
LY43:					! [internal]
	add	%i2,%i5,%i2
!	and	%l4,%l7,%o1
!	and	%o1,%l6,%o1
	xor	%l7,-1,%o1
	or	%o1,%l6,%o1
	and	%l4,%o1,%o1
	xor	%l7,%o1,%l7
	tst	%i2
	ble	L77460
	stb	%l7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77460:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY43
	ldub	[%i4],%l7
LY49:					! [internal]
	cmp	%l7,10
	bgu	LY22
	sll	%l7,2,%l7
	sethi	%hi(L2000013),%o1
	or	%o1,%lo(L2000013),%o1	! [internal]
	ld	[%l7+%o1],%o0
	jmp	%o0
	nop
L2000013:
	.word	L815
	.word	L77463
	.word	L785
	.word	L825
	.word	L805
	.word	L77463
	.word	L77463
	.word	L77463
	.word	L77463
	.word	L77463
	.word	L795
L77463:
	b	L77542
	mov	0,%i2
L77467:
	ld	[%fp+92],%l4
	cmp	%l7,12
	sll	%l4,16,%l4
	sll	%l6,16,%l3
	sll	%l2,1,%l2
	srl	%l4,16,%l4
	srl	%l3,16,%l3
	bne	L77499
	add	%i4,%l2,%i4
	xor	%l3,-1,%l6
	and	%l3,%l4,%l7
	sll	%l7,16,%l7
	sll	%l6,16,%l6
	srl	%l6,16,%l6
	srl	%l7,16,%l7
	lduh	[%i4],%o7
LY42:					! [internal]
	add	%i2,%i5,%i2
	and	%o7,%l6,%o7
	or	%o7,%l7,%o7
	tst	%i2
	ble	L77471
	sth	%o7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77471:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY42
	lduh	[%i4],%o7
L854:
	and	%l3,%l4,%l7
	sll	%l7,16,%l7
	srl	%l7,16,%l7
	lduh	[%i4],%o3
LY41:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	xor	%o3,%l7,%o3
	ble	L77477
	sth	%o3,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77477:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY41
	lduh	[%i4],%o3
L864:
	and	%l3,%l4,%l7
	sll	%l7,16,%l7
	srl	%l7,16,%l7
	lduh	[%i4],%o7
LY40:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	or	%o7,%l7,%o7
	ble	L77482
	sth	%o7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77482:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY40
	lduh	[%i4],%o7
L874:
	xor	%l3,-1,%l7
	or	%l7,%l4,%l7
	sll	%l7,16,%l7
	srl	%l7,16,%l7
	lduh	[%i4],%o3
LY39:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	and	%o3,%l7,%o3
	ble	L77487
	sth	%o3,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77487:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY39
	lduh	[%i4],%o3
L884:
	mov	%l4,%l6
	mov	%l3,%l4
	lduh	[%i4],%l7
LY38:					! [internal]
	add	%i2,%i5,%i2
	or	%l7,%l6,%o4
	and	%l4,%o4,%o4
	xor	%l7,%o4,%l7
	tst	%i2
	ble	L77492
	sth	%l7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77492:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY38
	lduh	[%i4],%l7
L894:
	mov	%l4,%l6
	mov	%l3,%l4
	lduh	[%i4],%l7
LY37:					! [internal]
	add	%i2,%i5,%i2
!	and	%l4,%l7,%o7
!	and	%o7,%l6,%o7
	xor	%l7,-1,%o7
	or	%o7,%l6,%o7
	and	%l4,%o7,%o7
	xor	%l7,%o7,%l7
	tst	%i2
	ble	L77497
	sth	%l7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77497:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY37
	lduh	[%i4],%l7
L77499:
	dec	4,%l7
	cmp	%l7,10
	bgu	LY22
	sll	%l7,2,%l7
	sethi	%hi(L2000014),%o1
	or	%o1,%lo(L2000014),%o1	! [internal]
	ld	[%l7+%o1],%o0
	jmp	%o0
	nop
L2000014:
	.word	L884
	.word	L77500
	.word	L854
	.word	L894
	.word	L874
	.word	L77500
	.word	L77500
	.word	L77500
	.word	L77500
	.word	L77500
	.word	L864
L77500:
	b	L77542
	mov	0,%i2
L77503:
	ld	[%fp+92],%l4
	cmp	%l7,12
	sll	%l2,2,%l2
	mov	%l6,%l3
	bne	L77535
	add	%i4,%l2,%i4
	and	%l6,%l4,%l7
	xor	%l6,-1,%l6
	ld	[%i4],%o3
LY36:					! [internal]
	add	%i2,%i5,%i2
	and	%o3,%l6,%o3
	or	%o3,%l7,%o3
	tst	%i2
	ble	L77507
	st	%o3,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77507:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY36
	ld	[%i4],%o3
L923:
	and	%l6,%l4,%l7
	ld	[%i4],%o2
LY35:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	xor	%o2,%l7,%o2
	ble	L77513
	st	%o2,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77513:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY35
	ld	[%i4],%o2
L933:
	and	%l6,%l4,%l7
	ld	[%i4],%o0
LY34:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	or	%o0,%l7,%o0
	ble	L77518
	st	%o0,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77518:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY34
	ld	[%i4],%o0
L943:
	xor	%l6,-1,%l7
	or	%l7,%l4,%l7
	ld	[%i4],%o7
LY33:					! [internal]
	add	%i2,%i5,%i2
	tst	%i2
	and	%o7,%l7,%o7
	ble	L77523
	st	%o7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77523:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY33
	ld	[%i4],%o7
L77526:
	ld	[%i4],%l7
LY32:					! [internal]
	add	%i2,%i5,%i2
	or	%l7,%l4,%o3
	and	%l3,%o3,%o3
	xor	%l7,%o3,%l7
	tst	%i2
	ble	L77528
	st	%l7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77528:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY32
	ld	[%i4],%l7
L77531:
	ld	[%i4],%l7
LY31:					! [internal]
	add	%i2,%i5,%i2
!	and	%l3,%l7,%o1
!	and	%o1,%l4,%o1
	xor	%l7,-1,%o1
	or	%o1,%l4,%o1
	and	%l3,%o1,%o1
	xor	%l7,%o1,%l7
	tst	%i2
	ble	L77533
	st	%l7,[%i4]
	sub	%i2,%l5,%i2
	add	%i4,%i3,%i4
L77533:
	deccc	%i1
	bneg	LY22
	add	%i4,%i0,%i4
	b	LY31
	ld	[%i4],%l7
L77535:
	dec	4,%l7
	cmp	%l7,10
	bgu	LY22
	sll	%l7,2,%l7
	sethi	%hi(L2000015),%o1
	or	%o1,%lo(L2000015),%o1	! [internal]
	ld	[%l7+%o1],%o0
	jmp	%o0
	nop
L2000015:
	.word	L77526
	.word	L77536
	.word	L923
	.word	L77531
	.word	L943
	.word	L77536
	.word	L77536
	.word	L77536
	.word	L77536
	.word	L77536
	.word	L933
L77536:
	b	L77542
	mov	0,%i2
L77539:
	cmp	%l3,3
LY72:					! [internal]
	bgu	LY22
	sll	%l3,2,%o0
	sethi	%hi(L2000016),%o1
	or	%o1,%lo(L2000016),%o1	! [internal]
	ld	[%o0+%o1],%o0
	jmp	%o0
	nop
L2000016:
	.word	L77329
	.word	L77393
	.word	L77467
	.word	L77503
LY22:					! [internal]
	mov	0,%i2
L77542:
	ret
	restore	%g0,%i2,%o0
	.seg	"text"			! [internal]
_sccsid:
	.half	0x4028
	.half	0x2329
	.half	0x6d65
	.half	0x6d5f
	.half	0x7665
	.half	0x632e
	.half	0x6320
	.half	0x342e
	.half	0x3120
	.half	0x3837
	.half	0x2f31
	.half	0x302f
	.half	0x3239
	.half	0x2053
	.half	0x4d49
	.half	0
L45:
	.half	0x2814
	.half	0x181c
	.half	0x436
	.half	0x607
	.half	0x816
	.half	0xa1e
	.half	0xc17
	.half	0xe3e
	.half	0x2c14
	.half	0x181c
	.half	0x436
	.half	0x607
	.half	0x816
	.half	0xa1e
	.half	0xc17
	.half	0xe3c

	.align	4
