!
! @(#)pc2.il 1.1 94/10/31 SMI
!

! Inline code templates for Pascal library routines.
!
	.inline	_ACTFILE,4
	ld	[%o0+12],%o0	! _ACTFILE
	.end

	.inline	_ADDT,16
	mov	%g0,%o7		! _ADDT(result0, left, right, siz)
1:
	ld	[%o1+%o7],%o4
	ld	[%o2+%o7],%o5
	or	%o4,%o5,%o5
	st	%o5,[%o0+%o7]
	subcc	%o3,1,%o3
	bne	1b
	add	%o7,4,%o7
	.end

	.inline	_SUBT,16
	mov	%g0,%o7		! _SUBT(result0, left, right, siz)
1:
	ld	[%o1+%o7],%o4
	ld	[%o2+%o7],%o5
	andn	%o4,%o5,%o5
	st	%o5,[%o0+%o7]
	subcc	%o3,1,%o3
	bne	1b
	add	%o7,4,%o7
	.end

	.inline	_MULT,16
	mov	%g0,%o7		! _MULT(result0, left, right, siz)
1:
	ld	[%o1+%o7],%o4
	ld	[%o2+%o7],%o5
	and	%o4,%o5,%o5
	st	%o5,[%o0+%o7]
	subcc	%o3,1,%o3
	bne	1b
	add	%o7,4,%o7
	.end

	.inline	_IN,16
	sub	%o0,%o1,%o1	! _IN (element, lower, nwords, setptr)
	mov	%g0,%o0		! assume answer is zero, preload
	cmp	%o1,%o2
	bgu	1f
	and	%o1,7,%o2	! shift count
	srl	%o1,3,%o1	! byte offset in set
	ldub	[%o3+%o1],%o0	! fetch byte from set
	srl	%o0,%o2,%o0	! shift byte right 0..7 places
	and	%o0,1,%o0	! mask result to 1 bit
1:
	.end

	.inline	_NIL,4
	sethi	%hi(__minptr),%o1		! _NIL (ptr)
	ld	[%o1+%lo(__minptr)],%o1
	cmp	%o0,%o1
	bcs	1f
	nop
	sethi   %hi(__maxptr),%o1
	ld      [%o1+%lo(__maxptr)],%o1
	cmp     %o0,%o1
	bleu	2f
	nop
1:
	mov	%o0,%o1
	sethi	%hi(_ENIL),%o0
	add	%o0,%lo(_ENIL),%o0
	call	_ERROR
	nop
2:
	.end

	.inline	_INLINE_RELEQ,12
	mov	0,%o3	! _INLINE_RELEQ
1:
	ldsb	[%o1+%o3],%o4
	ldsb	[%o2+%o3],%o5
	cmp	%o4,%o5
	bne	2f
	add	%o3,1,%o3
	cmp	%o3,%o0
	bne	1b
	nop
	mov	1,%o0
	b	3f
	nop
2:
	mov	%g0,%o0
3:
	.end

	.inline	_INLINE_RELNE,12
	mov	0,%o3	! _INLINE_RELNE
1:
	ldsb	[%o1+%o3],%o4
	ldsb	[%o2+%o3],%o5
	cmp	%o4,%o5
	bne	2f
	add	%o3,1,%o3
	cmp	%o3,%o0
	bne	1b
	nop
	mov	%g0,%o0
	b	3f
	nop
2:
	mov	1,%o0
3:
	.end

	.inline	_INLINE_RELSLT,12
	mov	0,%o3	! _INLINE_RELSLT
1:
	ldsb	[%o1+%o3],%o4
	ldsb	[%o2+%o3],%o5
	cmp	%o4,%o5
	bne	2f
	add	%o3,1,%o3
	cmp	%o3,%o0
	bne	1b
	nop
2:	
	bl	3f
	nop
	mov	%g0,%o0
	b	4f
	nop
3:
	mov	1,%o0
4:
	.end

	.inline	_INLINE_RELSLE,12
	mov	0,%o3	! _INLINE_RESLE
1:
	ldsb	[%o1+%o3],%o4
	ldsb	[%o2+%o3],%o5
	cmp	%o4,%o5
	bne	2f
	add	%o3,1,%o3
	cmp	%o3,%o0
	bne	1b
	nop
2:	
	ble	3f
	nop
	mov	%g0,%o0
	b	4f
	nop
3:
	mov	1,%o0
4:
	.end

	.inline	_INLINE_RELSGT,12
	mov	0,%o3	! _INLINE_RELSGT
1:
	ldsb	[%o1+%o3],%o4
	ldsb	[%o2+%o3],%o5
	cmp	%o4,%o5
	bne	2f
	add	%o3,1,%o3
	cmp	%o3,%o0
	bne	1b
	nop
2:	
	bg	3f
	nop
	mov	%g0,%o0
	b	4f
	nop
3:
	mov	1,%o0
4:
	.end

	.inline	_INLINE_RELSGE,12
	mov	0,%o3	! _INLINE_RELSGE
1:
	ldsb	[%o1+%o3],%o4
	ldsb	[%o2+%o3],%o5
	cmp	%o4,%o5
	bne	2f
	add	%o3,1,%o3
	cmp	%o3,%o0
	bne	1b
	nop
2:	
	bge	3f
	nop
	mov	%g0,%o0
	b	4f
	nop
3:
	mov	1,%o0
4:
	.end

	.inline V_to_V,16
	mov	%g0,%o5
	cmp	%o2,%o3		! V_to_V(dest, source, maxdest, maxsource )
	bge	1f
	nop
	mov	%o2,%o3
1:
	ld	[%o1+%o5],%o2
	cmp	%o2,%o3
	bge	2f
	nop
	mov	%o2,%o3
2:
	st	%o3,[%o0+%o5]
	subcc	%o3,1,%o3
	blt	4f
	nop
	add	%o1,4,%o1
	add	%o0,4,%o0
3:
	ldub	[%o1+%o3],%o2
	stb	%o2,[%o0+%o3]
	subcc	%o3,1,%o3
	bge	3b
	nop
4:
	.end

	.inline STR_CONCAT,16
	mov	%g0,%o5
	ld	[%o2+%o5],%o4		! STR_CONCAT(dest, source, x, size)
	add	%o4,%o3,%o4
	st	%o4,[%o2+%o5]
	dec	%o3
1:
	ldub	[%o1+%o3],%o2
	stb	%o2,[%o0+%o3]
	subcc	%o3,1,%o3
	bge	1b
	nop
	.end

	.inline CHAR_CONCAT,12
	stb	%o1,[%o0+%g0]	! CHAR_CONCAT(dest, char, countaddr)
	ld	[%o2+%g0],%o0
	inc	%o0
	st	%o0,[%o2+%g0]
	.end

	.inline V_to_STR,16
	clr	%o5
	ld	[%o1+%o5],%o3	! V_to_STR(dest, source, destsize-1)
	dec	%o3
	sub	%o2,%o3,%o2
	add	%o1,4,%o1
	mov	%o3,%o5
1:
	ldub	[%o1+%o5],%o4
	stb	%o4,[%o0+%o5]
	subcc	%o5,1,%o5
	bge	1b
	nop
	addcc	%o0,%o3,%o0
2:
	mov	32,%o1
3:
	stb	%o1,[%o0+%o2]
	subcc	%o2,1,%o2
	bg	3b
	nop
	.end

	.inline V_CONCAT,12
	ld	[%o2+%g0],%o3	! V_CONCAT(destloc, source, destlen_addr)
	ld	[%o1+%g0],%o4
	add	%o3,%o4,%o3
	st	%o3,[%o2+%g0]
	add	%o1,4,%o1
	dec	%o4
1:
	ldub	[%o1+%o4],%o3
	stb	%o3,[%o0+%o4]
	subcc	%o4,1,%o4
	bge	1b
	nop
	.end
	
	.inline CHAR_to_V,8
	stb	%o1,[%o0+4]	! CHAR_to_V(dest, ch)
	mov	1,%o1
	st	%o1,[%o0+%g0]
	.end

	.inline STR_TO_V,16
	st	%o3,[%o2+%g0]	! STR_TO_V(dest, source, x, len)
	dec	%o3
1:
	ldub	[%o1+%o3],%o4
	stb	%o4,[%o0+%o3]
	subcc	%o3,1,%o3
	bge	1b
	nop
	.end

	.inline _RANG4,12
	cmp	%o0,%o2		! _RANG4(val, min, max)  --untested.
	bgt	1f
	nop
	cmp	%o0,%o1
	bge	2f
	nop
1:
	mov	%o0,%o1
	set	_ERANG,%o0
	call	_ERROR
	nop
2:
	.end

	.inline _SUBSC,12
	cmp	%o0,%o2		! _SUBSC(val, min, max)  --see RANG4.
	bgt	1f
	nop
	cmp	%o0,%o1
	bge	2f
	nop
1:
	mov	%o0,%o1
	set	_ERANG,%o0
	call	_ERROR
	nop
2:
	.end

	.inline _RSNG4,8
	cmp	%o0,%o1		! _RSNG4(val,max)  -- untested.
	ble	1f
	nop
	mov	%o0,%o1
	set	_ERANG,%o0
	call	_ERROR
	nop
1:
	.end

	.inline _SUBSCZ,8
	cmp	%o0,%o1		! _SUBSCZ(val,max)  -- see RSNG4.
	ble	1f
	nop
	mov	%o0,%o1
	set	_ERANG,%o0
	call	_ERROR
	nop
1:
	.end
