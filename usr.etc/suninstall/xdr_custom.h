/*
 *
 * @(#)xdr_custom.h 1.1 94/10/31 Copyr 1987 Sun Micro
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 *
 */

struct futureslop { char dummy; };
extern bool_t xdr_futureslop();

struct nomedia { char dummy; };
extern bool_t xdr_nomedia();
