/* @(#)umount.s 1.1 94/10/31 SMI */

#include "SYS.h"

#define SYS_umount	22
SYSCALL(umount)
	RET
