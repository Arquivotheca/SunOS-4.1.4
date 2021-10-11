/*
 *      .seg    "data"
 *      .asciz  "@(#)strcpy.s 1.1 94/10/31"
 *      Copyright (c) 1987 by Sun Microsystems, Inc.
 * FINAL VERSION 
 */

#include <sun4/asm_linkage.h>

        .seg    "text"
        .align  4

#define DEST    %i0
#define SRC     %i1
#define DESTSV  %i2
#define ADDMSK  %l0
#define ANDMSK  %l1
#define MSKB0   %l2
#define MSKB1   %l3
#define MSKB2   %l4
#define SL      %o0
#define SR      %o1
#define MSKB3   0xff

/*
 * strcpy(s1, s2)
 * Copy string s2 to s1.  s1 must be large enough.
 * return s1
 */
        ENTRY(strcpy)
        save    %sp, -SA(WINDOWSIZE), %sp       ! get a new window
        andcc   SRC, 3, %i3             ! is src word aligned
        bz      aldest
        mov     DEST, DESTSV            ! save return value
        cmp     %i3, 2                  ! is src half-word aligned
        be      s2algn
        cmp     %i3, 3                  ! src is byte aligned
        ldub    [SRC], %i3              ! move 1 or 3 bytes to align it
        inc     1, SRC
        stb     %i3, [DEST]             ! move a byte to align src
        be      s3algn
        tst     %i3
s1algn: bnz     s2algn                  ! now go align dest
        inc     1, DEST
        b,a     done
s2algn: lduh    [SRC], %i3              ! know src is 2 byte alinged
        inc     2, SRC
        srl     %i3, 8, %i4
        tst     %i4
        stb     %i4, [DEST]
        bnz,a   1f
        stb     %i3, [DEST + 1]
        b,a     done
1:      andcc   %i3, MSKB3, %i3
        bnz     aldest
        inc     2, DEST
        b,a     done
s3algn: bnz     aldest
        inc     1, DEST
        b,a     done

aldest: 
        set     0x7efefeff, ADDMSK      ! masks to test for terminating null
        set     0x81010100, ANDMSK
        sethi   %hi(0xff000000), MSKB0
        sethi   %hi(0x00ff0000), MSKB1

        ! source address is now aligned
        andcc   DEST, 3, %i3            ! is destination word aligned?
        bz      w4str
        srl     MSKB1, 8, MSKB2         ! generate 0x0000ff00 mask
        cmp     %i3, 2                  ! is destination halfword aligned?
        be      w2str                   
        cmp     %i3, 3                  ! worst case, dest is byte alinged
w1str:  ld      [SRC], %i3              ! read a word
        inc     4, SRC                  ! point to next one
        be      w3str
        andcc   %i3, MSKB0, %g0         ! check if first byte was zero
        bnz     1f
        andcc   %i3, MSKB1, %g0         ! check if second byte was zero
        b,a     done1
1:      bnz     1f
        andcc   %i3, MSKB2, %g0         ! check if third byte was zero
        b,a     w1done2
1:      bnz     1f
        andcc   %i3, MSKB3, %g0         ! check if last byte is zero
        b,a     w1done3
1:      bz      w1done4
        srl     %i3, 24, %o3            ! move three bytes to align dest
        stb     %o3, [DEST]
        srl     %i3, 8, %o3
        sth     %o3, [DEST + 1]
        inc     3, DEST                 ! destination now aligned
        mov     %i3, %o3
        mov     24, SL
        b       8f                      ! inner loop same for w1str and w3str
        mov     8, SR                   ! shift amounts are different

2:      inc     4, DEST
8:      sll     %o3, SL, %o2            ! save remaining byte
        andcc   %o2,MSKB0,%g0           ! mhm check first byte for zero
        bz,a    done1                   ! mhm if first byte zero,branch
        andcc   %o2,MSKB0,%g0           ! mhm delay statement
        ld      [SRC], %o3              ! read a word
        inc     4, SRC                  ! point to next one
        srl     %o3, SR, %i3
        or      %i3, %o2, %i3

        add     %i3, ADDMSK, %l5        ! check for a zero byte 
        xor     %l5, %i3, %l5
        and     %l5, ANDMSK, %l5
        cmp     %l5, ANDMSK
        be,a    2b                      ! if no zero byte in word, loop
        st      %i3, [DEST]

        andcc   %i3, MSKB0, %g0         ! check if first byte was zero
        bnz     1f
        andcc   %i3, MSKB1, %g0         ! check if second byte was zero
        b,a     done1
1:      bnz     1f
        andcc   %i3, MSKB2, %g0         ! check if third byte was zero
        b,a     w1done2
1:      bnz     1f
        andcc   %i3, MSKB3, %g0         ! check if last byte is zero
        b,a     w1done3
1:      st      %i3, [DEST]             ! it is safe to write the word now
        bnz     8b                      ! if not zero, go read another word
        inc     4, DEST                 ! else finished
        b,a     done

w1done4:
        stb     %i3, [DEST + 3]
w1done3:
        srl     %i3, 8, %o3
        stb     %o3, [DEST + 2]
w1done2:
        srl     %i3, 24, %o3
        stb     %o3, [DEST]
        srl     %i3, 16, %o3
        b       done
        stb     %o3, [DEST + 1]

w3str:  bnz     1f
        andcc   %i3, MSKB1, %g0         ! check if second byte was zero
        b,a     done1
1:      bnz     1f
        andcc   %i3, MSKB2, %g0         ! check if third byte was zero
        b,a     w1done2
1:      bnz     1f
        andcc   %i3, MSKB3, %g0         ! check if last byte is zero
        b,a     w1done3
1:      bz      w1done4
        srl     %i3, 24, %o3
        stb     %o3, [DEST]
        inc     1, DEST
        mov     %i3, %o3
        mov     8, SL
        b       8f                      ! inner loop same for w1str and w3str
        mov     24, SR                  ! shift amounts are different

2:      inc     4, DEST                 ! MHM  ADDED THIS WHOLE BLOCK HERE
8:      sll     %o3, SL, %o2            ! save remaining byte
        andcc   %o2, MSKB0, %g0         ! check if first byte was zero
        bnz     1f
        andcc   %o2, MSKB1, %g0         ! check if second byte was zero
        b,a     done1
1:      bnz     1f
        andcc   %o2, MSKB2, %g0         ! check if third byte was zero
        b,a     r2don2                  ! MHM CHANGED TO r2done2
1:      bz,a    r2don3                  ! MHM CHANGED TO r3done3
        andcc   %i3, MSKB3, %g0         ! Delay statement
        ld      [SRC], %o3              ! read a word
        inc     4, SRC                  ! point to next one
        srl     %o3, SR, %i3
        or      %i3, %o2, %i3

        add     %i3, ADDMSK, %l5        ! check for a zero byte 
        xor     %l5, %i3, %l5
        and     %l5, ANDMSK, %l5
        cmp     %l5, ANDMSK
        be,a    2b                      ! if no zero byte in word, loop
        st      %i3, [DEST]

        andcc   %i3, MSKB0, %g0         ! check if first byte was zero
        bnz     1f
        andcc   %i3, MSKB1, %g0         ! check if second byte was zero
        b,a     done1
1:      bnz     1f
        andcc   %i3, MSKB2, %g0         ! check if third byte was zero
        b,a     w1done2
1:      bnz     1f
        andcc   %i3, MSKB3, %g0         ! check if last byte is zero
        b,a     w1done3
1:      st      %i3, [DEST]             ! it is safe to write the word now
        bnz     8b                      ! if not zero, go read another word
        inc     4, DEST                 ! else finished
        b,a     done                    ! MHM END OF ADDED BLOCK

w2done4:
        srl     %i3, 16, %o3
        sth     %o3, [DEST]
        b       done
        sth     %i3, [DEST + 2]
        
w2str:  ld      [SRC], %i3              ! read a word
        inc     4, SRC                  ! point to next one
        andcc   %i3, MSKB0, %g0         ! check if first byte was zero
        bnz     1f
        andcc   %i3, MSKB1, %g0         ! check if second byte was zero
        b,a     done1
1:      bnz     1f
        andcc   %i3, MSKB2, %g0         ! check if third byte was zero
        b,a     done2
1:      bnz     1f
        andcc   %i3, MSKB3, %g0         ! check if last byte is zero
        b,a     done3
1:      bz      w2done4

        srl     %i3, 16, %o3
        sth     %o3, [DEST]
        inc     2, DEST
        b       9f
        mov     %i3, %o3

2:      inc     4, DEST
9:      sll     %o3, 16, %o2            ! save rest
        andcc   %o2,MSKB0,%g0           ! mhm check first byte for zero
        bnz     3f                      ! mhm branch if first byte not null
        andcc   %o2,MSKB1,%g0           ! mhm check 2nd byte for zero
        b,a     done1                   ! mhm
3:      bz,a    r2don2                  ! mhm 2nd byte is a null
        andcc   %o2,MSKB1,%g0           ! mhm check 2nd byte for zero
        ld      [SRC], %o3              ! read a word
        inc     4, SRC                  ! point to next one
        srl     %o3, 16, %i3
        or      %i3, %o2, %i3

        add     %i3, ADDMSK, %l5        ! check for a zero byte 
        xor     %l5, %i3, %l5
        and     %l5, ANDMSK, %l5
        cmp     %l5, ANDMSK
        be,a    2b                      ! if no zero byte in word, loop
        st      %i3, [DEST]

        andcc   %i3, MSKB0, %g0         ! check if first byte was zero
        bnz     1f
        andcc   %i3, MSKB1, %g0         ! check if second byte was zero
        b,a     done1
1:      bnz     1f
        andcc   %i3, MSKB2, %g0         ! check if third byte was zero
        b,a     done2
1:      bnz     1f
        andcc   %i3, MSKB3, %g0         ! check if last byte is zero
        b,a     done3
1:      st      %i3, [DEST]             ! it is safe to write the word now
        bnz     9b                      ! if not zero, go read another word
        inc     4, DEST                 ! else fall through
        b,a     done

2:      inc     4, DEST
w4str:  ld      [SRC], %i3              ! read a word
        inc     4, %i1                  ! point to next one

        add     %i3, ADDMSK, %l5        ! check for a zero byte 
        xor     %l5, %i3, %l5
        and     %l5, ANDMSK, %l5
        cmp     %l5, ANDMSK
        be,a    2b                      ! if no zero byte in word, loop
        st      %i3, [DEST]

        andcc   %i3, MSKB0, %g0         ! check if first byte was zero
        bnz     1f
        andcc   %i3, MSKB1, %g0         ! check if second byte was zero
        b,a     done1
1:      bnz     1f
        andcc   %i3, MSKB2, %g0         ! check if third byte was zero
        b,a     done2
1:      bnz     1f
        andcc   %i3, MSKB3, %g0         ! check if last byte is zero
        b,a     done3
1:      st      %i3, [DEST]             ! it is safe to write the word now
        bnz     w4str                   ! if not zero, go read another word
        inc     4, DEST                 ! else fall through

done:   ret                     ! last byte of word was the terminating zero 
        restore DESTSV, %g0, %o0

done1:  stb     %g0, [DEST]     ! first byte of word was the terminating zero
        ret     
        restore DESTSV, %g0, %o0

done2:  srl     %i3, 16, %i4    ! second byte of word was the terminating zero
        sth     %i4, [DEST]
        ret     
        restore DESTSV, %g0, %o0

done3:  srl     %i3, 16, %i4    ! third byte of word was the terminating zero
        sth     %i4, [DEST]
        stb     %g0, [DEST + 2]
        ret     
        restore DESTSV, %g0, %o0

r2don2: srl     %o2, 16, %i4    ! mhm second byte of word was terminating zero
        sth     %i4, [DEST]     ! mhm write out contents of o2 
        ret                     ! mhm
        restore DESTSV, %g0, %o0

r2don3: srl     %o2, 16, %i4    ! mhm third byte of word was terminating zero
        sth     %i4, [DEST]     ! mhm write out contents of o2
        stb     %g0, [DEST + 2] ! mhm 
        ret                     ! mhm
        restore DESTSV, %g0, %o0

