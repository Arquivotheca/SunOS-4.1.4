/*
  @(#)ATS tcm_dtm.h 1.1 94/10/31 Copyright Sun Microsystems Inc.
*/
/*
 * tcm_dtm.h
 *
 *	This files lists the rpc proc nums for messages from tcm to dtm.
 *
 */

struct strings {
    int num;
    char op[5][80];
};

struct option_file {
  char fname[80];
  int action;
};

extern xdr_strings();
extern xdr_option();
