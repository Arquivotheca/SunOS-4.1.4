#ifndef lint
static	char sccsid[] = "@(#)dcp_amdtest.c 1.1 94/10/31 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <des_crypt.h>
#include "dcptest.h"
#include "dcp_amddata.h"
#include "sdrtns.h"

extern struct des_info {
        char *key;              /* encryption key */
        char *ivec;             /* initialization vector: CBC mode only */
        unsigned flags;         /* direction, device flags */
        unsigned mode;          /* des mode: ecb or cbc */
}  g_des;

extern char msg[];
/************************************************************************/
/* amd_ecb_test () -  tests ECB mode with a known set of data           */
/************************************************************************/

int
amd_ecb_test()
{
   char *inkey, *outkey;
   static char inkeybuf[KEY_BUFSIZ], outkeybuf[KEY_BUFSIZ];
   char databuf[KEY_BUFSIZ], testbuf[KEY_BUFSIZ];
   char dbuf1[BUFSIZ-1], dbuf2[BUFSIZ-1], dbuf3[BUFSIZ-1];
   short cbuf[KEY_BUFSIZ];
   unsigned flags, mode = DES_ECB;
   Parity parity = high_bit_parity;
   int i, j, errs = 0;
   short t;
 
   for(i = 0; i < NUM_ECB_TESTS; i++) {
      for (j = 0; j < KEY_BUFSIZ; j++)
         inkeybuf[j] = keybuf[j];
      inkey = inkeybuf;
      flags = DES_HW | DES_ENCRYPT;
      flags = set_flags(flags);
      des_setup(inkey, mode, flags, parity);
 
      for (j = 0; j < KEY_BUFSIZ; j++)
         testbuf[j] = plaintext[i][j];
      (void) sprintf(dbuf1, "%2x%2x%2x%2x%2x%2x%2x%2x", 
         plaintext[i][0], plaintext[i][1], plaintext[i][2], plaintext[i][3], 
         plaintext[i][4], plaintext[i][5], plaintext[i][6], plaintext[i][7]);
      (void) sprintf(dbuf2, "%2x%2x%2x%2x%2x%2x%2x%2x", 
         testbuf[0], testbuf[1], testbuf[2], testbuf[3], 
         testbuf[4], testbuf[5], testbuf[6], testbuf[7]);
      send_message(0, DEBUG, "plaintext = %s, testbuf   = %s\n", dbuf1, dbuf2);
      if (!docrypt(testbuf, KEY_BUFSIZ))
         errs++;

      bzero(dbuf3, BUFSIZ-1);
      (void) sprintf(dbuf3, "%2x%2x%2x%2x%2x%2x%2x%2x", 
        ecb_ciphertext[i][0], ecb_ciphertext[i][1], ecb_ciphertext[i][2], 
        ecb_ciphertext[i][3], ecb_ciphertext[i][4], ecb_ciphertext[i][5], 
        ecb_ciphertext[i][6], ecb_ciphertext[i][7]);
      bzero(cbuf, KEY_BUFSIZ);
      for (j = 0; j < KEY_BUFSIZ; j++) {
         t = (short)testbuf[j];
         cbuf[j] = t & 0x000000ff; 
      }
      bzero(dbuf2, BUFSIZ-1);
      (void) sprintf(dbuf2, "%2x%2x%2x%2x%2x%2x%2x%2x", cbuf[0], cbuf[1], 
                     cbuf[2], cbuf[3], cbuf[4], cbuf[5], cbuf[6],cbuf[7]);
      send_message(0, DEBUG, "cipherbuf = %s, cbuf      = %s\n", dbuf3, dbuf2);
      if (strcmp(dbuf3, dbuf2))
         errs++;

      for (j = 0; j < KEY_BUFSIZ; j++)
         outkeybuf[j] = keybuf[j];
      outkey = outkeybuf;
      flags |= DES_DECRYPT;
      flags = set_flags(flags);
      des_setup(outkey, mode, flags, parity);

      if (!docrypt(testbuf, KEY_BUFSIZ))
         errs++;
      bzero(dbuf2, BUFSIZ-1);
      (void) sprintf(dbuf2, "%2x%2x%2x%2x%2x%2x%2x%2x", 
         testbuf[0], testbuf[1], testbuf[2], testbuf[3], 
         testbuf[4], testbuf[5], testbuf[6], testbuf[7]);
      if (strcmp(dbuf1, dbuf2))
         errs++;
      send_message(0, DEBUG, "plaintext = %s, testbuf   = %s\n", dbuf1, dbuf2);
   }
   return((errs > 0) ? FALSE : TRUE);
}

/************************************************************************/
/* amd_cbc_test () -  tests CBC mode with a known set of data           */
/************************************************************************/

int
amd_cbc_test()
{
   char *inkey, *outkey;
   static char inkeybuf[KEY_BUFSIZ], outkeybuf[KEY_BUFSIZ];
   char databuf[KEY_BUFSIZ], testbuf[NUM_CBC_TESTS][KEY_BUFSIZ];
   char dbuf1[BUFSIZ-1], dbuf2[BUFSIZ-1], dbuf3[BUFSIZ-1];
   short cbuf[KEY_BUFSIZ];
   unsigned flags, mode = DES_CBC;
   Parity parity = low_bit_parity;
   int i, j, errs = 0;
   short t;
 
   for (j = 0; j < KEY_BUFSIZ; j++)
      inkeybuf[j] = keybuf[j];
   inkey = inkeybuf;
   flags = DES_HW | DES_ENCRYPT;
   flags = set_flags(flags);
   des_setup(inkey, mode, flags, parity);
 
   for(i = 0; i < NUM_CBC_TESTS; i++) {
      for (j = 0; j < KEY_BUFSIZ; j++)
         testbuf[i][j] = plaintext[i][j];
      (void) sprintf(dbuf1, "%2x%2x%2x%2x%2x%2x%2x%2x", 
         plaintext[i][0], plaintext[i][1], plaintext[i][2], plaintext[i][3], 
         plaintext[i][4], plaintext[i][5], plaintext[i][6], plaintext[i][7]);
      (void) sprintf(dbuf2, "%2x%2x%2x%2x%2x%2x%2x%2x", 
         testbuf[i][0], testbuf[i][1], testbuf[i][2], testbuf[i][3], 
         testbuf[i][4], testbuf[i][5], testbuf[i][6], testbuf[i][7]);
      send_message(0, DEBUG, "plaintext = %s, testbuf   = %s\n", dbuf1, dbuf2);
      if (!docrypt(testbuf[i], KEY_BUFSIZ))
         errs++;

      bzero(dbuf3, BUFSIZ-1);
      (void) sprintf(dbuf3, "%2x%2x%2x%2x%2x%2x%2x%2x", 
        cbc_ciphertext[i][0], cbc_ciphertext[i][1], cbc_ciphertext[i][2], 
        cbc_ciphertext[i][3], cbc_ciphertext[i][4], cbc_ciphertext[i][5], 
        cbc_ciphertext[i][6], cbc_ciphertext[i][7]);
      bzero(cbuf, KEY_BUFSIZ);
      for (j = 0; j < KEY_BUFSIZ; j++) {
         t = (short)testbuf[i][j];
         cbuf[j] = t & 0x000000ff; 
      }
      bzero(dbuf2, BUFSIZ-1);
      (void) sprintf(dbuf2, "%2x%2x%2x%2x%2x%2x%2x%2x", cbuf[0], cbuf[1], 
                     cbuf[2], cbuf[3], cbuf[4], cbuf[5], cbuf[6],cbuf[7]);
      send_message(0, DEBUG, "cipherbuf = %s, cbuf      = %s\n", dbuf3, dbuf2);
      if (strcmp(dbuf3, dbuf2))
         errs++;
   }

   for (j = 0; j < KEY_BUFSIZ; j++)
      outkeybuf[j] = keybuf[j];
   outkey = outkeybuf;
   flags |= DES_DECRYPT;
   flags = set_flags(flags);
   des_setup(outkey, mode, flags, parity);

   for(i = 0; i < NUM_CBC_TESTS; i++) {
      if (!docrypt(testbuf[i], KEY_BUFSIZ))
         errs++;
      bzero(dbuf1, BUFSIZ-1);
      (void) sprintf(dbuf1, "%2x%2x%2x%2x%2x%2x%2x%2x", 
         plaintext[i][0], plaintext[i][1], plaintext[i][2], plaintext[i][3], 
         plaintext[i][4], plaintext[i][5], plaintext[i][6], plaintext[i][7]);
      bzero(dbuf2, BUFSIZ-1);
      (void) sprintf(dbuf2, "%2x%2x%2x%2x%2x%2x%2x%2x", 
         testbuf[i][0], testbuf[i][1], testbuf[i][2], testbuf[i][3], 
         testbuf[i][4], testbuf[i][5], testbuf[i][6], testbuf[i][7]);
      if (strcmp(dbuf1, dbuf2))
         errs++;
      send_message(0, DEBUG, "plaintext = %s, testbuf   = %s\n", dbuf1, dbuf2);
   }
   return((errs > 0) ? FALSE : TRUE);
}
