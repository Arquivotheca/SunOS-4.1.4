	.data
	.asciz	"@(#)mem_bres.S 1.1 94/10/31 Copyr 1983 Sun Micro"
	.even
	.text

|	Copyright (c) 1983 by Sun Microsystems, Inc.

	.text
	.globl	_mem_bres_colseg_set,_mem_bres_rowsegs_set,_mem_bres_rowsegl_set
	.globl	_mem_vertln_set, _mem_horizln_set

dx = d7		| input params for _mem_bres_colseg_set
dy = d6
cnt = d5
err = d4
skew = d3
adr = a5
vert = a4
enderr = a3

bias = d7	| other variables for _mem_bres_colseg_set
inc = d3
lcnt = d2
slen = d1
bit = d0

_mem_bres_colseg_set:
	clrw	bit
	moveq	#15,d1
	subw	skew,d1
	bset	d1,bit
	subqw	#1,cnt
	blt	lastcs
	negl	err
	divu	dy,err
1$:	orw	bit,adr@
	addl	vert,adr
	dbf	err,1$
	rorw	#1,bit
	bcc	2$
	addql	#2,adr

2$:	swap	err
	negw	err
	addw	dy,err
	subw	dx,err
	tstw	cnt
	beq	lastcs

	movl	dx,slen
	divu	dy,slen
	subqw	#1,slen
	movl	slen,inc
	swap	inc
	negw	inc
	addw	inc,bias
	addw	bias,err
	bra	testcs

longcs:	orw	bit,adr@
	addl	vert,adr
	addw	dy,err

shortcs:movw	slen,lcnt
1$:	orw	bit,adr@
	addl	vert,adr
	dbf	lcnt,1$

	rorw	#1,bit
	bcc	2$
	addql	#2,adr

2$:	addw	inc,err
testcs:	dble	cnt,shortcs
	tstw	cnt
	dblt	cnt,longcs

	subw	bias,err

lastcs:	extl	err
	negl	err
	addw	enderr,err
	divu	dy,err
1$:	orw	bit,adr@
	addl	vert,adr
	dbf	err,1$
	rts


dx = d7		| input params for _mem_bres_rowsegs_set
dy = d6
cnt = d5
err = d4
skew = d3
adr = a5
vert = a4
enderr = a3
dxclip = a2

bias = d7	| other variables for _mem_bres_rowsegs_set
lbl = d3
bl = d2
lbs = d1
bs = d0
endskew = a2
inc = a1

_mem_bres_rowsegs_set:
	addw	skew,endskew
	subqw	#1,cnt
	blt	lastr1s
	negl	err
	divu	dy,err
	movl	#0x8000,d0
	asrw	err,d0
	rorl	skew,d0
	btst	#0,d0
	beq	1$
	orw	d0,adr@+
	swap	d0
1$:	orw	d0,adr@
	addl	vert,adr

	addw	err,skew
	addqw	#1,skew
	andw	#0xF,skew
	swap	err
	negw	err
	addw	dy,err
	subw	dx,err
	tstw	cnt
	beq	lastr1s

	movl	dx,lbs
	divu	dy,lbs
	
	movl	lbs,d2
	subqw	#1,d2
	movl	#0x8000,bs
	asrw	d2,bs
	rorl	skew,bs

	addw	dx,err
	swap	d2
	negw	d2
	movw	d2,inc
	addw	inc,bias

	movw	lbs,lbl			| lbl = lbs + 1
	addqw	#1,lbl
	movl	bs,bl			| create bl
	rorl	#1,bl
	orl	bs,bl			| initialize N cond. code
	bra	testrs

SBs:	orw	bs,adr@+		| write trailing edge short bar
	swap	bs			| get ready to do
	swap	bl			| leading edge

SIs:	orw	bs,adr@			| write short bar, interior
	addl	vert,adr
	rorl	lbs,bs			| move bars across
	rorl	lbs,bl

testrs:	bge	Is			| are we at boundary?

Bs:	addw	inc,err			| Bresenham test, boundary
	dble	cnt,SBs
	tstw	cnt
	dblt	cnt,LBs
	bra	lastrs

LIs:	btst	#0,bl			| check for writing to LSB
	beq	LI1s			| if not, then really an interior write

LBs:	orw	bl,adr@+		| write trailing edge long bar
	swap	bs			| get ready to do
	swap	bl			| leading edge

LI1s:	orw	bl,adr@			| write long bar, interior
	addl	vert,adr
	addw	dy,err			| adjust err for long segment
	rorl	lbl,bs			| move bars across
	rorl	lbl,bl

	blt	Bs			| are we at boundary?

Is:	addw	inc,err			| Bresenham test, interior
	dble	cnt,SIs
	tstw	cnt
	dblt	cnt,LIs

lastrs:	subw	bias,err
lastr1s:extl	err
	negl	err
	addw	enderr,err
	divu	dy,err
	movw	endskew,skew
	subw	err,skew
	andw	#0xF,skew
	movl	#0x8000,d0
	asrw	err,d0
	rorl	skew,d0
	bge	1$
	orw	d0,adr@+
	swap	d0
1$:	orw	d0,adr@
	rts


dx = d7		| input params for _mem_bres_rowsegl_set
dy = d6
cnt = d5
err = d4
skew = d3
adr = a5
vert = a4
enderr = a3

bias = d7	| other variables for _mem_bres_rowsegl_set
lencnt = d2
slen = a2
inc = a1
length = a0

_mem_bres_rowsegl_set:
	movl	#0x0000FFFF,d1

	subqw	#1,cnt
	blt	lastls

	negl	err
	divu	dy,err
	movw	err,d0
	addqw	#1,d0
	addw	skew,d0
	cmpw	#16,d0
	bge	1$
	movw	#0x8000,d2
	asrw	err,d2
	lsrw	skew,d2
	orw	d2,adr@
	addl	vert,adr
	movw	d0,skew
	bra	nxtls

1$:	movw	d1,d2
	lsrw	skew,d2
	orw	d2,adr@+
	movw	d0,skew
	andw	#0xF,skew
	moveq	#-16,lencnt
	extl	d0
	addl	d0,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	3$
2$:	orw	d1,adr@+
	dbf	lencnt,2$
3$:	swap	lencnt
	rolw	#4,lencnt
	movl	d1,d0
	swap	d0
	lsrl	lencnt,d0
	orw	d0,adr@
	addl	vert,adr

nxtls:	swap	err
	negw	err
	addw	dy,err
	subw	dx,err
	tstw	cnt
	beq	lastls

	movl	dx,d0
	divu	dy,d0
	movl	d0,d2
	extl	d0
	movl	d0,slen
	movl	slen,length
	swap	d2
	negw	d2
	movw	d2,inc
	addw	inc,bias
	addw	bias,err
	bra	testls

longls:	addqw	#1,length
	addw	dy,err
shortls:movw	d1,d2		| Note that length >= 16 here
	lsrw	skew,d2
	orw	d2,adr@+
	moveq	#-16,lencnt
	addl	skew,lencnt
	addl	length,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	2$
1$:	orw	d1,adr@+
	dbf	lencnt,1$
2$:	swap	lencnt
	rolw	#4,lencnt
	movl	d1,d0
	swap	d0
	lsrl	lencnt,d0
	orw	d0,adr@
	addw	length,skew
	andw	#0xF,skew
	addl	vert,adr

	movl	slen,length
	addw	inc,err
testls:	dble	cnt,shortls
	tstw	cnt
	dblt	cnt,longls

	subw	bias,err

lastls:	extl	err
	negl	err
	addw	enderr,err
	divu	dy,err
	movw	err,d0
	addqw	#1,d0
	addw	skew,d0
	cmpw	#16,d0
	bgt	1$
	movw	#0x8000,d2
	asrw	err,d2
	lsrw	skew,d2
	orw	d2,adr@
	rts

1$:	movw	d1,d2
	lsrw	skew,d2
	orw	d2,adr@+
	moveq	#-16,lencnt
	extl	d0
	addl	d0,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	3$
2$:	orw	d1,adr@+
	dbf	lencnt,2$
3$:	swap	lencnt
	rolw	#4,lencnt
	swap	d1
	lsrl	lencnt,d1
	orw	d1,adr@
	rts


dy = d6		| input params for _mem_vertln_set
skew = d3
adr = a5
vert = a4

_mem_vertln_set:
	clrw	d0
	moveq	#15,d1
	subw	skew,d1
	bset	d1,d0
1$:	orw	d0,adr@
	addl	vert,adr
	dbf	dy,1$
	rts


dx = d7		| input params for _mem_horizln_set
skew = d3
adr = a5

lencnt = d2	| other variables for _mem_horizln_set

_mem_horizln_set:
	movl	#0x0000FFFF,d1

	addqw	#1,dx
	movw	dx,d0
	addw	skew,d0
	cmpw	#16,d0
	bgt	1$
	swap	d1
	lsrl	dx,d1
	lsrw	skew,d1
	orw	d1,adr@
	rts

1$:	movw	d1,d0
	lsrw	skew,d0
	orw	d0,adr@+
	moveq	#-16,lencnt
	addl	skew,lencnt
	addl	dx,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	3$
2$:	orw	d1,adr@+
	dbf	lencnt,2$
3$:	swap	lencnt
	rolw	#4,lencnt
	swap	d1
	lsrl	lencnt,d1
	orw	d1,adr@
	rts


	.text
	.globl	_mem_bres_colseg_not,_mem_bres_rowsegs_not,_mem_bres_rowsegl_not
	.globl	_mem_vertln_not, _mem_horizln_not

dx = d7		| input params for _mem_bres_colseg_not
dy = d6
cnt = d5
err = d4
skew = d3
adr = a5
vert = a4
enderr = a3

bias = d7	| other variables for _mem_bres_colseg_not
inc = d3
lcnt = d2
slen = d1
bit = d0

_mem_bres_colseg_not:
	clrw	bit
	moveq	#15,d1
	subw	skew,d1
	bset	d1,bit
	subqw	#1,cnt
	blt	lastcn
	negl	err
	divu	dy,err
1$:	eorw	bit,adr@
	addl	vert,adr
	dbf	err,1$
	rorw	#1,bit
	bcc	2$
	addql	#2,adr

2$:	swap	err
	negw	err
	addw	dy,err
	subw	dx,err
	tstw	cnt
	beq	lastcn

	movl	dx,slen
	divu	dy,slen
	subqw	#1,slen
	movl	slen,inc
	swap	inc
	negw	inc
	addw	inc,bias
	addw	bias,err
	bra	testcn

longcn:	eorw	bit,adr@
	addl	vert,adr
	addw	dy,err

shortcn:movw	slen,lcnt
1$:	eorw	bit,adr@
	addl	vert,adr
	dbf	lcnt,1$

	rorw	#1,bit
	bcc	2$
	addql	#2,adr

2$:	addw	inc,err
testcn:	dble	cnt,shortcn
	tstw	cnt
	dblt	cnt,longcn

	subw	bias,err

lastcn:	extl	err
	negl	err
	addw	enderr,err
	divu	dy,err
1$:	eorw	bit,adr@
	addl	vert,adr
	dbf	err,1$
	rts


dx = d7		| input params for _mem_bres_rowsegs_not
dy = d6
cnt = d5
err = d4
skew = d3
adr = a5
vert = a4
enderr = a3
dxclip = a2

bias = d7	| other variables for _mem_bres_rowsegs_not
lbl = d3
bl = d2
lbs = d1
bs = d0
endskew = a2
inc = a1

_mem_bres_rowsegs_not:
	addw	skew,endskew
	subqw	#1,cnt
	blt	lastr1n
	negl	err
	divu	dy,err
	movl	#0x8000,d0
	asrw	err,d0
	rorl	skew,d0
	btst	#0,d0
	beq	1$
	eorw	d0,adr@+
	swap	d0
1$:	eorw	d0,adr@
	addl	vert,adr

	addw	err,skew
	addqw	#1,skew
	andw	#0xF,skew
	swap	err
	negw	err
	addw	dy,err
	subw	dx,err
	tstw	cnt
	beq	lastr1n

	movl	dx,lbs
	divu	dy,lbs
	
	movl	lbs,d2
	subqw	#1,d2
	movl	#0x8000,bs
	asrw	d2,bs
	rorl	skew,bs

	addw	dx,err
	swap	d2
	negw	d2
	movw	d2,inc
	addw	inc,bias

	movw	lbs,lbl			| lbl = lbs + 1
	addqw	#1,lbl
	movl	bs,bl			| create bl
	rorl	#1,bl
	orl	bs,bl			| initialize N cond. code
	bra	testrn

SBn:	eorw	bs,adr@+		| write trailing edge short bar
	swap	bs			| get ready to do
	swap	bl			| leading edge

SIn:	eorw	bs,adr@			| write short bar, interior
	addl	vert,adr
	rorl	lbs,bs			| move bars across
	rorl	lbs,bl

testrn:	bge	In			| are we at boundary?

Bn:	addw	inc,err			| Bresenham test, boundary
	dble	cnt,SBn
	tstw	cnt
	dblt	cnt,LBn
	bra	lastrn

LIn:	btst	#0,bl			| check for writing to LSB
	beq	LI1n			| if not, then really an interior write

LBn:	eorw	bl,adr@+		| write trailing edge long bar
	swap	bs			| get ready to do
	swap	bl			| leading edge

LI1n:	eorw	bl,adr@			| write long bar, interior
	addl	vert,adr
	addw	dy,err			| adjust err for long segment
	rorl	lbl,bs			| move bars across
	rorl	lbl,bl

	blt	Bn			| are we at boundary?

In:	addw	inc,err			| Bresenham test, interior
	dble	cnt,SIn
	tstw	cnt
	dblt	cnt,LIn

lastrn:	subw	bias,err
lastr1n:extl	err
	negl	err
	addw	enderr,err
	divu	dy,err
	movw	endskew,skew
	subw	err,skew
	andw	#0xF,skew
	movl	#0x8000,d0
	asrw	err,d0
	rorl	skew,d0
	bge	1$
	eorw	d0,adr@+
	swap	d0
1$:	eorw	d0,adr@
	rts


dx = d7		| input params for _mem_bres_rowsegl_not
dy = d6
cnt = d5
err = d4
skew = d3
adr = a5
vert = a4
enderr = a3

bias = d7	| other variables for _mem_bres_rowsegl_not
lencnt = d2
slen = a2
inc = a1
length = a0

_mem_bres_rowsegl_not:
	movl	#0x0000FFFF,d1

	subqw	#1,cnt
	blt	lastln

	negl	err
	divu	dy,err
	movw	err,d0
	addqw	#1,d0
	addw	skew,d0
	cmpw	#16,d0
	bge	1$
	movw	#0x8000,d2
	asrw	err,d2
	lsrw	skew,d2
	eorw	d2,adr@
	addl	vert,adr
	movw	d0,skew
	bra	nxtln

1$:	movw	d1,d2
	lsrw	skew,d2
	eorw	d2,adr@+
	movw	d0,skew
	andw	#0xF,skew
	moveq	#-16,lencnt
	extl	d0
	addl	d0,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	3$
2$:	eorw	d1,adr@+
	dbf	lencnt,2$
3$:	swap	lencnt
	rolw	#4,lencnt
	movl	d1,d0
	swap	d0
	lsrl	lencnt,d0
	eorw	d0,adr@
	addl	vert,adr

nxtln:	swap	err
	negw	err
	addw	dy,err
	subw	dx,err
	tstw	cnt
	beq	lastln

	movl	dx,d0
	divu	dy,d0
	movl	d0,d2
	extl	d0
	movl	d0,slen
	movl	slen,length
	swap	d2
	negw	d2
	movw	d2,inc
	addw	inc,bias
	addw	bias,err
	bra	testln

longln:	addqw	#1,length
	addw	dy,err
shortln:movw	d1,d2		| Note that length >= 16 here
	lsrw	skew,d2
	eorw	d2,adr@+
	moveq	#-16,lencnt
	addl	skew,lencnt
	addl	length,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	2$
1$:	eorw	d1,adr@+
	dbf	lencnt,1$
2$:	swap	lencnt
	rolw	#4,lencnt
	movl	d1,d0
	swap	d0
	lsrl	lencnt,d0
	eorw	d0,adr@
	addw	length,skew
	andw	#0xF,skew
	addl	vert,adr

	movl	slen,length
	addw	inc,err
testln:	dble	cnt,shortln
	tstw	cnt
	dblt	cnt,longln

	subw	bias,err

lastln:	extl	err
	negl	err
	addw	enderr,err
	divu	dy,err
	movw	err,d0
	addqw	#1,d0
	addw	skew,d0
	cmpw	#16,d0
	bgt	1$
	movw	#0x8000,d2
	asrw	err,d2
	lsrw	skew,d2
	eorw	d2,adr@
	rts

1$:	movw	d1,d2
	lsrw	skew,d2
	eorw	d2,adr@+
	moveq	#-16,lencnt
	extl	d0
	addl	d0,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	3$
2$:	eorw	d1,adr@+
	dbf	lencnt,2$
3$:	swap	lencnt
	rolw	#4,lencnt
	swap	d1
	lsrl	lencnt,d1
	eorw	d1,adr@
	rts


dy = d6		| input params for _mem_vertln_not
skew = d3
adr = a5
vert = a4

_mem_vertln_not:
	clrw	d0
	moveq	#15,d1
	subw	skew,d1
	bset	d1,d0
1$:	eorw	d0,adr@
	addl	vert,adr
	dbf	dy,1$
	rts


dx = d7		| input params for _mem_horizln_not
skew = d3
adr = a5

lencnt = d2	| other variables for _mem_horizln_not

_mem_horizln_not:
	movl	#0x0000FFFF,d1

	addqw	#1,dx
	movw	dx,d0
	addw	skew,d0
	cmpw	#16,d0
	bgt	1$
	swap	d1
	lsrl	dx,d1
	lsrw	skew,d1
	eorw	d1,adr@
	rts

1$:	movw	d1,d0
	lsrw	skew,d0
	eorw	d0,adr@+
	moveq	#-16,lencnt
	addl	skew,lencnt
	addl	dx,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	3$
2$:	eorw	d1,adr@+
	dbf	lencnt,2$
3$:	swap	lencnt
	rolw	#4,lencnt
	swap	d1
	lsrl	lencnt,d1
	eorw	d1,adr@
	rts


	.text
	.globl	_mem_bres_colseg_clr,_mem_bres_rowsegs_clr,_mem_bres_rowsegl_clr
	.globl	_mem_vertln_clr, _mem_horizln_clr

dx = d7		| input params for _mem_bres_colseg_clr
dy = d6
cnt = d5
err = d4
skew = d3
adr = a5
vert = a4
enderr = a3

bias = d7	| other variables for _mem_bres_colseg_clr
inc = d3
lcnt = d2
slen = d1
bit = d0

_mem_bres_colseg_clr:
	moveq	#-1,bit
	moveq	#15,d1
	subw	skew,d1
	bclr	d1,bit
	subqw	#1,cnt
	blt	lastcc
	negl	err
	divu	dy,err
1$:	andw	bit,adr@
	addl	vert,adr
	dbf	err,1$
	rorw	#1,bit
	bcs	2$
	addql	#2,adr

2$:	swap	err
	negw	err
	addw	dy,err
	subw	dx,err
	tstw	cnt
	beq	lastcc

	movl	dx,slen
	divu	dy,slen
	subqw	#1,slen
	movl	slen,inc
	swap	inc
	negw	inc
	addw	inc,bias
	addw	bias,err
	bra	testcc

longcc:	andw	bit,adr@
	addl	vert,adr
	addw	dy,err

shortcc:movw	slen,lcnt
1$:	andw	bit,adr@
	addl	vert,adr
	dbf	lcnt,1$

	rorw	#1,bit
	bcs	2$
	addql	#2,adr

2$:	addw	inc,err
testcc:	dble	cnt,shortcc
	tstw	cnt
	dblt	cnt,longcc

	subw	bias,err

lastcc:	extl	err
	negl	err
	addw	enderr,err
	divu	dy,err
1$:	andw	bit,adr@
	addl	vert,adr
	dbf	err,1$
	rts


dx = d7		| input params for _mem_bres_rowsegs_clr
dy = d6
cnt = d5
err = d4
skew = d3
adr = a5
vert = a4
enderr = a3
dxclip = a2

bias = d7	| other variables for _mem_bres_rowsegs_clr
lbl = d3
bl = d2
lbs = d1
bs = d0
endskew = a2
inc = a1

_mem_bres_rowsegs_clr:
	addw	skew,endskew
	subqw	#1,cnt
	blt	lastr1c
	negl	err
	divu	dy,err
	movl	#0xFFFF7FFF,d0
	asrw	err,d0
	rorl	skew,d0
	btst	#0,d0
	bne	1$
	andw	d0,adr@+
	swap	d0
1$:	andw	d0,adr@
	addl	vert,adr

	addw	err,skew
	addqw	#1,skew
	andw	#0xF,skew
	swap	err
	negw	err
	addw	dy,err
	subw	dx,err
	tstw	cnt
	beq	lastr1c

	movl	dx,lbs
	divu	dy,lbs
	
	movl	lbs,d2
	moveq	#-1,bs
	lsrw	d2,bs
	rorl	skew,bs

	addw	dx,err
	swap	d2
	negw	d2
	movw	d2,inc
	addw	inc,bias

	movw	lbs,lbl			| lbl = lbs + 1
	addqw	#1,lbl
	movl	bs,bl			| create bl
	rorl	#1,bl
	andl	bs,bl			| initialize N cond. code
	bra	testrc

SBc:	andw	bs,adr@+		| write trailing edge short bar
	swap	bs			| get ready to do
	swap	bl			| leading edge

SIc:	andw	bs,adr@			| write short bar, interior
	addl	vert,adr
	rorl	lbs,bs			| move bars across
	rorl	lbs,bl

testrc:	blt	Ic			| are we at boundary?

Bc:	addw	inc,err			| Bresenham test, boundary
	dble	cnt,SBc
	tstw	cnt
	dblt	cnt,LBc
	bra	lastrc

LIc:	btst	#0,bl			| check for writing to LSB
	bne	LI1c			| if not, then really an interior write

LBc:	andw	bl,adr@+		| write trailing edge long bar
	swap	bs			| get ready to do
	swap	bl			| leading edge

LI1c:	andw	bl,adr@			| write long bar, interior
	addl	vert,adr
	addw	dy,err			| adjust err for long segment
	rorl	lbl,bs			| move bars across
	rorl	lbl,bl

	bge	Bc			| are we at boundary?

Ic:	addw	inc,err			| Bresenham test, interior
	dble	cnt,SIc
	tstw	cnt
	dblt	cnt,LIc

lastrc:	subw	bias,err
lastr1c:extl	err
	negl	err
	addw	enderr,err
	divu	dy,err
	movw	endskew,skew
	subw	err,skew
	andw	#0xF,skew
	addqw	#1,err
	moveq	#-1,d0
	lsrw	err,d0
	rorl	skew,d0
	blt	1$
	andw	d0,adr@+
	swap	d0
1$:	andw	d0,adr@
	rts


dx = d7		| input params for _mem_bres_rowsegl_clr
dy = d6
cnt = d5
err = d4
skew = d3
adr = a5
vert = a4
enderr = a3

bias = d7	| other variables for _mem_bres_rowsegl_clr
lencnt = d2
slen = a2
inc = a1
length = a0

_mem_bres_rowsegl_clr:
	moveq	#-1,d1		| d1 = 0xFFFF0000
	clrw	d1

	subqw	#1,cnt
	blt	lastlc

	negl	err
	divu	dy,err
	addqw	#1,err
	movw	err,d0
	addw	skew,d0
	cmpw	#16,d0
	bge	1$
	moveq	#-1,d2
	lsrw	err,d2
	rorw	skew,d2
	andw	d2,adr@
	addl	vert,adr
	movw	d0,skew
	bra	nxtlc

1$:	movl	d1,d2
	lsrl	skew,d2
	andw	d2,adr@+
	movw	d0,skew
	andw	#0xF,skew
	moveq	#-16,lencnt
	extl	d0
	addl	d0,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	3$
2$:	andw	d1,adr@+
	dbf	lencnt,2$
3$:	swap	lencnt
	rolw	#4,lencnt
	movl	d1,d0
	swap	d0
	lsrw	lencnt,d0
	andw	d0,adr@
	addl	vert,adr

nxtlc:	swap	err
	negw	err
	addw	dy,err
	subw	dx,err
	tstw	cnt
	beq	lastlc

	movl	dx,d0
	divu	dy,d0
	movl	d0,d2
	extl	d0
	movl	d0,slen
	movl	slen,length
	swap	d2
	negw	d2
	movw	d2,inc
	addw	inc,bias
	addw	bias,err
	bra	testlc

longlc:	addqw	#1,length
	addw	dy,err
shortlc:movl	d1,d2		| Note that length >= 16 here
	lsrl	skew,d2
	andw	d2,adr@+
	moveq	#-16,lencnt
	addl	skew,lencnt
	addl	length,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	2$
1$:	andw	d1,adr@+
	dbf	lencnt,1$
2$:	swap	lencnt
	rolw	#4,lencnt
	movl	d1,d0
	swap	d0
	lsrw	lencnt,d0
	andw	d0,adr@
	addw	length,skew
	andw	#0xF,skew
	addl	vert,adr

	movl	slen,length
	addw	inc,err
testlc:	dble	cnt,shortlc
	tstw	cnt
	dblt	cnt,longlc

	subw	bias,err

lastlc:	extl	err
	negl	err
	addw	enderr,err
	divu	dy,err
	addqw	#1,err
	movw	err,d0
	addw	skew,d0
	cmpw	#16,d0
	bgt	1$
	moveq	#-1,d2
	lsrw	err,d2
	rorw	skew,d2
	andw	d2,adr@
	rts

1$:	movl	d1,d2
	lsrl	skew,d2
	andw	d2,adr@+
	moveq	#-16,lencnt
	extl	d0
	addl	d0,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	3$
2$:	andw	d1,adr@+
	dbf	lencnt,2$
3$:	swap	lencnt
	rolw	#4,lencnt
	swap	d1
	lsrw	lencnt,d1
	andw	d1,adr@
	rts


dy = d6		| input params for _mem_vertln_clr
skew = d3
adr = a5
vert = a4

_mem_vertln_clr:
	moveq	#-1,d0
	moveq	#15,d1
	subw	skew,d1
	bclr	d1,d0
1$:	andw	d0,adr@
	addl	vert,adr
	dbf	dy,1$
	rts


dx = d7		| input params for _mem_horizln_clr
skew = d3
adr = a5

lencnt = d2	| other variables for _mem_horizln_clr

_mem_horizln_clr:
	moveq	#-1,d1		| d1 = 0xFFFF0000
	clrw	d1

	addqw	#1,dx
	movw	dx,d0
	addw	skew,d0
	cmpw	#16,d0
	bgt	1$
	swap	d1
	lsrw	dx,d1
	rorw	skew,d1
	andw	d1,adr@
	rts

1$:	movl	d1,d0
	lsrl	skew,d0
	andw	d0,adr@+
	moveq	#-16,lencnt
	addl	skew,lencnt
	addl	dx,lencnt
	rorl	#4,lencnt
	subqw	#1,lencnt
	bmi	3$
2$:	andw	d1,adr@+
	dbf	lencnt,2$
3$:	swap	lencnt
	rolw	#4,lencnt
	swap	d1
	lsrw	lencnt,d1
	andw	d1,adr@
	rts
