
/*     @(#)dcp_amddata.h 1.1 94/10/31 SMI      */

/* 
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */
#define NUM_ECB_TESTS 3
#define NUM_CBC_TESTS 3
#define NUM_LONG_TESTS 1

static char keybuf[KEY_BUFSIZ] =
   { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

static char plaintext[NUM_ECB_TESTS][KEY_BUFSIZ] =
   { 0x4e, 0x6f, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74,
     0x68, 0x65, 0x20, 0x74, 0x69, 0x6b, 0x65, 0x20,
     0x66, 0x6f, 0x72, 0x20, 0x61, 0x6c, 0x6c, 0x20 };

static short ecb_ciphertext[NUM_ECB_TESTS][KEY_BUFSIZ] =
   { 0x3f, 0xa4, 0x0e, 0x8a, 0x98, 0x4d, 0x48, 0x15,
     0xd4, 0xce, 0x05, 0x6a, 0x69, 0xde, 0x60, 0x94,
     0x89, 0x3d, 0x51, 0xec, 0x4b, 0x56, 0x3b, 0x53 };

static short cbc_ciphertext[NUM_CBC_TESTS][KEY_BUFSIZ] =
   { 0x41, 0xd9, 0x29, 0x94, 0x43, 0xbe, 0x33, 0x8b,
     0x0f, 0x0d, 0xc6, 0x01, 0xa1, 0x92, 0xaa, 0xbe,
     0xdc, 0x34, 0x16, 0x22, 0x21, 0xe5, 0x1d, 0x8f };
