#ifndef lint
static	char sccsid[] = "@(#)dcptest.c 1.1 94/10/31 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include <sys/types.h>
#include <stdio.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <nlist.h>
#include <sys/ioctl.h>
#include <des_crypt.h>
#include <sys/des.h>
#include "sdrtns.h"				/* sundiag standard include */
#include "../../../lib/include/libonline.h"	/* online library include */
#include "dcptest.h"
#include "dcpdata.h"
#include "dcptest_msg.h"

extern char *sprintf(), *getenv(), *getwd();
extern long lseek();
unsigned set_flags();
void des_setup();

int dcp_present = FALSE;
 
int errors = 0;
char device_dcpname[30]= "";
char *device = device_dcpname;
char msg[BUFSIZ-1];

struct des_info {
        char *key;              /* encryption key */
        char *ivec;             /* initialization vector: CBC mode only */
        unsigned flags;         /* direction, device flags */
        unsigned mode;          /* des mode: ecb or cbc */
}  g_des;

static int  check_hardware(), dcp_tests();
static int  no_hwdevice(), run_dcp_tests();
static int check_encrypt(), check_decrypt();
static void run_tests(), init_bufs();
static void check_des_dev_file();
static int probedcp();
static void check_eeprom();
static int  dummy(){return FALSE;}
/*
 * main (argc, argv) - get arguments, run tests                         
 * arguments: match - checks that some arguments match                  
 *            exec_by_sundiag - (s) sundiag mode = not verbose and runs
 *               for 4-5 minutes                                        
 *            d - debug                                      
 *            v - verbose                                           
 *            q - quick test -> 1 pass only                             
 *            e# - simulate error#                                      
 *            r - run on error                                         
 * run tests: runs dcp tests                                            
 */
main(argc, argv)
int     argc;
char    *argv[];
{
    versionid = "1.1";	/* SCCS version id */
			/* Begin Sundiag test */
    test_init(argc, argv, dummy, dummy, (char *)NULL);
    device_name = device;

    check_hardware();
    run_tests();
    test_end();		/* Sundiag normal exit */
}

/*
 * run_tests () - check hardware for dcp, run one pass of dcp tests    
 */
static void
run_tests()
{
    if (!run_dcp_tests()) 
        (void) send_message(-DCPTEST_ERROR, FATAL, test_failed_msg);
    else
        (void) send_message(0, VERBOSE, "Passed.");
}

/*
 * check_hardware () - check for the presence of the des device file,   
 *                     check the eeprom configuration                   
 */
static int
check_hardware()
{
   if (probe())
      dcp_present = TRUE;
   check_des_dev_file();
   if ((getenv("SUN_MANUFACTURING")) && 
       (strcmp(getenv("SUN_MANUFACTURING"), "yes") == 0))
      check_eeprom();
   if (dcp_present) {
      (void)send_message(0, VERBOSE, dcp_install_msg, "A");
      (void) strcpy(device, "DCP");
   } else {
      (void)send_message(0, VERBOSE, dcp_install_msg, "No");
      (void) strcpy(device, "softdcp");
   }
}

/*
 * check_des_dev_file () - check the file status of the des device file,
 *                     make it if it doesn't already exist              
 *                     remove it if the permissions/mode is wrong       
 *		       and make another one				
 */
static void
check_des_dev_file()
{
   struct stat f_stat;
   char fname[BUFSIZ-1];
   int stat1, no_dev_file = TRUE;
   char save_pathname[MAXPATHLEN];

   (void) sprintf(fname, "/dev/des");
   while (no_dev_file == TRUE) {
      if (dcp_present == TRUE) {
         if (stat(fname, &f_stat) == -1) {
	    (void)send_message(0, VERBOSE, no_device_msg, fname);
            (void) getwd(save_pathname);
            if ((stat1 = chdir("/dev/")) == 0) {
               (void) system("MAKEDEV std");
               (void) chdir(save_pathname);
            }
            if (stat1) 
	       (void)send_message(-PROBE_DES_ERROR, ERROR, cant_make_msg);
         } else if ((((f_stat.st_mode & S_IFMT) != S_IFCHR) &&
               ((f_stat.st_mode & S_IREAD) != S_IREAD)) &&
	       ((f_stat.st_mode & S_IREAD) != S_IWRITE)) {
	 	(void)send_message(0, VERBOSE, no_rw_msg, fname);
               (void) system("rm -f /dev/des");
         } else
	    no_dev_file = FALSE;
      } else if (stat(fname, &f_stat) == 0) {
	 (void)send_message(0, VERBOSE, file_nodcp_msg, fname);
         (void) system("rm -f /dev/des");
	 no_dev_file = FALSE;
      } else
	 no_dev_file = FALSE;
   }
}

/*
 * check_eeprom () - probe eeprom for appropriate DCP configuration     
 */
static void
check_eeprom()
{
   int probe = 0;

   if ((probe = probedcp()) == -1)
       (void) send_message(0, INFO, eeprom_err_msg);

   if (dcp_present && !probe)        /* physically, there is a DCP */
       (void) send_message(0, INFO, system_err_msg);

   if (!dcp_present && probe == 1)  /* no DCP */
       (void) send_message(0, INFO, install_err_msg);
}

/*
 * probedcp () - look for DCP/DES setting in eeprom                     
 *      returns: 1 - if configured OK                                   
 *               0 - if configured OK but no FF in next 2 bytes         
 *              -1 - if could not open/read/etc. eeprom                 
 */
static int
probedcp()
{
   long    seek_ok, offset = 0x0000BC;
   int     eeopen, pointer, val, i;
   char    ee_buffer[EEPROM_BYTES];
   int     nbytes = EEPROM_BYTES;
   unsigned short buf;
 
   pointer = L_SET;
   eeopen = open("/dev/eeprom",O_RDONLY);
   (void) send_message(0, DEBUG, eeopen_msg, eeopen);
   if (eeopen < 0) {
      (void) send_message(0, ERROR, eeopen_err_msg);
      return(-1);
   }
   for (i = 0; i < EEPROM_BOARDS; i++) {
      if ((seek_ok = lseek(eeopen, offset, pointer)) == -1) {
         (void) send_message(0, ERROR, lseek_err_msg);
         return(-1);
      }
      (void) send_message(0, DEBUG, lseek_pass_msg, seek_ok);
      if ((val = read(eeopen, ee_buffer, nbytes)) == -1) {
         (void) send_message(0, ERROR, read_err_msg);
         return(-1);
      }
      (void) send_message(0, DEBUG, val_msg, val);
      (void) sprintf(msg, "ee_buffer[0] = %x\nee_buffer[1] = %x\n\
	ee_buffer[2] = %x\nee_buffer[3] = %x\nee_buffer[4] = %x\n\
	ee_buffer[5] = %x\nee_buffer[6] = %x\nee_buffer[7] = %x\n\
	passed read\n", ee_buffer[0],ee_buffer[1],ee_buffer[2], ee_buffer[3], 
	ee_buffer[4], ee_buffer[5], ee_buffer[6], ee_buffer[7]);
      send_message(0, DEBUG, msg);

      if ((buf = ((short)ee_buffer[0]) & 0xFF) == 0xFF) {
	 send_message(0, ERROR, no_config_msg);
         return(-1);
      }
      if (ee_buffer[0] == 0x01) {      /* 0x01 = CPU board  */
         buf = ee_buffer[2];           /* installed options */
	 (void) send_message(0, DEBUG, buf_msg, buf);
         buf = (buf >> 1) & 0x01;      /* move DCP option to end and mask */
	 (void) send_message(0, DEBUG, buf_msg, buf);
         return((buf == 1) ? 1 : 0);
      }
      offset = 0;
      pointer = L_INCR;
      if (i == EEPROM_BOARDS) {
	 send_message(0, ERROR, bad_config_msg);
         return(-1);
      }        
   }
   return(-1);
}

/*
 * run_dcp_tests() - runs the tests, provides extra loops on the AMD tests
 *		     if the DCP chip is installed
 */
static int
run_dcp_tests()
{
   int save_errors, i;
   int loop_num = 1;

   save_errors = errors;

   if (!dcp_tests(DES_CBC, high_bit_parity, block))
      errors++;
   if (!dcp_tests(DES_CBC, low_bit_parity, block))
      errors++;
   if (!dcp_tests(DES_ECB, high_bit_parity, block))
      errors++;
   if (!dcp_tests(DES_ECB, low_bit_parity, block))
      errors++;
   if (!dcp_tests(DES_CBC, high_bit_parity, quick))
      errors++;
   if (!dcp_tests(DES_CBC, low_bit_parity, quick))
      errors++;
   if (!dcp_tests(DES_ECB, high_bit_parity, quick))
      errors++;
   if (!dcp_tests(DES_ECB, low_bit_parity, quick))
      errors++;

   if (dcp_present == TRUE)
      loop_num = LOOP_TESTS;

   for (i = 0; i < loop_num; i++) {
      if (!amd_ecb_test())
         errors++;
      if (!amd_cbc_test())
         errors++;
   }

   return((errors > save_errors) ? FALSE : TRUE);
}

/*
 * no_hwdevice () - checks the status of the DCP chip via the           
 *                  DESERR_NOHWDEVICE status return for ecb_crypt       
 *                  from des.c                                          
 */
static int
no_hwdevice()
{
   char key[8];
   char buf[8];

   return(ecb_crypt(key, buf, 8, DES_ENCRYPT | DES_HW) == DESERR_NOHWDEVICE);
}

/*
 * dcp_tests (mode, parity, Des_ioctl) -  test both CBC and ECD mode    
 *            encryption & decryption, with high bit and low bit parity 
 *            parity set in encryption key (test high bit parity to     
 *            mimic action of des.c) and try out both quick and block   
 *            ioctls - do CBC twice to check that ivec chaining works   
 */
static int
dcp_tests(mode, parity, des_ioctl)
   unsigned mode;
   Parity parity;
   Des_ioctl des_ioctl;
{
   char *inkey, *outkey;
   static char inkeybuf[KEY_BUFSIZ] = 
      { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
   static char outkeybuf[KEY_BUFSIZ] = 
      { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
   char block_testbuf[DES_MAXDATA-1], databuf[DES_MAXDATA-1];
   char chain_block_testbuf[DES_MAXDATA-1], chain_quick_testbuf[DES_QUICKLEN-1];
   char quick_testbuf[DES_QUICKLEN-1];
   unsigned flags = DES_HW;
   register int errs = 0, decrypt_errs = 0, encrypt_errs = 0;

   (void) sprintf(msg, start_mode_msg, (mode == DES_CBC) ? "CBC" : "ECB");
   (void) send_message(0, DEBUG, msg);

   inkey = inkeybuf;
   flags |= DES_ENCRYPT;
   flags = set_flags(flags);
   des_setup(inkey, mode, flags, parity);

   if (des_ioctl == block) {
      init_bufs(mode, databuf, block_testbuf, chain_block_testbuf, DES_MAXDATA);
      if (!docrypt(block_testbuf, DES_MAXDATA))
         errs++;
      if (mode == DES_CBC)
         if (!docrypt(chain_block_testbuf, DES_MAXDATA))
            errs++;
      encrypt_errs = check_encrypt(mode, databuf, block_testbuf, 
         chain_block_testbuf, DES_MAXDATA);
      errs = errs + encrypt_errs;
   } else {
      init_bufs(mode, databuf, quick_testbuf, chain_quick_testbuf,DES_QUICKLEN);
      if (!docrypt(quick_testbuf, DES_QUICKLEN))
         errs++;
      if (mode == DES_CBC)
         if (!docrypt(chain_quick_testbuf, DES_QUICKLEN))
            errs++;
      encrypt_errs = check_encrypt(mode, databuf, quick_testbuf, 
         chain_quick_testbuf, DES_QUICKLEN);
      errs += encrypt_errs;
   }

   outkey = outkeybuf;
   flags |= DES_DECRYPT;
   flags = set_flags(flags);
   des_setup(outkey, mode, flags, parity);

   if (des_ioctl == block) {
      if (!docrypt(block_testbuf, DES_MAXDATA))
         errs++;
      if (mode == DES_CBC)
         if (!docrypt(chain_block_testbuf, DES_MAXDATA))
            errs++;
      decrypt_errs = check_decrypt(mode, databuf, quick_testbuf, 
         chain_quick_testbuf, DES_MAXDATA);
      errs += decrypt_errs;
   } else {
      if (!docrypt(quick_testbuf, DES_QUICKLEN))
         errs++;
      if (mode == DES_CBC)
         if (!docrypt(chain_quick_testbuf, DES_QUICKLEN))
            errs++;
      decrypt_errs = check_decrypt(mode, databuf, quick_testbuf, 
         chain_quick_testbuf, DES_QUICKLEN);
      errs += decrypt_errs;
   }

   return(errs ? FALSE : TRUE);
}

/*
 * init_bufs(mode, databuf, testbuf, chain_testbuf, len) - initializes all 
 *	of the data for dcp_tests; len is the length of all *buf
 */
static void
init_bufs(mode, databuf, testbuf, chain_testbuf, len)
   unsigned mode;
   char *databuf;
   char *testbuf, *chain_testbuf;
   int len;
{
   register int i, j, base = 0;

   bzero(databuf, len);
   if (len == DES_MAXDATA) {
      for (i = 0; i < (DES_MAXDATA/DATA_BUFSIZ); i++, base += DATA_BUFSIZ) {
         for (j = 0; j < DATA_BUFSIZ; j++) 
            *(databuf+base+j) = (i%2)? char_buf[(DATA_BUFSIZ-j)-1]:char_buf[j];
      }  
   } else if (len == DES_QUICKLEN) {
      for (i = 0; i < len; i++)
         *(databuf+i) = char_buf[i];
   }

   for (i = 0; i < len; i++)
      *(testbuf+i) = databuf[i];
   if (mode == DES_CBC)
      for (i = 0; i < len; i++)
         *(chain_testbuf+i) = databuf[i];
}

/*
 * set_flags(flags) - sets the flags re: software vs hardware des - from des.c
 */
unsigned
set_flags(flags)
   unsigned flags;
{
   if ((flags & DES_DEVMASK) == DES_HW && no_hwdevice()) {
      flags &= ~DES_DEVMASK;
      flags |= DES_SW;
   }
   return(flags);
}

/*
 * check_encrypt(mode, databuf, testbuf, chain_testbuf, len) - makes sure that
 *	every piece of data has been encrypted, by checking vs. the original
 */
static int
check_encrypt(mode, databuf, testbuf, chain_testbuf, len)
   unsigned mode;
   char *databuf;
   char *testbuf, *chain_testbuf;
   int len;
{
   register int i, encrypt_errs = 0, errs = 0;

   for (i = 0; i++; i < len) {
      if (*(testbuf+i) == *(databuf+i))
         encrypt_errs++;
   }
   if (encrypt_errs == len) {
      (void) send_message(0, ERROR, encrypt_err_msg);
      errs++;
   }
   encrypt_errs = 0;
   if (mode == DES_CBC) {
      for (i = 0; i++; i < len) {
         if (*(chain_testbuf+i) == *(databuf+i))
            encrypt_errs++;
      }  
      if (encrypt_errs == len) {
         (void) send_message(0, ERROR, encrypt_err_msg);
         errs++;
      }
   }
   return(errs);
}

/*
 * check_encrypt(mode, databuf, testbuf, chain_testbuf, len) - makes sure that
 *	every piece of data has been decrypted, by checking vs. the original
 */
static int
check_decrypt(mode, databuf, testbuf, chain_testbuf, len)
   unsigned mode;
   char *databuf;
   char *testbuf, *chain_testbuf;
   int len;
{
   register int i, decrypt_errs = 0, errs = 0;

   for (i = 0; i++; i < len) {
      if (*(testbuf+i) != *(databuf+i))
         decrypt_errs++;
   }
   if (decrypt_errs == len) {
      (void) send_message(0, ERROR, encrypt_err_msg);
      errs++;
   }
   decrypt_errs = 0;
   if (mode == DES_CBC) {
      for (i = 0; i++; i < len) {
         if (*(chain_testbuf+i) != *(databuf+i))
             decrypt_errs++;
      }  
      if (decrypt_errs == len) {
         (void) send_message(0, ERROR, encrypt_err_msg);
         errs++;
      }
   }
   return(errs);
}

/*
 * des_setup (key, mode, flags, parity) -  from des.c, added call to    
 *                        des_setparity, if parity != high_bit_parity   
 */
void
des_setup(key, mode, flags, parity)
   char *key;
   unsigned mode, flags;
   Parity parity;
{
   static char ivec[8];

   g_des.flags = flags;
   g_des.mode = mode;
   if (parity == high_bit_parity)
      putparity(key);
   else
      des_setparity(key);
   g_des.key = key;

   bzero(ivec,8);
   g_des.ivec = ivec;
}

/*
 * putparity (p) -  from des.c
 */
static
putparity(p)
        char *p;
{
    int i;

    for (i = 0; i < 8; i++, p++) 
        *p = partab[*p & 0x7f];
}

/*
 * docrypt (buf, len) -  from des.c, added status error return check
 */
int
docrypt(buf,len)
        char *buf;
        unsigned len;
{
        int status;

        if (g_des.mode == DES_ECB)
           status = ecb_crypt(g_des.key, buf, len, g_des.flags);
        else
           status = cbc_crypt(g_des.key, buf, len, g_des.flags, g_des.ivec);

        return(DES_FAILED(status) ? 0 : 1);
}


/************************************************************
 Dummy routine to satisfy libsdrtns.a.
*************************************************************/
clean_up()
{
}
