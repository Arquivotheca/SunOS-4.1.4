# 1 "gp1_shmem.IL" 









# 12 "gp1_shmem.IL" 




# 18 "gp1_shmem.IL" 


# 40 "gp1_shmem.IL" 


# 59 "gp1_shmem.IL" 








	.inline	__gp1_lock,4
	sethi	%hi((2000 * 1000) * 8), %o2
	ldstub	[%o0], %o1
1:	tst	%o1
	bz,a	2f
	mov	0, %o0
	deccc	%o2
	bge,a	1b
	ldstub	[%o0], %o1
	mov	-1, %o0
2:
	.end

