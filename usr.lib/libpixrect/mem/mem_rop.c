#ifndef lint
static char sccsid[] = "@(#)mem_rop.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986-1989 Sun Microsystems,  Inc.
 */

/*
 * Memory pixrect rasterop
 *
 * 32-bit version 
 *
 * David DiGiacomo, Sun Microsystems
 */

/*
 * Compile-time options:
 *
 * define NEVER_SLOW to generate fast code only
 * define NEVER_FAST to generate slow code only
 *
 * define NO_OPS to turn off all ops except PIX_SRC for debugging
 *
 * define SHORT_ALIGN to 0 to force 32 bit alignment of 32 bit accesses,
 *		      to 1 to allow 32 bit accesses on 16 bit boundaries
 *
 * define UNROLL to 0 to turn off loop unrolling
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/mem_rop_impl_ops.h>
#include <pixrect/mem_rop_impl_util.h>

/* shift count 32 valid test macro */
#define	IFSHIFT32	IF68000

/* basic rop data type */
typedef	u_long	ROP_T;

/* 1 -> 8 op src data type */
typedef	u_short	XOPS_T;

/* type for temporaries when writing to short dst (compiler hack) */
typedef	IFSPARC(u_long,u_short) TMP16;

/* direction macros */
#define	L_TO_R	IFTRUE
#define	R_TO_L	IFFALSE

/* loop macro */
#define	LOOP(count, isreg, op) \
	do { op; } while (isreg(LOOP_DECR(count), --(count) >= 0))

#if sparc
#define	edges ((lm) | (rm))
#endif

/* convert opcode to bit mask */
#define	OPBIT(op)	(1 << (op))

/* ops which don't need src */
#define	NOSRCBIT	(OPBIT(PIX_OPCLR) | OPBIT(PIX_OPNOT(PIX_OPDST)) | \
				OPBIT(PIX_OPDST) | OPBIT(PIX_OPSET))

/* ops for which we want to invert src */
#define	INVSRCBIT	(OPBIT(1) | OPBIT(2) | OPBIT(3) | \
				OPBIT(9) | OPBIT(11) | OPBIT(13))


/* 
 * generic function dispatch macros
 *
 *	(code op)
 *	0 clear		1 ~(d|s)	2 d & ~s	3 ~s
 *	4 ~d & s	5 ~d		6 d ^ s		7 ~(d & s)
 *	8 d & s		9 ~(d ^ s)	A d		B d|~s
 *	C s		D ~d|s		E d|s		F set
 */

#ifndef	NO_OPS

#define	CASE_FILL(op,macro,nomask,noedges) _STMT( \
	switch (op) { \
	case 0x0: \
		macro(0,UFAST,nomask,noedges); break; \
	case 0x1: \
	case 0x4: \
		macro(4,SLOW,nomask,noedges); break; \
	case 0x5: \
	case 0x6: \
	case 0x9: \
		macro(6,UFAST,nomask,noedges); break; \
	case 0x7: \
	case 0xD: \
		macro(7,SLOW,nomask,noedges); break; \
	case 0x2: \
	case 0x8: \
		macro(8,UFAST,nomask,noedges); break; \
	case 0xA: \
		break; \
	case 0x3: \
	case 0xC: \
		macro(C,FAST,nomask,noedges); break; \
	case 0xB: \
	case 0xE: \
		macro(E,UFAST,nomask,noedges); break; \
	case 0xF: \
		macro(F,UFAST,nomask,noedges); break; \
	} )

#define	CASE_XOP(op,macro,noskew,noprime,nomask,noedges) _STMT( \
	switch (op) { \
	case 0x0: \
	case 0x5: \
	case 0xA: \
	case 0xF: \
		break; \
	case 0x1: \
	case 0x4: \
		macro(4,SLOW,noskew,noprime,nomask,noedges); break; \
	case 0x6: \
	case 0x9: \
		macro(6,UFAST,noskew,noprime,nomask,noedges); break; \
	case 0x7: \
	case 0xD: \
		macro(7,SLOW,noskew,noprime,nomask,noedges); break; \
	case 0x2: \
	case 0x8: \
		macro(8,UFAST,noskew,noprime,nomask,noedges); break; \
	case 0x3: \
	case 0xC: \
		macro(C,FAST,noskew,noprime,nomask,noedges); break; \
	case 0xB: \
	case 0xE: \
		macro(E,UFAST,noskew,noprime,nomask,noedges); break; \
	} )

#define	CASE_ROP(op,macro,dir,noskew,noprime,nomask,noedges) _STMT( \
	switch (op) { \
	case 0x0: break; \
	case 0x1: macro(1,SLOW,dir,noskew,noprime,nomask,noedges); break; \
	case 0x2: macro(2,SLOW,dir,noskew,noprime,nomask,noedges); break; \
	case 0x3: macro(3,UFAST,dir,noskew,noprime,nomask,noedges); break; \
	case 0x4: macro(4,SLOW,dir,noskew,noprime,nomask,noedges); break; \
	case 0x5: break; \
	case 0x6: macro(6,FAST,dir,noskew,noprime,nomask,noedges); break; \
	case 0x7: macro(7,SLOW,dir,noskew,noprime,nomask,noedges); break; \
	case 0x8: macro(8,UFAST,dir,noskew,noprime,nomask,noedges); break; \
	case 0x9: macro(9,SLOW,dir,noskew,noprime,nomask,noedges); break; \
	case 0xA: break; \
	case 0xB: macro(B,SLOW,dir,noskew,noprime,nomask,noedges); break; \
	case 0xC: macro(C,FAST,dir,noskew,noprime,nomask,noedges); break; \
	case 0xD: macro(D,SLOW,dir,noskew,noprime,nomask,noedges); break; \
	case 0xE: macro(E,FAST,dir,noskew,noprime,nomask,noedges); break; \
	case 0xF: break; \
	} )

#else NO_OPS

#define	CASE_FILL(op,macro,nomask,noedges) _STMT( \
	switch (op) { \
	case 0xC: macro(C,FAST,nomask,noedges); break; \
	} )

#define	CASE_XOP(op,macro,noskew,noprime,nomask,noedges) _STMT( \
	switch (op) { \
	case 0xC: macro(C,FAST,noskew,noprime,nomask,noedges); break; \
	} )

#define	CASE_ROP(op,macro,dir,noskew,noprime,nomask,noedges) _STMT( \
	switch (op) { \
	case 0xC: macro(C,FAST,dir,noskew,noprime,nomask,noedges); break; \
	} )

#endif

/* narrow fills */
#define	FILLN(op,speed,nomask,noedges) _STMT( \
	_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
		_FILLN(op,IFTRUE), \
		_FILLN(op,IFFALSE)); \
	)

#define _FILLN(op,nomask) _STMT( \
	nomask( \
		k = _CAT(OP_ufgen,op)(k); \
	, \
		k = _CAT(OP_mfgen,op)(m,k); \
		m = _CAT(OP_mfmsk,op)(m); \
	) \
	LOOP(h, IFTRUE, \
		FWORD(op, nomask, t, *d, (TMP16) m, (TMP16) k); \
		d++); )

/* wide fills */
#define	FILLW(op,speed,nomask,noedges) _STMT( \
	_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
		_CAT(noedges,T)(_CAT(speed,T), edges == 0, \
			_FILLW(op,speed,IFTRUE,IFTRUE), \
			_FILLW(op,speed,IFTRUE,IFFALSE)), \
		_FILLW(op,speed,IFFALSE,IFFALSE)); \
	)

#define	_FILLW(op,speed,nomask,noedges) _STMT( \
	noedges(, \
		IFSPARC(, if (lm != 0)) \
			lk = _CAT(OP_mfgen,op)(lm, k); \
		lm = _CAT(OP_mfmsk,op)(lm); \
		IFSPARC(, if (rm != 0)) \
			rk = _CAT(OP_mfgen,op)(rm, k); \
		rm = _CAT(OP_mfmsk,op)(rm); \
	) \
	nomask( \
		k = _CAT(OP_ufgen,op)(k); \
	, \
		k = _CAT(OP_mfgen,op)(m, k); \
		m = _CAT(OP_mfmsk,op)(m); \
	) \
	/* fix w for unrolled loop */ \
	IFAND2(IFUNROLL,speed, w++;, ) \
	LOOP(h, IFTRUE, \
		noedges(, \
			if (lm != _CAT(OP_mfmsk,op)(0)) { \
				FWORD(op, IFFALSE, t, *d, lm, lk); \
				d++; \
			} \
		) \
		x = (LOOP_T) w; \
		IFAND2(IFUNROLL,speed, \
			while ((x -= UNROLL) >= 0) \
				FBLOCK(op, nomask, t, d, m, k); \
			x += UNROLL; \
			while LOOP_DECR(x) { \
				FWORD(op, nomask, t, *d, m, k); \
				d++; \
			} \
		, \
			noedges(, if (x >= 0)) \
				LOOP(x, IFTRUE, \
					FWORD(op, nomask, t, *d, m, k); \
					d++); \
		) \
		noedges(, \
			if (rm != _CAT(OP_mfmsk,op)(0)) \
				FWORD(op, IFFALSE, t, *d, rm, rk); \
		) \
		PTR_INCR(ROP_T *, d, doffset); \
	); )

/* narrow 1 -> n rops */
#define	XOPN(op,speed,noskew,noprime,nomask,noedges) _STMT( \
	_CAT(noprime,T)(_CAT(speed,T), sprime == 0, \
		_CAT(nomask,T)(_CAT(speed,T), m == (UMPR_T) ~0, \
			_XOPN(op,IFTRUE,IFTRUE), \
			_XOPN(op,IFTRUE,IFFALSE)), \
		_CAT(nomask,T)(_CAT(speed,T), m == (UMPR_T) ~0, \
			_XOPN(op,IFFALSE,IFTRUE), \
			_XOPN(op,IFFALSE,IFFALSE))); \
	)

#define	_XOPN(op,noprime,nomask) _STMT( \
	nomask(, m = _CAT(OP_mmsk,op)(m);) \
	LOOP(h, IFTRUE, \
		noprime( \
			scurr = *s++; \
		, \
			scurr = 0; \
			if (sprime != 0) \
				scurr = *s++ << 16; \
			if (sprime >= 0) \
				scurr |= *s++; \
		) \
		scurr >>= sshift; \
		scurr &= 3; \
		RWORD(op, nomask, t, *d, (TMP16) m, \
			(TMP16) smask[(TMP16) scurr]); \
		d++; \
		PTR_INCR(XOPS_T *, s, soffset)); )

/* wide 1 -> n rops */
#define	XOPW(op,speed,noskew,noprime,nomask,noedges) _STMT( \
	_CAT(noedges,T)(_CAT(speed,T), edges == 0, \
		_CAT(noprime,T)(_CAT(speed,T), sprime == 0 && sflush == 0, \
			_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
				_XOPW(op,IFFALSE,IFTRUE,IFTRUE,IFTRUE), \
				_XOPW(op,IFFALSE,IFTRUE,IFFALSE,IFTRUE)), \
			_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
				_XOPW(op,IFFALSE,IFFALSE,IFTRUE,IFTRUE), \
				_XOPW(op,IFFALSE,IFFALSE,IFFALSE,IFTRUE))), \
		_CAT(noprime,T)(_CAT(speed,T), sprime == 0 && sflush == 0, \
			_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
				_XOPW(op,IFFALSE,IFTRUE,IFTRUE,IFFALSE), \
				_XOPW(op,IFFALSE,IFTRUE,IFFALSE,IFFALSE)), \
			_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
				_XOPW(op,IFFALSE,IFFALSE,IFTRUE,IFFALSE), \
				_XOPW(op,IFFALSE,IFFALSE,IFFALSE,IFFALSE)))); \
	)

#define	_XOPW(op,noskew,noprime,nomask,noedges) _STMT( \
	noedges(, \
		lm = _CAT(OP_mmsk,op)(lm); \
		rm = _CAT(OP_mmsk,op)(rm); \
	) \
	nomask(, m = _CAT(OP_mmsk,op)(m);) \
	LOOP(h, IFFALSE, \
		sshift = skew; \
		scurr = 0; \
		noprime(, \
			if (sprime) \
				scurr |= *s++; \
		) \
		noedges(, \
			if (lm != _CAT(OP_mmsk,op)(0)) { \
				noskew(, \
					scurr <<= 16; \
				) \
				scurr |= *s++; \
				RWORD(op, IFFALSE, t, *d, lm, \
					smask[scurr >> sshift & 15]); \
				d++; \
			} \
		) \
		x = w; \
		noedges(, if (x >= 0)) \
			LOOP(x, IFTRUE, \
				if ((sshift -= sbits) < 0) { \
					sshift += 16; \
					noskew( \
						scurr = *s++; \
					, \
						scurr <<= 16; \
						scurr |= *s++; \
					) \
				} \
				RWORD(op, nomask, t, *d, m, \
					smask[scurr >> sshift & 15]); \
				d++); \
		noedges(, \
			if (rm != _CAT(OP_mmsk,op)(0)) { \
				if ((sshift -= sbits) < 0) { \
					sshift += 16; \
					noskew( \
						scurr = *s++; \
					, \
						scurr <<= 16; \
					) \
					noprime(, \
						if (sflush) \
							scurr |= *s++; \
					) \
				} \
				scurr >>= sshift; \
				scurr &= 15; \
				RWORD(op, IFFALSE, t, *d, rm, \
					smask[scurr]); \
			} \
		) \
		PTR_INCR(ROP_T *, d, doffset); \
		PTR_INCR(XOPS_T *, s, soffset); \
	); )

/* narrow rops */
#define	ROPN(op,speed,dir,noskew,noprime,nomask,noedges) _STMT( \
	_CAT(noskew,T)(_CAT(speed,T), rskew == 0, \
		_ROPN(op,IFTRUE,IFTRUE), \
		_CAT(noprime,T)(_CAT(speed,T), sprime == 0 && sflush == 0, \
			_ROPN(op,IFFALSE,IFTRUE), \
			_ROPN(op,IFFALSE,IFFALSE))); \
	)

#define	_ROPN(op,noskew,noprime) _STMT( \
	lm = _CAT(OP_mmsk,op)(lm); \
	rm = _CAT(OP_mmsk,op)(rm); \
	LOOP(h, IFTRUE, \
		noskew(, \
			sprev = 0; \
			noprime(, \
				if (sprime) \
					sprev |= *s++; \
			) \
		) \
		if (lm != (UMPR_T) _CAT(OP_mmsk,op)(0)) { \
			noskew( \
				RWORD(op, IFFALSE, t, *d, lm, *s++); \
				d++; \
			, \
				sprev <<= lskew; \
				IFSHIFT32(, \
					if (lskew >= 32) \
						sprev = 0; \
				) \
				scurr = *s++; \
				RWORD(op, IFFALSE, t, *d, lm, \
					sprev | scurr >> rskew); \
				d++; \
				sprev = scurr; \
			) \
		} \
		if (rm != (UMPR_T) _CAT(OP_mmsk,op)(0)) { \
			noskew( \
				RWORD(op, IFFALSE, t, *d, rm, *s); \
			, \
				sprev <<= lskew; \
				IFSHIFT32(, \
					if (lskew >= 32) \
						sprev = 0; \
				) \
				noprime(, \
					if (sflush) \
						sprev |= \
							*s >> rskew; \
				) \
				RWORD(op, IFFALSE, t, *d, rm, sprev); \
			) \
		} \
		PTR_INCR(UMPR_T *, d, doffset); \
		PTR_INCR(UMPR_T *, s, soffset); \
	); )

/* wide rops */
#define	ROPW(op,speed,dir,noskew,noprime,nomask,noedges) _STMT( \
	_CAT(noskew,T)(_CAT(speed,T), rskew == 0, \
		/* noskew implies noprime (at least for now) */ \
		_CAT(noedges,T)(_CAT(speed,T), edges == 0, \
			_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
				_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFTRUE,IFTRUE,IFTRUE,IFTRUE), \
				_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFTRUE,IFTRUE,IFFALSE,IFTRUE)), \
			_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
				_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFTRUE,IFTRUE,IFTRUE,IFFALSE), \
				_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFTRUE,IFTRUE,IFFALSE,IFFALSE))), \
		_CAT(noedges,T)(_CAT(speed,T), edges == 0, \
			/* noedges implies sflush == 0 */ \
			_CAT(noprime,T)(_CAT(speed,T), sprime == 0, \
				_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
					_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFFALSE,IFTRUE,IFTRUE,IFTRUE), \
					_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFFALSE,IFTRUE,IFFALSE,IFTRUE)), \
				_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
					_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFFALSE,IFFALSE,IFTRUE,IFTRUE), \
					_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFFALSE,IFFALSE,IFFALSE,IFTRUE))), \
			_CAT(nomask,T)(_CAT(speed,T), m == ~0, \
				_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFFALSE,IFFALSE,IFTRUE,IFFALSE), \
				_ROPW(op,speed,dir(IFTRUE,IFFALSE), \
					IFFALSE,IFFALSE,IFFALSE,IFFALSE)))); \
	)

#define	_ROPW(op,speed,dir,noskew,noprime,nomask,noedges) _STMT( \
	noedges(, \
		lm = _CAT(OP_mmsk,op)(lm); \
		rm = _CAT(OP_mmsk,op)(rm); \
	) \
	nomask(, \
		m = _CAT(OP_mmsk,op)(m); \
	) \
	/* fix w for unrolled loop */ \
	IFAND5(IFUNROLL,speed,dir,noskew,nomask,w++;, ) \
	LOOP(h, IFFALSE, \
	dir( \
		noskew(, \
			noprime(, \
				if (sprime) \
					sprev = *s++; \
			) \
		) \
		noedges(, \
			if (lm != _CAT(OP_mmsk,op)(0)) { \
				noskew( \
					RWORD(op, IFFALSE, t, *d, lm, *s++); \
					d++; \
				, \
					sprev <<= lskew; \
					IFSHIFT32(, \
						if (lskew >= 32) \
							sprev = 0; \
					) \
					scurr = *s++; \
					RWORD(op, IFFALSE, t, *d, lm, \
						sprev | scurr >> rskew); \
					d++; \
					sprev = scurr; \
				) \
			} \
		) \
	, \
		noedges( \
			noskew(, \
				if (rskew != 0) \
					scurr = *--s; \
			) \
		, \
			if (rm != _CAT(OP_mmsk,op)(0)) { \
				noskew( \
					RWORD(op, IFFALSE, t, *d, rm, *s); \
				, \
					if (sflush) { \
						scurr = *s; \
						if (rskew != 0) { \
							scurr >>= rskew; \
							sprev = *--s; \
						} \
					} \
					else { \
						scurr = 0; \
						sprev = *--s; \
					} \
					IFSHIFT32(, \
						if (lskew >= 32) \
							sprev = 0; \
					) \
					RWORD(op, IFFALSE, t, *d, rm, \
						sprev << lskew | scurr); \
					scurr = sprev; \
				) \
			} \
		noskew(, \
			else { \
				if (rskew != 0) \
					scurr = *--s; \
			} \
			if (rskew == 0) { \
				rskew = lskew; \
				lskew = 0; \
			} \
		) \
		) \
	) \
	x = w; \
	/* unrolled loop for fast, noskew, left to right case */ \
	IFAND5(IFUNROLL,speed,dir,noskew,nomask, \
		while ((x -= UNROLL) >= 0) \
			RBLOCK(op, nomask, t, d, m, s); \
		x += UNROLL; \
		while LOOP_DECR(x) { \
			RWORD(op, nomask, t, *d, m, *s); \
			d++; \
			s++; \
		} \
	, \
	noedges(, if (x >= 0)) \
	LOOP(x, IFTRUE, \
		noskew( \
			dir(, --s;) \
			dir(, --d;) \
			RWORD(op, nomask, t, *d, m, *s); \
			dir(s++; ,) \
			dir(d++; ,) \
		, \
			dir( \
				sprev <<= lskew; \
				IFSHIFT32(, \
					if (lskew >= 32) \
						sprev = 0; \
				) \
				scurr = *s++; \
				RWORD(op, nomask, t, *d, m, \
					sprev | scurr >> rskew); \
				d++; \
				sprev = scurr; \
			, \
				scurr >>= rskew; \
				IFSHIFT32(, \
					if (rskew >= 32) \
						scurr = 0; \
				) \
				sprev = *--s; \
				--d; \
				RWORD(op, nomask, t, *d, m, \
					sprev << lskew | scurr); \
				scurr = sprev; \
			) \
		) \
	); /* LOOP */ \
	) /* IFAND (unroll) */ \
	dir( \
		noedges(, \
			if (rm != _CAT(OP_mmsk,op)(0)) { \
				noskew( \
					RWORD(op, IFFALSE, t, *d, rm, *s); \
				, \
					sprev <<= lskew; \
					IFSHIFT32(, \
						if (lskew >= 32) \
							sprev = 0; \
					) \
					noprime(, \
						if (sflush) \
							scurr = *s; \
					) \
					RWORD(op, IFFALSE, t, *d, rm, \
						sprev | scurr >> rskew); \
				) \
			} \
		) \
	, \
		noedges(, \
			noskew(, \
				if (lskew == 0) { \
					lskew = rskew; \
					rskew = 0; \
				} \
			) \
			if (lm != _CAT(OP_mmsk,op)(0)) { \
				noskew( \
					--d; \
					RWORD(op, IFFALSE, t, *d, lm, *--s); \
				, \
					if (rskew == 0) \
						scurr = *--s; \
					scurr >>= rskew; \
					noprime(, \
						if (sprime) \
							sprev = *--s; \
					) \
					--d; \
					IFSHIFT32(, \
						if (lskew >= 32) \
							sprev = 0; \
					) \
					RWORD(op, IFFALSE, t, *d, lm, \
						sprev << lskew | scurr); \
				) \
			} \
		) \
	) \
		_ROPW_PINCR(d,dir,noskew); \
		_ROPW_PINCR(s,dir,noskew); \
	); )

#define	_ROPW_PINCR(p,dir,noskew) \
	PTR_INCR(ROP_T *, p, IFAND3(IFSPARC, dir, noskew, \
		(int) _CAT(_ROPW_POFF_,p), _CAT(p,offset)))

#define	_ROPW_POFF_d sprev
#define	_ROPW_POFF_s scurr


/* single word fill/rop macros */

/* XXX should be op-sensitive */
#define	IFUSETMP(op,a,b) a

#define	FWORD(op,nomask,t,d,m,k) \
	nomask(((d) = _CAT(OP_ufill,op)((d), (k))), \
		IFUSETMP(op, \
		_STMT((t) = (d); (d) = _CAT(OP_mfill,op)((t), (m), (k));), \
		((d) = _CAT(OP_mfill,op)((d), (m), (k)))))

#define	RWORD(op,nomask,t,d,m,s) \
	nomask(((d) = _CAT(OP_urop,op)((d), (s))), \
		IFUSETMP(op, \
		_STMT((t) = (d); (d) = _CAT(OP_mrop,op)((t), (m), (s));), \
		((d) = _CAT(OP_mrop,op)((d), (m), (s)))))


/* unrolled loop stuff */
#ifndef UNROLL
#define	UNROLL		IFSPARC(8,4)	/* larger cache on SPARC */
#endif
#if UNROLL > 0 && !defined(KERNEL)
#define	IFUNROLL	IFTRUE
#else
#define	IFUNROLL	IFFALSE
#endif
#define	UNARG(p,i)	IFSPARC((p)[(i)], *(p))
#define	UNINC(p)	IFSPARC(, (p)++)
#define	UNEND(p)	IFSPARC((p) += UNROLL, )

#define	FBLOCK(op,nomask,t,dp,m,k) _STMT( \
	FWORD(op,nomask,t,UNARG(dp,0),m,k); UNINC(dp); \
	FWORD(op,nomask,t,UNARG(dp,1),m,k); UNINC(dp); \
	FWORD(op,nomask,t,UNARG(dp,2),m,k); UNINC(dp); \
	FWORD(op,nomask,t,UNARG(dp,3),m,k); UNINC(dp); \
	IFSPARC( \
	FWORD(op,nomask,t,UNARG(dp,4),m,k); UNINC(dp); \
	FWORD(op,nomask,t,UNARG(dp,5),m,k); UNINC(dp); \
	FWORD(op,nomask,t,UNARG(dp,6),m,k); UNINC(dp); \
	FWORD(op,nomask,t,UNARG(dp,7),m,k); UNINC(dp); \
	, ) \
	UNEND(dp); \
)

#define	RBLOCK(op,nomask,t,dp,m,sp) _STMT( \
	RWORD(op,nomask,t,UNARG(dp,0),m,UNARG(sp,0)); UNINC(dp); UNINC(sp); \
	RWORD(op,nomask,t,UNARG(dp,1),m,UNARG(sp,1)); UNINC(dp); UNINC(sp); \
	RWORD(op,nomask,t,UNARG(dp,2),m,UNARG(sp,2)); UNINC(dp); UNINC(sp); \
	RWORD(op,nomask,t,UNARG(dp,3),m,UNARG(sp,3)); UNINC(dp); UNINC(sp); \
	IFSPARC( \
	RWORD(op,nomask,t,UNARG(dp,4),m,UNARG(sp,4)); UNINC(dp); UNINC(sp); \
	RWORD(op,nomask,t,UNARG(dp,5),m,UNARG(sp,5)); UNINC(dp); UNINC(sp); \
	RWORD(op,nomask,t,UNARG(dp,6),m,UNARG(sp,6)); UNINC(dp); UNINC(sp); \
	RWORD(op,nomask,t,UNARG(dp,7),m,UNARG(sp,7)); UNINC(dp); UNINC(sp); \
	, ) \
	UNEND(dp); UNEND(sp); \
)

/**************************************************************/

#ifdef SHLIB
#define	MEM_ROP	_mem_rop
#else
#define	MEM_ROP	mem_rop
#endif

MEM_ROP(dpr, dx, dy, dw, dh, op, spr, sx, sy)
Pixrect	*dpr;
int dx, dy, dw, dh;
int op;
Pixrect	*spr;
int sx, sy;
{
	int	color;		/* color argument */
	int	planes;		/* plane mask; ~0 means unmasked */

	MPR_T	*dimage;	/* dst image */
	ROP_T	*dfirst;	/* dst first word */
	int	dlinebytes;	/* dst linebytes */
	int	dxbits;		/* dst x, bits */
	int	dwbits;		/* dst width, bits */
	int	ddepth;		/* dst depth */
				/* -1, 0, 1, 2 -> 1, 8, 16, 32 bits */

	MPR_T	*simage;	/* src image */
	ROP_T	*sfirst;	/* src first word */
	int	slinebytes;	/* src linebytes */
	int	sxbits;		/* src x, bits */

	int	notrop;		/* not an n -> n rop */
	int	narrow;		/* dst or src is < 32 bits wide */
	IFKERNEL(, int rtolrop;) /* do rop from right to left */

	int	words;		/* complete dst words to be written */
	IFSPARC(, int edges;)	/* partial words on dst left, right */
	int	sprime;		/* src must be primed on each line */
	int	sflush;		/* src must be flushed on each line */

	int	skew;		/* src skew relative to dst */
	int	doffset;	/* dst line to line offset */
	int	soffset;	/* src line to line offset */

	ROP_T	lmask;		/* left edge dst mask */
	ROP_T	rmask;		/* right edge dst mask */

	{
		register Pixrect *rdpr = dpr;
		register Pixrect *rspr = spr;
		register struct mprp_data *dprd;
		register int rop;
		register int rcolor;
		register int rddepth = rdpr->pr_depth;
		register int rsdepth;

		/*
		 * We support 1, 8, 16 and 32 bit dsts.
		 * Src must have the same depth as dst, or depth 1.
		 */
		if (rspr != 0) {
			rsdepth = rspr->pr_depth;
			if (rddepth != rsdepth && rsdepth != 1)
				return PIX_ERR;
		}

		/* convert dst depth to pseudo-log code */
		switch (rddepth) {
		case 1:  rddepth = -1; break;
		case 8:  rddepth =  0; break;
		case 16: rddepth =  1; break;
		case 32: rddepth =  2; break;
		default: return PIX_ERR;
		}

		/* convert src depth */
		if (rsdepth == 1)
			rsdepth = -1;
		else
			rsdepth = rddepth;

		/* handle clipping */
		rop = op;
		if ((rop & PIX_DONTCLIP) == 0) {
			struct pr_subregion dst;
			struct pr_prpos src;
			extern int pr_clip();

			dst.pr = rdpr;
			dst.pos.x = dx;
			dst.pos.y = dy;
			dst.size.x = dw;
			dst.size.y = dh;

			src.pr = rspr;
			if (rspr != 0) {
				src.pos.x = sx;
				src.pos.y = sy;
			}

			(void) pr_clip(&dst, &src);

			dx = dst.pos.x;
			dy = dst.pos.y;
			dw = dst.size.x;
			dh = dst.size.y;

			if (rspr != 0) {
				sx = src.pos.x;
				sy = src.pos.y;
			}
		}

		/* check for zero-size op */
		if (dw <= 0 || dh <= 0)
			return 0;

		/* extract color and op code */
		rcolor = PIXOP_COLOR(rop);
		rop = PIXOP_OP(rop);

		/* correct op for reverse video dst */
		dprd = mprp_d(rdpr);
		if (rddepth < 0 &&
			dprd->mpr.md_flags & MP_REVERSEVIDEO)
			rop = PIX_OP_REVERSEDST(rop);

		/* discard source if op does not require it */
		if (OPBIT(rop) & NOSRCBIT) {
			spr = rspr = 0;
			rcolor = ~0;	/* for ~d -> d ^ s */
		}

#ifndef	KERNEL
		/*
		 * Identify weird type of fill: src is 1 x 1
		 */
		if (rspr != 0 &&
			rspr->pr_size.x == 1 && rspr->pr_size.y == 1) {
			register int newcolor;

			newcolor = pr_get(rspr, 0, 0);
			if (rsdepth >= 0 || rddepth < 0)
				rcolor = newcolor;
			else {
				if (rcolor == 0)
					rcolor = ~rcolor;
				if (newcolor == 0)
					rcolor = 0;
			}
			spr = rspr = 0;
		}
#endif	!KERNEL

		/* if it's a fill it can't be a 1 -> n */
		if (rspr == 0) 
			rsdepth = 0;
		/* not a fill, do some src things */
		else {
#if !mc68000 || !defined PIC
			register struct mpr_data *sprd;
#else
#define sprd ((struct mpr_data *) dprd)
#endif
			register int rslinebytes;

			/* if spr is not an mpr, let it do the work */
			if (MP_NOTMPR(rspr))
				return (*(rspr)->pr_ops->pro_rop)(rdpr, 
					dx, dy, dw, dh, 
					op | PIX_DONTCLIP, 
					rspr, sx, sy);

#ifndef sprd
			sprd = mpr_d(rspr);
#else
			dprd = mprp_d(rspr);
#endif

			/* correct for src offset */
			sx += sprd->md_offset.x;
			sy += sprd->md_offset.y;

			if (rsdepth < 0) {
				/* save sx as bit number */
				sxbits = sx;

				/* correct op for reverse video src */
				if (sprd->md_flags & MP_REVERSEVIDEO)
					rop = PIX_OP_REVERSESRC(rop);

				/* clear rsdepth if 1 bit -> 1 bit op */
				if (rddepth < 0)
					rsdepth = 0;
				/* 1 -> n, save register variable */
				else
					ddepth = rddepth;
			}
			else {
				/* convert sx to bit number */
				sxbits = sx << rsdepth + 3;

				/* clear rsdepth, not 1 bit -> n bit op */
				rsdepth = 0;
			}

			/* point to src pixrect image */
			simage = sprd->md_image;

			rslinebytes = sprd->md_linebytes;
			if (rsdepth == 0 && rslinebytes > 2) {
				/* compute first 32-bit word of src */
				sfirst = (ROP_T *) PTR_ADD(simage,
					pr_product(rslinebytes, sy));

				/* correct for misaligned pixrect data */
				if (((int) sfirst & 2) != 0) {
					PTR_INCR(ROP_T *, sfirst, -2);
					sxbits += 16;
				}

				PTR_INCR(ROP_T *, sfirst, sxbits >> 5 << 2);

				sxbits &= 31;
			}
			else {
				/* compute first 16-bit word of src */
				sfirst = (ROP_T *) PTR_ADD(simage,
					pr_product(rslinebytes, sy) +
					(sxbits >> 4 << 1));

				sxbits &= 15;
			}

			/* save register variable */
			slinebytes = rslinebytes;

#ifdef sprd
#undef sprd
			dprd = mprp_d(rdpr);
#endif sprd
		}

		{
			register int rdlinebytes;

			/* correct for dst offset */
			dx += dprd->mpr.md_offset.x;
			dy += dprd->mpr.md_offset.y;

			/* convert dx, dw to bit numbers */
			if (rddepth < 0) {
				dxbits = dx;
				dwbits = dw;
			}
			else {
				dxbits = dx << rddepth + 3;
				dwbits = dw << rddepth + 3;
			}

			/* point to dst pixrect image */
			dimage = dprd->mpr.md_image;

			if ((rdlinebytes = dprd->mpr.md_linebytes) > 2) {
				/* compute first 32-bit word of dst */
				dfirst = (ROP_T *) PTR_ADD(dimage,
					pr_product(rdlinebytes, dy));

				/* correct for misaligned pixrect data */
				if (((int) dfirst & 2) != 0) {
					PTR_INCR(ROP_T *, dfirst, -2);
					dxbits += 16;

					/* update dx for 1->n rops */
					if (rsdepth != 0) {
						switch (rddepth) {
						case -1: dx += 16; break;
						case  0: dx +=  2; break;
						case  1: dx +=  1; break;
						/* can't re-align 1->32 */
						case  2:
#if SHORT_ALIGN
							PTR_INCR(ROP_T *,
								dfirst, 2);
							dxbits -= 16;
							break;
#else SHORT_ALIGN
							return PIX_ERR;
#endif SHORT_ALIGN
						}

#if !SHORT_ALIGN
						/* 
						 * mung dimage so split rops
						 * will come out right even
						 * though dx was adjusted
						 */
						dimage--;
#endif !SHORT_ALIGN
					}
				}

				PTR_INCR(ROP_T *, dfirst, dxbits >> 5 << 2);

				dxbits &= 31;
			}
			else {
				/* compute first 16-bit word of dst */
				dfirst = (ROP_T *) PTR_ADD(dimage,
					pr_product(rdlinebytes, dy) +
					(dxbits >> 4 << 1));

				dxbits &= 15;
			}

			/* save register variable */
			dlinebytes = rdlinebytes;
		}

#if !SHORT_ALIGN
		/* split rop if we have to */
		if ((rspr == 0 || rsdepth != 0 ?
			dlinebytes & 2 :
			slinebytes > 2 &&
			(dlinebytes & 2 || slinebytes & 2)) &&
			dh > 1 && dlinebytes > 2) {
#ifdef KERNEL
			return PIX_ERR;
#else KERNEL

			Pixrect	dpr1, spr1;
			struct mprp_data dprd1;
			struct mpr_data sprd1;

			/*
			 * pathological overlapped scrolls
			 * are done line-by-line
			 */
			if (rspr != 0 && simage == dimage &&
				dx + dw > sx && dx < sx + dw &&
				dy + dh > sy && dy < sy + dh &&
				(dy & 1) != (sy & 1)) {
				register int yinc = 1;

				op |= PIX_DONTCLIP;

				/* southward rops are bottom-to-top */
				if (dy > sy) {
					dy += dh - 1;
					sy += dh - 1;
					yinc = -yinc;
				}

				while (--dh >= 0) {
					(void) MEM_ROP(rdpr, dx, dy, dw, 1, 
						op, rspr, sx, sy);

					dy += yinc;
					sy += yinc;
				}
				return 0;
			}

			dpr1 = *rdpr;
			dpr1.pr_data = (caddr_t) &dprd1;

			if (rspr != 0) {
				spr1 = *rspr;
				spr1.pr_data = (caddr_t) &sprd1;
			}

			dprd1.mpr.md_image = 
				(MPR_T *) PTR_ADD(dimage, dlinebytes);
			dprd1.mpr.md_linebytes = (dlinebytes <<= 1);
			dprd1.mpr.md_offset.x = 0;
			dprd1.mpr.md_offset.y = 0;
			dprd1.mpr.md_primary = 0;
			if ((dprd1.mpr.md_flags = dprd->mpr.md_flags) &
				MP_PLANEMASK)
				dprd1.planes = dprd->planes;

			if (dy & 1) 
				dprd1.mpr.md_image =
					(MPR_T *) PTR_ADD(dimage, dlinebytes);

			if (rspr != 0) {
				sprd1.md_image =
					(MPR_T *) PTR_ADD(simage, slinebytes);
				sprd1.md_linebytes = (slinebytes <<= 1);
				sprd1.md_offset.x = 0;
				sprd1.md_offset.y = 0;
				sprd1.md_primary = 0;
				sprd1.md_flags = mpr_d(rspr)->md_flags;

				if (sy & 1) 
					sprd1.md_image = (MPR_T *)
						PTR_ADD(simage, slinebytes);
			}

			/* do odd lines */
			(void) MEM_ROP(&dpr1, dx, dy >> 1, dw, dh >> 1,
				op | PIX_DONTCLIP, 
				rspr ? &spr1 : 0, sx, sy >> 1);

			/* do even lines */
			dh = (dh + 1) >> 1;
#endif KERNEL
		}
#endif !SHORT_ALIGN

		/*
		 * If op is fill or 1 bit -> n bits, replicate color
		 * bits for dst depth.
		 */
		if (rspr == 0 || rsdepth != 0) {
			/* if 1 -> n w/color 0, use ~0 */
			if (rsdepth != 0 && rcolor == 0)
				rcolor = ~0;
			else 
				switch (rddepth) {
				case -1: /* depth 1 */
					if (rcolor != 0)
						rcolor = ~0;
					break;

				case 0:	/* depth 8 */
					rcolor = rcolor & 0xff | 
						rcolor << 8;
					/* fall through */
				case 1: /* depth 16 */
					rcolor = rcolor & 0xffff | 
						rcolor << 16;
					break;

				case 2: /* depth 32 */
				/* if dst. is misaligned, swap halves */
					if (dxbits != 0)
						rcolor = rcolor << 16 |
							rcolor >> 16;
				}

			/* save register variable */
			color = rcolor;

			/* flag non-rop */
			notrop = ~0;
		}
		else
			notrop = 0;

		{
			register int rplanes = ~0;

			if (rddepth >= 0 &&
				dprd->mpr.md_flags & MP_PLANEMASK) {
				rplanes = dprd->planes;

				/* replicate plane mask for dst depth */
				switch (rddepth) {
				case 0:	/* depth 8 */
					rplanes &= 0xff;
					rplanes |= rplanes << 8;
					rplanes |= rplanes << 16;
					break;

				case 1: /* depth 16 */
					rplanes &= 0xffff;
					rplanes |= rplanes << 16;
					break;

				case 2: /* depth 32 */
					/* if dst is misaligned, swap halves */
					if (dxbits != 0)
						rplanes = rplanes << 16 |
							rplanes >> 16;
					break;
				}
			}

			/* fill op reduction */
			if (rspr == 0) {
				rcolor &= rplanes;

				/* special case if color is 0 or ~0 */
				if (rcolor == 0) {
					rop &= 3;
					rop |= rop << 2;
				}
				else if (rcolor == rplanes) {
					rop >>= 2;
					rop |= rop << 2;
				}

				/* convert ~d to d ^ s */
				if (rop == PIX_OPNOT(PIX_OPDST)) {
					rop = PIX_OPDST ^ PIX_OPSRC;
					color = ~0;
				}
				/* complement color if necessary */
				else if (OPBIT(rop) & INVSRCBIT)
					color = ~rcolor;
			}

			/* save register variable */
			planes = rplanes;
		}

		/* save register variable */
		op = rop;
	}

	/* calculate masks, skews, etc. */
	{
		register int lbit, rbit;
		register ROP_T rlmask, rrmask;
		register int rrskew;

		lbit = dxbits;
		rbit = lbit + dwbits;

		words = ((rbit >> 5) - ((lbit + 31) >> 5));
		if (words < 0)
			words = 0;

		rlmask = 0;
		if (lbit != 0)
			rlmask = (ROP_T) ~0 >> lbit;

		rrmask = (ROP_T) ~0;

		/* if dst is one word, merge left mask into right mask */
		if (rbit < 32 && rlmask != 0) {
			rrmask = rlmask;
			rlmask = 0;
		}

		if ((rbit &= 31) == 0)
			rbit = 32;
		else
			rrmask &= (ROP_T) ~0 << (32 - rbit);

		if (rrmask == (ROP_T) ~0)
			rrmask = 0;

		/* fills and 1 -> n ops */
		if (notrop) {
			
			/* classify as narrow or wide */
			if (dlinebytes == 2) {
				narrow = ~0;
				rrmask >>= 16;
			}
			else {
				narrow = 0;
				doffset = dlinebytes - (words << 2);
				if (rlmask != 0)
					doffset -= 4;
			}

			/* 1 -> n specific stuff */
			if (spr != 0) {

				rrskew = 0;
				sprime = 0;

				/* narrow 1 -> n ops */
				if (narrow) {
					soffset = slinebytes - 2;

					/* src straddles two shorts */
					if (ddepth == 0 && sxbits == 15) {
						/* need left half ? */
						if (lbit > 15) {
							/* no */
							sxbits = 0;
							sfirst = (ROP_T *)
								((XOPS_T *) 
								sfirst + 1);
						}
						else {
							sxbits = -1;

							/* need right half ? */
							if (rbit <= 16)  {
								/* no */
								sprime--;
							}
							else {
								sprime++;
								soffset -= 2;
							}
						}
					}
					rrskew = 16 - sxbits - (2 >> ddepth);
				}
				/* wide 1 -> n ops */
				else {
					/* compute # bits per src field */
					register int bits = (4 >> ddepth);

					/* adjust dw for src offset */
					dw += sxbits;

					/* compute src words/line */
					soffset = slinebytes -
						((dw + 15) >> 4 << 1);

					/*
				 	 * For single dst word 1 -> n ops
				 	 * we want to use the left mask only.
				 	 * This is inconsistent with rops and
				 	 * should be fixed.
				 	 */
					if (rlmask == 0 && words == 0) {
						rlmask = rrmask;
						rrmask = 0;
						doffset -= 4;
					}

					/*
					 * adjust sx for partial
					 * write of first dst word on left
					 */
					if (rlmask != 0) 
						sxbits -= dx & bits - 1;

					/* 
					 * compute how far to shift to get 
					 * the first src field in the low
					 * part of the source word
					 */
					rrskew = 16 - sxbits;

					/* 
					 * if a src field can be split 
					 * across words, we may have to
					 * read an extra word at the
					 * beginning or end of line
					 */
					sflush = 0;

					/* at start of line */
					if ((sxbits & (bits - 1)) != 0 &&
						rrskew < bits) {
						sprime++;
						if (words == 0)
							soffset -= 2;
					}

					/* at end of line */
					if ((dw &= 15) != 0 &&
						dw < bits)
						sflush++;

					/*
					 * adjust shift to valid range
					 */
					rrskew = rrskew - bits & 15;

					if (rlmask == 0) {
						/*
					 	 * adjust for initial subtract
					 	 * in center/right code
					 	 */
						rrskew += bits;

						/*
					 	 * force read of src since 
					 	 * it won't be done by the
						 * left partial code
					 	 */
						if (rrskew >= bits)
							rrskew -= 16;

						/* this may be redundant */
						if (sprime == 0 && words == 0)
							sflush++;
					}
				}
			}
		}

		/* rop specific stuff */
		else {
			sprime = 0;
			sflush = 0;
			rrskew = lbit - sxbits;
			if (dlinebytes == 2 || slinebytes == 2) {
				narrow = ~0;

				/* 
				 * if narrow dst may have to skip
				 * first short of src
				 */
				if (sxbits >= 16) {
					rrskew += 16;
					sfirst = (ROP_T *)
						((MPR_T *) sfirst + 1);
				}

				if (rrskew < 0)
					rrskew += 16;

				doffset = dlinebytes;
				soffset = slinebytes;

				/* right long only? */
				if (rlmask == 0) {
					/* move high part of rmask to lmask */
					rlmask = rrmask >> 16;
					rrmask &= 0xffff;
				}
				else {
					rlmask &= 0xffff;
					rrmask >>= 16;
					dfirst = (ROP_T *)
						((MPR_T *) dfirst + 1);
					rrskew -= 16;
				}

				/* right short only -- bump dst addr */
				if (rlmask == 0) {
					dfirst = (ROP_T *)
						((MPR_T *) dfirst + 1);
					lbit &= 15;
					rrskew &= 15;
				}

				/* left short only -- use right code */
				if (rrmask == 0) {
					rrmask = rlmask;
					rlmask = 0;
				}

				if (lbit < rrskew) {
					soffset -= 2;
					sprime++;
				}

				if (rlmask != 0)  {
					doffset -= 2;
					soffset -= 2;
				}

				if (rbit > rrskew)
					sflush++;
			}
			else {
				narrow = 0;

				if (rrskew < 0)
					rrskew += 32;

				doffset = dlinebytes - (words << 2);
				soffset = slinebytes - (words << 2);

				if (rlmask != 0) {
					doffset -= 4;
					soffset -= 4;
				}
				if (lbit < rrskew) {
					sprime++;
					soffset -= 4;
				}
				if (rrmask != 0) {
					if (rbit > rrskew)
						sflush++;
				}
			}

			/* assume left to right rop */
			IFKERNEL(, rtolrop = 0;)

			/*
			 * Rops are normally done top-to-bottom 
			 * left-to-right, but if src and dst are same 
			 * pixrect and overlap horizontally, it may be
			 * necessary to change this.
			 */
			if (dimage == simage &&
				dx + dw > sx && dx < sx + dw) {
#ifdef KERNEL
				return PIX_ERR;
#else
				register int offset;

				/*
				 * If src and dst overlap vertically,
				 * do southward rops from bottom to top.
				 */
				if (dy > sy) {
					if (dy < sy + dh) {
						/*
					 	 * line to line adjustment
					 	 */
						offset = dlinebytes << 1;

						doffset -= offset;
						soffset -= offset;

						/* 
					 	 * adjust dst and src to 
					 	 * beginning of last line
					 	 */
						offset >>= 1;
						offset = pr_product(offset, 
							dh - 1);

						PTR_INCR(ROP_T *, dfirst,
							offset);
						PTR_INCR(ROP_T *, sfirst,
							offset);
					}
				}
				/* 
				 * If src and dst overlap horizontally,
				 * do wide due east rops from right to left.
				 */
				else if (dy == sy && 
					dx > sx && 
					narrow == 0 && 
					(rlmask != 0 || words > 0)) {

					/* flag right to left rop */
					rtolrop++;

					/*
					 * adjust dst and src to end of
					 * first line
					 */
					offset = doffset - dlinebytes;

					PTR_INCR(ROP_T *, dfirst, -offset);

					doffset -= offset << 1;

					offset = soffset - slinebytes;

					PTR_INCR(ROP_T *, sfirst, -offset);

					soffset -= offset << 1;
				}
#endif KERNEL
			}
		}

		/* save register variables */
		skew  = rrskew;
#if !sparc
		edges = rlmask | rrmask;
#ifdef lint
		/* 
		 * Depending on CASE_OP options, edges may never be used.
		 * Frob it here to keep lint happy.
		 */
		if (edges != 0)
			edges++;
#endif lint
#endif !sparc
		lmask = rlmask;
		rmask = rrmask;
	}

	if (notrop) {
		register ROP_T m = planes;

		/* fills */
		if (spr == 0) {
			register ROP_T k = color;
			register LOOP_T h = dh - 1;

			if (narrow) {
				register UMPR_T t;
				register UMPR_T *d = (UMPR_T *) dfirst;

				m &= rmask;
				CASE_FILL(op, FILLN, NEVER, @);
			}
			else {
				register ROP_T t;
				register ROP_T *d = dfirst;
				register LOOP_T x;
				register ROP_T lm = lmask & m;
				register ROP_T rm = rmask & m;
				register INT_T w = (INT_T) (words - 1);
				register ROP_T lk, rk;
				register INT_T rdoffset = (INT_T) doffset;
#define	doffset	(int) rdoffset

#ifndef	KERNEL
				CASE_FILL(op, FILLW, OPTION, NEVER);
#else
				CASE_FILL(op, FILLW, OPTION, NEVER);
#endif

#undef doffset
			}
		}
		/* 1 bit -> n bit ops */
		else {
			register XOPS_T *s = (XOPS_T *) sfirst;
			register ROP_T scurr;

			/*
			 * Source mask tables -- used to expand 1-4 bits from
			 * the source word into 16 (XOPN) or 32 (XOPW) bits
			 * of destination pixels.
			 *
			 * If source color is ~0 we can use the precomputed
			 * table, otherwise we have to make a new one, anding
			 * in the color.
			 */

			static UMPR_T 
			mtabs[4 + 4] = {
				/* 8 bit dst */
				0x0000, 0x00ff, 0xff00, 0xffff,

				/* 16 bit dst */
				0x0000, 0xffff, 0x0000, 0xffff
			};

			static ROP_T
			mtabl[16 + 16 + 16] = {
				/* 8 bit dst */
				0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff,
				0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff,
				0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff,
				0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff,

				/* 16 bit dst */
				0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff,
				0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff,
				0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff,
				0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff,

				/* 32 bit dst */
				0x00000000, 0xffffffff, 0x00000000, 0xffffffff,
				0x00000000, 0xffffffff, 0x00000000, 0xffffffff,
				0x00000000, 0xffffffff, 0x00000000, 0xffffffff,
				0x00000000, 0xffffffff, 0x00000000, 0xffffffff
			};

			/* temporarily use scurr for src inversion flag */
			scurr = 0;
			if (OPBIT(op) & INVSRCBIT)
				scurr = ~scurr;

			if (narrow) {
				register UMPR_T t;
				register UMPR_T *d;
				register UMPR_T *smask = &mtabs[ddepth << 2];
				register unsigned sshift = skew;
				register LOOP_T h;
				register int rsprime = sprime;
#define	sprime	rsprime
#if sparc
				register int rsoffset = soffset;
#define	soffset rsoffset
#endif sparc
				/* custom mask table */
				UMPR_T mtab[4];

				t = color & m;
				if (t != (UMPR_T) m || scurr) {
					d = smask;
					smask = mtab;
					h = 4 - 1;
					PR_LOOPVP(h,
						*smask++ = (*d++ & t) ^ scurr);
					smask -= 4;
				}
				h = dh - 1;
				d = (UMPR_T *) dfirst;
				m &= rmask;

#ifndef KERNEL
				CASE_XOP(op, XOPN, @, OPTION, OPTION, @);
#else
				CASE_XOP(op, XOPN, @, NEVER, NEVER, @);
#endif

#undef sprime
#undef soffset
			}
			else {
				register ROP_T t;
				register ROP_T *d;
				register ROP_T *smask = &mtabl[ddepth << 4];
				register sshift;
				register sbits = 4 >> ddepth;
				register LOOP_T x;
				register ROP_T lm = lmask & m;
				register ROP_T rm = rmask & m;
				register LOOP_T h = dh - 1;
				register LOOP_T w = words - 1;

				/* custom mask table */
				ROP_T mtab[16];

				t = color & m;
				if (t != m || scurr) {
					d = smask;
					smask = mtab;
					x = 15;
					PR_LOOPVP(x,
						*smask++ = (*d++ & t) ^ scurr);
					smask -= 16;
				}
				d = dfirst;
#ifndef KERNEL
				CASE_XOP(op, XOPW,
					NEVER, OPTION, OPTION, OPTION);
#else
				CASE_XOP(op, XOPW,
					NEVER, NEVER, NEVER, NEVER);
#endif
			}
		}
	}

	/* n bit to n bit ops */
	else {
		register ROP_T m = planes;
		register unsigned rskew = skew;
		register unsigned lskew;

		if (narrow) {
			register UMPR_T t;
			register UMPR_T *d = (UMPR_T *) dfirst;
			register UMPR_T *s = (UMPR_T *) sfirst;
			register UMPR_T sprev, scurr;
			register LOOP_T h = dh - 1;
			register UMPR_T lm = lmask & m;
			register INT_T rdoffset = (INT_T) doffset;
			register INT_T rsoffset = (INT_T) soffset;
#define	doffset (int) rdoffset
#define	soffset (int) rsoffset
#define	rm m	/* use register variable m for right mask */

			rm &= rmask;

			lskew = 16 - rskew;

			CASE_ROP(op, ROPN, L_TO_R, NEVER, NEVER, @, @);

#undef doffset
#undef soffset
#undef rm
		}
		else {
			register ROP_T t;
			register ROP_T *d = dfirst;
			register ROP_T *s = sfirst;
			register ROP_T sprev, scurr;
			register LOOP_T x;
			register ROP_T lm = lmask & m;
			register ROP_T rm = rmask & m;
			register LOOP_T h = dh - 1;
			register LOOP_T w = words - 1;
#if mc68000
			register INT_T rdoffset = (INT_T) doffset;
			register INT_T rsoffset = (INT_T) soffset;
#define	doffset (int) rdoffset
#define	soffset (int) rsoffset
#endif

			lskew = 32 - rskew;

#ifdef KERNEL
			CASE_ROP(op, ROPW, L_TO_R,
				NEVER, NEVER, NEVER, NEVER);
#else KERNEL
			if (rtolrop == 0) {
#if sparc
				if (rskew == 0) {
					_ROPW_POFF_d = doffset;
					_ROPW_POFF_s = soffset;
				}
#endif sparc
				CASE_ROP(op, ROPW, L_TO_R, 
					OPTION, OPTION, OPTION, NEVER);
			}
			else
				CASE_ROP(op, ROPW, R_TO_L, 
					NEVER, NEVER, OPTION, NEVER);
#endif KERNEL

#undef doffset
#undef soffset
		}
	}

	/* successfully did something */
	return 0;
}
