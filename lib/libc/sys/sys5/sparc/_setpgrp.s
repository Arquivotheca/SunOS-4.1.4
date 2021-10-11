/*	@(#)_setpgrp.s 1.1 94/10/31 SMI; from UCB 4.1 82/12/04	*/

#include "SYS.h"

BSDSYSCALL(setpgrp)
	RET		/* _setpgrp(pid, pgrp); */
